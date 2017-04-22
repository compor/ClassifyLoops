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
} // namespace llvm end

namespace icsa {

class ClassifyLoopExits : public llvm::FunctionPass {
public:
  static char ID;

  ClassifyLoopExits() : llvm::FunctionPass(ID) {}

  bool runOnFunction(llvm::Function &f) override;
  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;

private:
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

} // namespace unnamed end
