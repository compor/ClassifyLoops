
#include <memory>
// using std::unique_ptr

#include "llvm/IR/LLVMContext.h"
// using llvm::LLVMContext

#include "llvm/IR/Module.h"
// using llvm::Module

#include "llvm/IR/LegacyPassManager.h"
// using llvm::legacy::PassMananger

#include "llvm/PassRegistry.h"
// using llvm::PassRegistry

#include "llvm/Pass.h"
// using llvm::Pass

#include "llvm/Analysis/LoopInfo.h"
// using llvm::LoopInfoWrapperPass

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
public:
  LoopExitClassifierTest() : m_FPM(nullptr) {
    llvm::initializeLoopInfoWrapperPassPass(
        *llvm::PassRegistry::getPassRegistry());

    m_LP = std::make_unique<llvm::LoopInfoWrapperPass>();
    m_FPM.add(m_LP.get());
  }

protected:
  void ParseAssembly(const char *Assembly) {
    llvm::SMDiagnostic Error;
    m_Module = llvm::parseAssemblyString(Assembly, Error, llvm::getGlobalContext());

    std::string errMsg;
    llvm::raw_string_ostream os(errMsg);
    Error.print("", os);

    if (!m_Module)
      llvm::report_fatal_error(os.str().c_str());

    llvm::Function *F = m_Module->getFunction("foo");
    if (!F)
      llvm::report_fatal_error("Test must have a function named @foo");

    return;
  }

  std::unique_ptr<llvm::Module> m_Module;
  std::unique_ptr<llvm::LoopInfoWrapperPass> m_LP;
  llvm::legacy::FunctionPassManager m_FPM;
};

} // namespace unnamed end

int main(int argc, char *argv[]) {
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
