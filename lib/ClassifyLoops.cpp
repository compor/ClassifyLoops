//
//
//

#include "llvm/IR/Type.h"
// using llvm::Type

#include "llvm/IR/Instruction.h"
// using llvm::Instruction

#include "llvm/Analysis/LoopInfo.h"
// using llvm::LoopInfoWrapperPass
// using llvm::LoopInfo

#include "llvm/IR/Instructions.h"
// using llvm::CallInst

#include "llvm/Support/Casting.h"
// using llvm::dyn_cast

#include "llvm/Analysis/TargetLibraryInfo.h"
// using llvm::TargetLibraryInfo

#include "llvm/ADT/SmallVector.h"
// using llvm::SmallVector

#ifdef USE_APPLYIOATTRIBUTE
#include "ApplyIOAttribute.hpp"
#endif // USE_APPLYIOATTRIBUTE

#ifdef USE_ANNOTATELOOPS
#include "AnnotateLoops.hpp"
#endif // USE_ANNOTATELOOPS

#include "ClassifyLoops.hpp"

namespace icsa {

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
