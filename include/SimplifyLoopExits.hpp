//
//
//

#include "llvm/Pass.h"
// using llvm::FunctionPass

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

struct LoopExitStats {
  static unsigned int getNonHeaderExits(const llvm::Loop &L);
};

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
