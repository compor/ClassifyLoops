
#include <memory>
// using std::unique_ptr

#include "llvm/IR/LLVMContext.h"
// using llvm::LLVMContext

#include "llvm/IR/Module.h"
// using llvm::Module

#include "llvm/Support/SourceMgr.h"
// using llvm::SMDiagnostic

#include "llvm/AsmParser/Parser.h"
// using llvm::parseAssemblyString

#include "llvm/Support/ErrorHandling.h"
// using llvm::report_fatal_error

#include "llvm/Support/raw_ostream.h"
// using llvm::raw_string_ostream

#include "gtest/gtest.h"

namespace {

class LoopExitClassifierTest : public testing::Test {
protected:
  void ParseAssembly(const char *Assembly) {
    llvm::SMDiagnostic Error;
    M = llvm::parseAssemblyString(Assembly, Error, llvm::getGlobalContext());

    std::string errMsg;
    llvm::raw_string_ostream os(errMsg);
    Error.print("", os);

    if (!M)
      llvm::report_fatal_error(os.str().c_str());

    llvm::Function *F = M->getFunction("foo");
    if (!F)
      llvm::report_fatal_error("Test must have a function named @foo");

    return;
  }

  std::unique_ptr<llvm::Module> M;
};

} // namespace unnamed end

int main(int argc, char *argv[]) {
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
