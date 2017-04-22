
#include <memory>
// using std::unique_ptr

#include "llvm/IR/LLVMContext.h"
// using llvm::LLVMContext

#include "llvm/IR/Module.h"
// using llvm::Module

#include "llvm/IR/LegacyPassManager.h"
// using llvm::legacy::PassMananger

#include "llvm/Pass.h"
// using llvm::Pass
// using llvm::PassInfo

#include "llvm/Analysis/LoopInfo.h"
// using llvm::LoopInfoWrapperPass
// using llvm::LoopInfo

#include "llvm/Support/SourceMgr.h"
// using llvm::SMDiagnostic

#include "llvm/AsmParser/Parser.h"
// using llvm::parseAssemblyString

#include "llvm/Support/ErrorHandling.h"
// using llvm::report_fatal_error

#include "llvm/Support/raw_ostream.h"
// using llvm::raw_string_ostream

#include "gtest/gtest.h"
// using testing::Test

#include "SimplifyLoopExits.hpp"

namespace icsa {

class TestClassifyLoopExits : public testing::Test {
public:
  TestClassifyLoopExits()
      : m_Module{nullptr}, m_Func{nullptr}, CLE{*new ClassifyLoopExits()} {}

  void ParseAssembly(const char *Assembly) {
    llvm::SMDiagnostic err;

    m_Module =
        llvm::parseAssemblyString(Assembly, err, llvm::getGlobalContext());

    std::string errMsg;
    llvm::raw_string_ostream os(errMsg);
    err.print("", os);

    if (!m_Module)
      llvm::report_fatal_error(os.str().c_str());

    m_Func = m_Module->getFunction("test");
    if (!m_Func)
      llvm::report_fatal_error("Test must have a function named @test");

    m_PM.add(&CLE);
    m_PM.run(*m_Module);

    return;
  }

protected:
  llvm::legacy::PassManager m_PM;
  std::unique_ptr<llvm::Module> m_Module;
  llvm::Function *m_Func;
  ClassifyLoopExits &CLE;
};

TEST_F(TestClassifyLoopExits, DISABLED_Dummy) {
  ParseAssembly("define void @foo() {\n"
                "entry:\n"
                "  bitcast i8 undef to i8\n"
                "  %B = bitcast i8 undef to i8\n"
                "  bitcast i8 undef to i8\n"
                "  bitcast i8 undef to i8\n"
                "  %A = bitcast i8 undef to i8\n"
                "  ret void\n"
                "}\n");

  EXPECT_EQ(true, true);
}

TEST_F(TestClassifyLoopExits, ReturnsZeroLoopExitsWhenNoLoops) {
  ParseAssembly("define void @foo() {\n"
                "entry:\n"
                "  bitcast i8 undef to i8\n"
                "  %B = bitcast i8 undef to i8\n"
                "  bitcast i8 undef to i8\n"
                "  bitcast i8 undef to i8\n"
                "  %A = bitcast i8 undef to i8\n"
                "  ret void\n"
                "}\n");
}

TEST_F(TestClassifyLoopExits, ReturnsSingleExitForRegularLoop) {
  ParseAssembly("define void @foo() {\n"
                "%i = alloca i32, align 4\n"
                "%a = alloca i32, align 4\n"
                "store i32 100, i32* %i, align 4\n"
                "store i32 0, i32* %a, align 4\n"
                "br label %1\n"

                "%2 = load i32, i32* %i, align 4\n"
                "%3 = add nsw i32 %2, -1\n"
                "store i32 %3, i32* %i, align 4\n"
                "%4 = icmp ne i32 %3, 0\n"
                "br i1 %4, label %5, label %8\n"

                "%6 = load i32, i32* %a, align 4\n"
                "%7 = add nsw i32 %6, 1\n"
                "store i32 %7, i32* %a, align 4\n"
                "br label %1\n"

                "ret void\n"
                "}\n");
}

int main(int argc, char *argv[]) {
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}

} // namespace unnamed end
