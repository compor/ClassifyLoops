//
//
//

#ifndef SIMPLIFYLOOPEXITSPASS_HPP
#define SIMPLIFYLOOPEXITSPASS_HPP

#include "llvm/Pass.h"
// using llvm::FunctionPass

namespace llvm {
class Function;
class AnalysisUsage;
} // namespace llvm end

namespace icsa {

struct ClassifyLoopsPass : public llvm::ModulePass {
  static char ID;

  ClassifyLoopsPass() : llvm::ModulePass(ID) {}

  bool runOnModule(llvm::Module &CurModule) override;
  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
};

//

class SimplifyLoopExitsPass : public llvm::FunctionPass {
public:
  static char ID;

  SimplifyLoopExitsPass() : llvm::FunctionPass(ID) {}

  bool runOnFunction(llvm::Function &f) override;
  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
};

} // namespace icsa end

#endif // SIMPLIFYLOOPEXITSPASS_HPP
