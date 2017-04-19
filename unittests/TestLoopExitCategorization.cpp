
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

class TestLoopExitClassifier : public testing::Test {
public:
  std::unique_ptr<llvm::legacy::FunctionPassManager> m_FPM;
  llvm::LoopInfoWrapperPass *m_LP;

  std::unique_ptr<llvm::Module> m_Module;
  llvm::Function *m_Func;

  llvm::LoopInfo *m_LI;

  void ParseAssembly(const char *Assembly) {
    llvm::SMDiagnostic err;

    m_Module =
        llvm::parseAssemblyString(Assembly, err, llvm::getGlobalContext());

    std::string errMsg;
    llvm::raw_string_ostream os(errMsg);
    err.print("", os);

    if (!m_Module)
      llvm::report_fatal_error(os.str().c_str());

    m_Func = m_Module->getFunction("foo");
    if (!m_Func)
      llvm::report_fatal_error("Test must have a function named @foo");

    m_FPM = std::make_unique<llvm::legacy::FunctionPassManager>(m_Module.get());

    m_FPM->run(*m_Func);
    m_LI = &m_LP->getLoopInfo();

    return;
  }

  TestLoopExitClassifier()
      : m_FPM{nullptr}, m_LP{new llvm::LoopInfoWrapperPass()},
        m_Module{nullptr}, m_Func{nullptr} {}
};

} // namespace unnamed end

TEST_F(TestLoopExitClassifier, Foo) {
  ParseAssembly("define void @foo() {\n"
                "entry:\n"
                "  bitcast i8 undef to i8\n"
                "  %B = bitcast i8 undef to i8\n"
                "  bitcast i8 undef to i8\n"
                "  bitcast i8 undef to i8\n"
                "  %A = bitcast i8 undef to i8\n"
                "  ret void\n"
                "}\n");

  EXPECT_EQ(m_LI->empty(), true);
}

int main(int argc, char *argv[]) {
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
