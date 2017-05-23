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

#include "llvm/Support/Debug.h"
// using DEBUG macro
// using llvm::dbgs

#include <set>

#include "SimplifyLoopExits.hpp"

#define DEBUG_TYPE "SimplifyLoopExits"

// plugin registration for opt

#define STRINGIFY_UTIL(x) #x
#define STRINGIFY(x) STRINGIFY_UTIL(x)

#define PRJ_CMDLINE_STRING(x) x " (version: " STRINGIFY(VERSION_STRING) ")"

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
  m_LI = &getAnalysis<llvm::LoopInfoWrapperPass>().getLoopInfo();

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

std::vector<LoopStats> calculate(const llvm::LoopInfo &LI) {
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

    stats.emplace_back(L, sd);
  }

  return stats;
}

} // namespace icsa end
