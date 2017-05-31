//
//
//

#include "llvm/Pass.h"
// using llvm::RegisterPass

#include "llvm/IR/Type.h"
// using llvm::Type

#include "llvm/IR/Instruction.h"
// using llvm::Instruction

#include "llvm/IR/Module.h"
// using llvm::Module

#include "llvm/IR/Function.h"
// using llvm::Function

#include "llvm/Analysis/LoopInfo.h"
// using llvm::LoopInfoWrapperPass
// using llvm::LoopInfo

#include "llvm/IR/Instructions.h"
// using llvm::CallInst

#include "llvm/Support/Casting.h"
// using llvm::dyn_cast

#include "llvm/Analysis/TargetLibraryInfo.h"
// using llvm::TargetLibraryInfo
// using llvm::TargetLibraryInfoWrapperPass

#include "llvm/IR/LegacyPassManager.h"
// using llvm::PassManagerBase

#include "llvm/Transforms/IPO/PassManagerBuilder.h"
// using llvm::PassManagerBuilder
// using llvm::RegisterStandardPasses

#include "llvm/ADT/SmallVector.h"

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

#include <set>
// using std::set

#include "Config.hpp"

#include "BWList.hpp"

#include "SimplifyLoopExits.hpp"

#ifdef USE_APPLYIOATTRIBUTE
#include "ApplyIOAttribute.hpp"
#endif // USE_APPLYIOATTRIBUTE

#ifdef USE_ANNOTATELOOPS
#include "AnnotateLoops.hpp"
#endif // USE_ANNOTATELOOPS

#define DEBUG_TYPE "simplify-loop-exits"

// plugin registration for opt

#define STRINGIFY_UTIL(x) #x
#define STRINGIFY(x) STRINGIFY_UTIL(x)

#define PRJ_CMDLINE_DESC(x)                                                    \
  x " (version: " STRINGIFY(SIMPLIFYLOOPEXITS_VERSION) ")"

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

char SimplifyLoopExits::ID = 0;
static llvm::RegisterPass<SimplifyLoopExits>
    tmp1("simplifyloopexits", PRJ_CMDLINE_DESC("simplify loop exits"), false,
         false);

char ClassifyLoops::ID = 0;
static llvm::RegisterPass<ClassifyLoops>
    tmp2("classify-loops", PRJ_CMDLINE_DESC("classify loop"), false, false);

// plugin registration for clang

// the solution was at the bottom of the header file
// 'llvm/Transforms/IPO/PassManagerBuilder.h'
// create a static free-floating callback that uses the legacy pass manager to
// add an instance of this pass and a static instance of the
// RegisterStandardPasses class

static void registerSimplifyLoopExits(const llvm::PassManagerBuilder &Builder,
                                      llvm::legacy::PassManagerBase &PM) {
  PM.add(new SimplifyLoopExits());

  return;
}

static llvm::RegisterStandardPasses
    RegisterSimplifyLoopExits(llvm::PassManagerBuilder::EP_EarlyAsPossible,
                              registerSimplifyLoopExits);

//

static void registerClassifyLoops(const llvm::PassManagerBuilder &Builder,
                                  llvm::legacy::PassManagerBase &PM) {
  PM.add(new ClassifyLoops());

  return;
}

static llvm::RegisterStandardPasses
    RegisterClassifyLoops(llvm::PassManagerBuilder::EP_EarlyAsPossible,
                          registerClassifyLoops);

//

bool ClassifyLoops::runOnModule(llvm::Module &CurModule) {
  std::set<std::string> IOFuncs;
  std::set<std::string> NonLocalExitFuncs;

  // if (!IOFuncsFilename.empty()) {
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

void ClassifyLoops::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
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

//

bool SimplifyLoopExits::runOnFunction(llvm::Function &f) { return false; }

void SimplifyLoopExits::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.setPreservesAll();

  return;
}

std::vector<LoopStats> calculate(const llvm::LoopInfo &LI,
                                 const std::set<std::string> *IOFuncs,
                                 const std::set<std::string> *NonLocalExitFuncs,
                                 const llvm::TargetLibraryInfo *TLI
#ifdef HAS_ITERWORK
                                 ,
                                 const DecoupleLoopsPass *DLP
#endif // HAS_ITERWORK
                                 ) {
  std::vector<LoopStats> stats{};

#ifdef USE_ANNOTATELOOPS
  AnnotateLoops queryAnnotator;
#endif // USE_ANNOTATELOOPS

  for (const auto &L : LI) {
    if (L->getLoopDepth() > 1)
      continue;

    LoopStatsData sd{};

#ifdef USE_ANNOTATELOOPS
    if (queryAnnotator.hasAnnotatedId(*L))
      sd.LoopId = queryAnnotator.getAnnotatedId(*L);
#endif // USE_ANNOTATELOOPS

    const auto *containingFunc = L->getHeader()->getParent();
    if (containingFunc->hasName())
      sd.ContainingFunc = containingFunc->getName();

    llvm::SmallVector<llvm::BasicBlock *, 5> exiting;
    L->getExitingBlocks(exiting);

    for (const auto &e : exiting)
      if (L->getHeader() == e)
        sd.NumHeaderExits++;

    sd.NumNonHeaderExits = exiting.size() - sd.NumHeaderExits;

    llvm::SmallVector<llvm::BasicBlock *, 5> exitLandings;
    L->getExitBlocks(exitLandings);
    std::set<llvm::BasicBlock *> uniqueExitLandings;
    for (const auto &e : exitLandings)
      uniqueExitLandings.insert(e);

    sd.NumDiffExitLandings = uniqueExitLandings.size();

    const auto subloops = L->getSubLoops();
    sd.NumInnerLoops = subloops.size();

    std::set<llvm::BasicBlock *> uniqueInnerExiting;
    for (const auto &innerloop : subloops) {
      llvm::SmallVector<llvm::BasicBlock *, 6> innerExiting;
      innerloop->getExitingBlocks(innerExiting);
      for (const auto &e : innerExiting)
        uniqueInnerExiting.insert(e);
    }

    sd.NumInnerLoopExits = uniqueInnerExiting.size();

    for (const auto &e : uniqueInnerExiting)
      for (auto i = 0; i < e->getTerminator()->getNumSuccessors(); ++i)
        if (!L->contains(e->getTerminator()->getSuccessor(i)))
          sd.NumInnerLoopTopLevelExits++;

    for (const auto *bb : L->getBlocks())
      for (const auto &inst : *bb) {
        const auto *callinst = llvm::dyn_cast<llvm::CallInst>(&inst);

        if (!callinst)
          continue;

        const auto calledfunc = callinst->getCalledFunction();

// TODO convert count to simple bool and exit early if true
#ifdef USE_APPLYIOATTRIBUTE
        if (TLI) {
          ApplyIOAttribute aioattr(*TLI);
          sd.HasIOCalls |= aioattr.hasIO(*L);
        }
#endif // USE_APPLYIOATTRIBUTE

        if (!calledfunc)
          continue;

        if (!IOFuncs)
          continue;

        const auto foundIO = IOFuncs->find(calledfunc->getName().str());
        if (std::end(*IOFuncs) != foundIO)
          sd.HasIOCalls |= true;

        if (!NonLocalExitFuncs)
          continue;

        const auto foundNLE =
            NonLocalExitFuncs->find(calledfunc->getName().str());
        if (std::end(*NonLocalExitFuncs) != foundNLE)
          sd.NumNonLocalExits++;
      }

#ifdef HAS_ITERWORK
    if (DLP)
      sd.HasIteratorSeparableWork = DLP->hasWork(L);
#endif // HAS_ITERWORK

    stats.emplace_back(L, sd);
  }

  return stats;
}

} // namespace icsa end
