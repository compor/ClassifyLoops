//
//
//

#ifndef CLASSIFYLOOPSPASS_HPP
#define CLASSIFYLOOPSPASS_HPP

#include "Config.hpp"

#include "llvm/Pass.h"
// using llvm::ModulePass

namespace llvm {
class Module;
} // namespace llvm end

namespace icsa {

class ClassifyLoopsPass : public llvm::ModulePass {
public:
  static char ID;

  ClassifyLoopsPass() : llvm::ModulePass(ID) {}

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  bool runOnModule(llvm::Module &M) override;
};

} // namespace icsa end

#endif // CLASSIFYLOOPSPASS_HPP
