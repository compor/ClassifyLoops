//
//
//

#include "llvm/Pass.h"
// using llvm::FunctionPass

#include <string>
#include <utility>
#include <set>
#include <vector>

#ifdef HAS_ITERWORK
#include "DecoupleLoops.h"
using icsa::DecoupleLoopsPass;
#endif // HAS_ITERWORK

namespace llvm {
class Function;
class AnalysisUsage;
class LoopInfo;
class Loop;
} // namespace llvm end

namespace icsa {

struct LoopStatsData {
  LoopStatsData()
      : ContainingFunc{"-"}, NumHeaderExits(0), NumNonHeaderExits(0),
        NumInnerLoops(0), NumInnerLoopExits(0), NumInnerLoopTopLevelExits(0),
        NumIOCalls(0), NumNonLocalExits(0), NumDiffExitLandings(0),
        HasIteratorSeparableWork(0) {}

  std::string ContainingFunc;
  unsigned NumHeaderExits;
  unsigned NumNonHeaderExits;
  unsigned NumInnerLoops;
  unsigned NumInnerLoopExits;
  unsigned NumInnerLoopTopLevelExits;
  unsigned NumIOCalls;
  unsigned NumNonLocalExits;
  unsigned NumDiffExitLandings;
  unsigned HasIteratorSeparableWork;
};

using LoopStats = std::pair<const llvm::Loop *, LoopStatsData>;

std::vector<LoopStats>
calculate(const llvm::LoopInfo &LI,
          const std::set<std::string> *IOFuncs = nullptr,
          const std::set<std::string> *NonLocalExitFuncs = nullptr,
          const llvm::TargetLibraryInfo *TLI = nullptr
#ifdef HAS_ITERWORK
          ,
          const DecoupleLoopsPass *DLP = nullptr
#endif // HAS_ITERWORK
          );

struct ClassifyLoops : public llvm::ModulePass {

  static char ID;

  ClassifyLoops() : llvm::ModulePass(ID) {}

  bool runOnModule(llvm::Module &CurModule) override;
  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;

  // llvm::LoopInfo *m_LI;
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
