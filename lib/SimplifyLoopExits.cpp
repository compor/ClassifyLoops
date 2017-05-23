//
//
//

#include "llvm/Pass.h"
// using llvm::RegisterPass

#include "llvm/IR/Type.h"
// using llvm::Type

#include "llvm/IR/Instruction.h"
// using llvm::Instruction

#include "llvm/IR/Function.h"
// using llvm::Function

#include "llvm/Analysis/LoopInfo.h"
// using llvm::LoopInfoWrapperPass
// using llvm::LoopInfo

#include "llvm/IR/Instructions.h"
// using llvm::CallInst

#include "llvm/Support/Casting.h"
// using llvm::dyn_cast

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
#include <set>

#include "SimplifyLoopExits.hpp"

#define DEBUG_TYPE "SimplifyLoopExits"

// plugin registration for opt

#define STRINGIFY_UTIL(x) #x
#define STRINGIFY(x) STRINGIFY_UTIL(x)

#define PRJ_CMDLINE_STRING(x) x " (version: " STRINGIFY(VERSION_STRING) ")"

static llvm::cl::opt<std::string>
    IOFuncsFilename("classify-loops-iofuncs", llvm::cl::desc("io funcs list"));

static llvm::cl::opt<std::string>
    NLEFuncsFilename("classify-loops-nlefuncs",
                     llvm::cl::desc("non-local exit funcs list"));

static llvm::cl::opt<std::string> ReportStatsFilename(
    "classify-loops-stats",
    llvm::cl::desc("classify loops pass stats report filename"));

namespace icsa {

char SimplifyLoopExits::ID = 0;
static llvm::RegisterPass<SimplifyLoopExits>
    tmp1("simplifyloopexits", PRJ_CMDLINE_STRING("simplify loop exits"), false,
         false);

char ClassifyLoopExits::ID = 0;
static llvm::RegisterPass<ClassifyLoopExits>
    tmp2("classifyloopexits", PRJ_CMDLINE_STRING("classify loop exits"), false,
         false);

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

static void registerClassifyLoopExits(const llvm::PassManagerBuilder &Builder,
                                      llvm::legacy::PassManagerBase &PM) {
  PM.add(new ClassifyLoopExits());

  return;
}

static llvm::RegisterStandardPasses
    RegisterClassifyLoopExits(llvm::PassManagerBuilder::EP_EarlyAsPossible,
                              registerClassifyLoopExits);

//

bool ClassifyLoopExits::runOnFunction(llvm::Function &f) {
  if (f.isDeclaration())
    return false;

  m_LI = &getAnalysis<llvm::LoopInfoWrapperPass>().getLoopInfo();
  std::set<std::string> IOFuncs;
  std::set<std::string> NonLocalExitFuncs;

  if (!IOFuncsFilename.empty()) {
    std::ifstream IOFuncsFile{IOFuncsFilename};
    std::string name;

    if (IOFuncsFile.is_open()) {
      while (IOFuncsFile >> name)
        IOFuncs.insert(name);
    } else
      llvm::errs() << "could open file: \'" << IOFuncsFilename << "\'\n";
  }

  if (!NLEFuncsFilename.empty()) {
    std::ifstream NLEFuncsFile{NLEFuncsFilename};
    std::string name;

    if (NLEFuncsFile.is_open()) {
      while (NLEFuncsFile >> name)
        NonLocalExitFuncs.insert(name);
    } else
      llvm::errs() << "could open file: \'" << NLEFuncsFilename << "\'\n";
  }

  const auto &loopstats = calculate(*m_LI, &IOFuncs, &NonLocalExitFuncs);

  if (!ReportStatsFilename.empty()) {
    std::error_code err;
    llvm::raw_fd_ostream report(ReportStatsFilename, err,
                                llvm::sys::fs::F_Text);

    if (err)
      llvm::errs() << "could not open file: \"" << ReportStatsFilename
                   << "\" reason: " << err.message() << "\n";
    else {
      for (const auto &ls : loopstats) {
        report << ls.second.NumHeaderExits << "\t";
        report << ls.second.NumNonHeaderExits << "\t";
        report << ls.second.NumInnerLoops << "\t";
        report << ls.second.NumInnerLoopExits << "\t";
        report << ls.second.NumInnerLoopTopLevelExits << "\t";
        report << ls.second.NumIOCalls << "\t";
        report << ls.second.NumNonLocalExits << "\t";
        report << ls.second.NumDiffExitLandings << "\t";
        report << "\n";
      }
    }
  }

  return false;
}

void ClassifyLoopExits::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.setPreservesAll();
  AU.addRequiredTransitive<llvm::LoopInfoWrapperPass>();

  return;
}

//

bool SimplifyLoopExits::runOnFunction(llvm::Function &f) { return false; }

void SimplifyLoopExits::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.setPreservesAll();

  return;
}

std::vector<LoopStats> calculate(const llvm::LoopInfo &LI,
                                 std::set<std::string> *IOFuncs,
                                 std::set<std::string> *NonLocalExitFuncs) {
  std::vector<LoopStats> stats{};

  for (const auto &L : LI) {
    if (L->getLoopDepth() > 1)
      continue;

    LoopStatsData sd{};

    llvm::SmallVector<llvm::BasicBlock *, 5> exiting;
    L->getExitingBlocks(exiting);

    for (const auto &e : exiting)
      if (LI.isLoopHeader(e))
        sd.NumHeaderExits++;

    sd.NumNonHeaderExits = exiting.size() - sd.NumHeaderExits;

    sd.NumInnerLoops = L->getLoopDepth() - 1;

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
      llvm::SmallVector<llvm::BasicBlock *, 5> innerExiting;
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

        if (!calledfunc)
          continue;

        if (!IOFuncs)
          continue;

        const auto foundIO = IOFuncs->find(calledfunc->getName().str());
        if (std::end(*IOFuncs) != foundIO)
          sd.NumIOCalls++;

        if (!NonLocalExitFuncs)
          continue;

        const auto foundNLE =
            NonLocalExitFuncs->find(calledfunc->getName().str());
        if (std::end(*NonLocalExitFuncs) != foundNLE)
          sd.NumNonLocalExits++;
      }

    stats.emplace_back(L, sd);
  }

  return stats;
}

} // namespace icsa end
