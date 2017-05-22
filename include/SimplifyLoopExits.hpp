//
//
//

#include "llvm/Pass.h"
// using llvm::FunctionPass

#include <utility>
#include <vector>

#ifndef VERSION_STRING
#define VERSION_STRING ""
#endif

namespace llvm {
class Function;
class AnalysisUsage;
class LoopInfo;
class Loop;
} // namespace llvm end

namespace icsa {

struct LoopStatsData {
  LoopStatsData()
      : NumHeaderExits(0), NumNonHeaderExits(0), NumPDomBlockExits(0),
        NumDomBlockExits(0), NumInnerLoops(0), NumInnerLoopExits(0),
        NumIOCalls(0), NumNonLocalExits(0), NumDiffExitLandings(0) {}

  unsigned NumHeaderExits;
  unsigned NumNonHeaderExits;
  unsigned NumPDomBlockExits;
  unsigned NumDomBlockExits;
  unsigned NumInnerLoops;
  unsigned NumInnerLoopExits;
  unsigned NumIOCalls;
  unsigned NumNonLocalExits;
  unsigned NumDiffExitLandings;
};

using LoopStats = std::pair<const llvm::Loop *, LoopStatsData>;

std::vector<LoopStats> calculate(const llvm::LoopInfo &LI);

struct ClassifyLoopExits : public llvm::FunctionPass {

  static char ID;

  ClassifyLoopExits() : llvm::FunctionPass(ID) {}

  bool runOnFunction(llvm::Function &f) override;
  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;

  llvm::LoopInfo *m_LI;
};

//

class SimplifyLoopExits : public llvm::FunctionPass {
public:
  static char ID;

  SimplifyLoopExits() : llvm::FunctionPass(ID) {}

  bool runOnFunction(llvm::Function &f) override;
  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
};

} // namespace icsa end
