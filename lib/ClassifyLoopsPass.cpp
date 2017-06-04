//
//
//

#include "llvm/Pass.h"
// using llvm::RegisterPass

#include "llvm/IR/Module.h"
// using llvm::Module

#include "llvm/IR/Function.h"
// using llvm::Function

#include "llvm/Analysis/LoopInfo.h"
// using llvm::LoopInfoWrapperPass
// using llvm::LoopInfo

#include "llvm/Analysis/TargetLibraryInfo.h"
// using llvm::TargetLibraryInfo
// using llvm::TargetLibraryInfoWrapperPass

#include "llvm/IR/LegacyPassManager.h"
// using llvm::PassManagerBase

#include "llvm/Transforms/IPO/PassManagerBuilder.h"
// using llvm::PassManagerBuilder
// using llvm::RegisterStandardPasses

#include "llvm/Support/raw_ostream.h"
// using llvm::raw_ostream

#include "llvm/Support/CommandLine.h"
// using llvm::cl::opt
// using llvm::cl::desc

#include "llvm/Support/FileSystem.h"
// using llvm::sys::fs::OpenFlags

#include "llvm/Support/Debug.h"
// using DEBUG macro
// using llvm::dbgs

#include <fstream>
// using std::ifstream

#include <vector>
// using std::vector

#include <set>
// using std::set

#include <string>
// using std::string

#include "Config.hpp"

#include "BWList.hpp"

#include "ClassifyLoops.hpp"

#include "ClassifyLoopsPass.hpp"

#define DEBUG_TYPE "classify-loop-exits"

// plugin registration for opt

#define STRINGIFY_UTIL(x) #x
#define STRINGIFY(x) STRINGIFY_UTIL(x)

#define PRJ_CMDLINE_DESC(x)                                                    \
  x " (version: " STRINGIFY(CLASSIFYLOOPS_VERSION) ")"

static llvm::cl::opt<std::string>
    FuncWhileListFilename("classify-loops-fn-whitelist",
                          llvm::cl::desc("function whitelist"));

static llvm::cl::list<std::string>
    IOFuncsFilename("classify-loops-iofuncs", llvm::cl::desc("io funcs list"));

static llvm::cl::list<std::string>
    NLEFuncsFilename("classify-loops-nlefuncs",
                     llvm::cl::desc("non-local exit funcs list"));

static llvm::cl::opt<std::string> ReportStatsFilename(
    "classify-loops-stats",
    llvm::cl::desc("classify loops pass stats report filename"));

namespace icsa {

char ClassifyLoopsPass::ID = 0;
static llvm::RegisterPass<ClassifyLoopsPass>
    tmp2("classify-loops", PRJ_CMDLINE_DESC("classify loop"), false, false);

// plugin registration for clang

// the solution was at the bottom of the header file
// 'llvm/Transforms/IPO/PassManagerBuilder.h'
// create a static free-floating callback that uses the legacy pass manager to
// add an instance of this pass and a static instance of the
// RegisterStandardPasses class

static void registerClassifyLoops(const llvm::PassManagerBuilder &Builder,
                                  llvm::legacy::PassManagerBase &PM) {
  PM.add(new ClassifyLoopsPass());

  return;
}

static llvm::RegisterStandardPasses
    RegisterClassifyLoops(llvm::PassManagerBuilder::EP_EarlyAsPossible,
                          registerClassifyLoops);

//

bool ClassifyLoopsPass::runOnModule(llvm::Module &CurModule) {
  std::set<std::string> IOFuncs;
  std::set<std::string> NonLocalExitFuncs;

  for (const auto &file : IOFuncsFilename) {
    std::ifstream IOFuncsFile{file};
    std::string name;

    if (IOFuncsFile.is_open()) {
      while (IOFuncsFile >> name)
        IOFuncs.insert(name);
    } else
      llvm::errs() << "could open file: \'" << file << "\'\n";
  }

  for (const auto &file : NLEFuncsFilename) {
    std::ifstream NLEFuncsFile{file};
    std::string name;

    if (NLEFuncsFile.is_open()) {
      while (NLEFuncsFile >> name)
        NonLocalExitFuncs.insert(name);
    } else
      llvm::errs() << "could open file: \'" << file << "\'\n";
  }

  BWList funcWhileList;
  if (!FuncWhileListFilename.empty()) {
    std::ifstream funcWhiteListFile{FuncWhileListFilename};

    if (funcWhiteListFile.is_open()) {
      funcWhileList.addRegex(funcWhiteListFile);
      funcWhiteListFile.close();
    } else
      llvm::errs() << "could open file: \'" << FuncWhileListFilename << "\'\n";
  }

  std::vector<LoopStats> totalLoopStats;

  for (auto &curFunc : CurModule) {
    if (!FuncWhileListFilename.empty() &&
        !funcWhileList.matches(curFunc.getName().data()))
      continue;

    if (curFunc.isDeclaration())
      continue;

    const auto *LI =
        &getAnalysis<llvm::LoopInfoWrapperPass>(curFunc).getLoopInfo();
#ifdef HAS_ITERWORK
    const auto *DLP = &getAnalysis<DecoupleLoopsPass>(curFunc);
#endif // HAS_ITERWORK

    const auto &TLI =
        getAnalysis<llvm::TargetLibraryInfoWrapperPass>().getTLI();
    const auto &loopstats = calculate(*LI, &IOFuncs, &NonLocalExitFuncs, &TLI
#ifdef HAS_ITERWORK
                                      ,
                                      DLP
#endif // HAS_ITERWORK
                                      );

    totalLoopStats.insert(totalLoopStats.end(), loopstats.begin(),
                          loopstats.end());
  }

  if (!ReportStatsFilename.empty()) {
    std::error_code err;
    llvm::raw_fd_ostream report(ReportStatsFilename, err,
                                llvm::sys::fs::F_Text);

    if (err)
      llvm::errs() << "could not open file: \"" << ReportStatsFilename
                   << "\" reason: " << err.message() << "\n";
    else {
      for (const auto &ls : totalLoopStats) {
        report << ls.second.LoopId << "\t";
        report << ls.second.ContainingFunc << "\t";
        report << ls.second.NumHeaderExits << "\t";
        report << ls.second.NumNonHeaderExits << "\t";
        report << ls.second.NumInnerLoops << "\t";
        report << ls.second.NumInnerLoopExits << "\t";
        report << ls.second.NumInnerLoopTopLevelExits << "\t";
        report << ls.second.HasIOCalls << "\t";
        report << ls.second.NumNonLocalExits << "\t";
        report << ls.second.NumDiffExitLandings << "\t";
        report << ls.second.HasIteratorSeparableWork << "\t";
        report << "\n";
      }
    }
  }

  return false;
}

void ClassifyLoopsPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.setPreservesAll();
  AU.setPreservesCFG();
  AU.addRequiredTransitive<llvm::LoopInfoWrapperPass>();
#ifdef USE_APPLYIOATTRIBUTE
  AU.addRequired<llvm::TargetLibraryInfoWrapperPass>();
#endif // USE_APPLYIOATTRIBUTE
#ifdef HAS_ITERWORK
  AU.addRequired<DecoupleLoopsPass>();
#endif // HAS_ITERWORK

  return;
}

} // namespace icsa end
