
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



namespace {

struct UtilityPass : public llvm::FunctionPass {
  static char ID;
  llvm::LoopInfo *m_LI;



  UtilityPass() : FunctionPass(ID) {}

  bool runOnFunction(llvm::Function &F) override {
    m_LI = &getAnalysis<llvm::LoopInfoWrapperPass>().getLoopInfo();
    // m_LI->print(llvm::outs());

    return false;
  }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesAll();
    AU.addRequired<llvm::LoopInfoWrapperPass>();
  }

  static int initialize() {
    llvm::PassInfo *PI =
        new llvm::PassInfo("Utility pass", "", &ID, nullptr, true, true);

    auto *registry = llvm::PassRegistry::getPassRegistry();
    registry->registerPass(*PI, false);

    llvm::initializeLoopInfoWrapperPassPass(*registry);

    return 0;
  }
};

char UtilityPass::ID = 0;

class TestLoopExitClassifier : public testing::Test {
public:
  std::unique_ptr<llvm::legacy::PassManager> m_PM;
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

    m_PM = std::make_unique<llvm::legacy::PassManager>();

    static int init = UtilityPass::initialize();
    (void)init;

    auto *P = new UtilityPass();
    m_PM->add(P);
    m_PM->run(*m_Module);

    return;
  }

  TestLoopExitClassifier()
      : m_PM{nullptr}, m_LI{nullptr}, m_Module{nullptr}, m_Func{nullptr} {}
};

} // namespace unnamed end

namespace LoopExitClassifier {
long int getLoopExitNumber(const llvm::Loop &CurLoop) {
  llvm::SmallVector<llvm::BasicBlock *, 5> exiting;
  CurLoop.getExitingBlocks(exiting);

  return -1;
}
}

TEST_F(TestLoopExitClassifier, DISABLED_Dummy) {
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

TEST_F(TestLoopExitClassifier, ReturnsZeroLoopExitsWhenNoLoops) {
  ParseAssembly("define void @foo() {\n"
                "entry:\n"
                "  bitcast i8 undef to i8\n"
                "  %B = bitcast i8 undef to i8\n"
                "  bitcast i8 undef to i8\n"
                "  bitcast i8 undef to i8\n"
                "  %A = bitcast i8 undef to i8\n"
                "  ret void\n"
                "}\n");

  EXPECT_EQ(LoopExitClassifier::getLoopExitNumber(*m_LI), 0);
}

int main(int argc, char *argv[]) {
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
