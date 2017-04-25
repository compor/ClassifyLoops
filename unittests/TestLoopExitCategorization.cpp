
#include <memory>
// using std::unique_ptr

#include <cassert>
// using assert

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
namespace {

class TestClassifyLoopExits : public testing::Test {
public:
  TestClassifyLoopExits() : m_Module{nullptr} {}

  void ParseAssembly(const char *Assembly) {
    llvm::SMDiagnostic err;

    m_Module =
        llvm::parseAssemblyString(Assembly, err, llvm::getGlobalContext());

    std::string errMsg;
    llvm::raw_string_ostream os(errMsg);
    err.print("", os);

    if (!m_Module)
      llvm::report_fatal_error(os.str().c_str());

    auto *Func = m_Module->getFunction("test");
    if (!Func)
      llvm::report_fatal_error("Test must have a function named @test");

    return;
  }

  void ExpectTestPass() {
    static char ID;

    struct UtilityPass : public llvm::FunctionPass {
      UtilityPass() : llvm::FunctionPass(ID) {}

      static int initialize() {
        auto *registry = llvm::PassRegistry::getPassRegistry();

        auto *PI = new llvm::PassInfo("Utility pass for unit tests", "", &ID,
                                      nullptr, true, true);

        registry->registerPass(*PI, false);
        llvm::initializeLoopInfoWrapperPassPass(*registry);

        return 0;
      }

      void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
        AU.setPreservesAll();
        AU.addRequiredTransitive<llvm::LoopInfoWrapperPass>();

        return;
      }

      bool runOnFunction(llvm::Function &F) override {
        if (!F.hasName() || !F.getName().startswith("test"))
          return false;

        auto &LI = getAnalysis<llvm::LoopInfoWrapperPass>().getLoopInfo();
        //LI.print(llvm::outs());

        auto &CurLoop = *LI.begin();
        assert(CurLoop && "Loop ptr is invalid");

        auto rv = LoopExitStats::getExits(*CurLoop);
        EXPECT_EQ(1, rv);

        return false;
      }
    };

    static int init = UtilityPass::initialize();
    (void)init; // do not optimize away

    auto *P = new UtilityPass();
    llvm::legacy::PassManager PM;

    PM.add(P);
    PM.run(*m_Module);

    return;
  }

protected:
  std::unique_ptr<llvm::Module> m_Module;
};

TEST_F(TestClassifyLoopExits, ReturnsSingleExitForRegularLoop) {
  ParseAssembly("define void @test() {\n"
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

  ExpectTestPass();
}

} // namespace anonymous end
} // namespace icsa end
