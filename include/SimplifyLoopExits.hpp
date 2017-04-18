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
} // namespace llvm end

namespace {

class SimplifyLoopExits : public llvm::FunctionPass {
public:
  static char ID;

  SimplifyLoopExits() : llvm::FunctionPass(ID) {}

  bool runOnFunction(llvm::Function &f) override;
};

} // namespace unnamed end
