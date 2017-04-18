//
//
//

#include "llvm/Pass.h"
// using llvm::FunctionPass

#ifndef PRJ_VERSION
#define PRJ_VERSION "development"
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
