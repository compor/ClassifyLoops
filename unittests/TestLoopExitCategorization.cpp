
#include <memory>
// using std::unique_ptr

#include <map>
// using std::map

#include <cassert>
// using assert

#include <cstdlib>
// using std::abort

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

#include "llvm/IR/Verifier.h"
// using llvm::verifyModule

#include "llvm/Support/ErrorHandling.h"
// using llvm::report_fatal_error

#include "llvm/Support/raw_ostream.h"
// using llvm::raw_string_ostream

#include "gtest/gtest.h"
// using testing::Test

#include "boost/variant.hpp"
// using boost::variant

#include "SimplifyLoopExits.hpp"

namespace icsa {
namespace {

using test_result_t = boost::variant<bool, unsigned int>;
using test_result_map = std::map<std::string, test_result_t>;

struct test_result_visitor : public boost::static_visitor<unsigned int> {
  unsigned int operator()(bool b) const { return b ? 1 : 0; }
  unsigned int operator()(unsigned int i) const { return i; }
};

class TestClassifyLoopExits : public testing::Test {
public:
  enum struct AssemblyHolderType : int { FILE_TYPE, STRING_TYPE };

  TestClassifyLoopExits()
      : m_Module{nullptr}, m_TestDataDir{"./unittests/data/"} {}

  void ParseAssembly(
      const char *AssemblyHolder,
      const AssemblyHolderType asmHolder = AssemblyHolderType::FILE_TYPE) {
    llvm::SMDiagnostic err;

    if (AssemblyHolderType::FILE_TYPE == asmHolder) {
      std::string fullFilename{m_TestDataDir};
      fullFilename += AssemblyHolder;

    m_Module =
          llvm::parseAssemblyFile(fullFilename, err, llvm::getGlobalContext());

    } else {
      m_Module = llvm::parseAssemblyString(AssemblyHolder, err,
                                           llvm::getGlobalContext());
    }

    std::string errMsg;
    llvm::raw_string_ostream os(errMsg);
    err.print("", os);

    if (llvm::verifyModule(*m_Module, &(llvm::errs())))
      llvm::report_fatal_error("module verification failed\n");

    if (!m_Module)
      llvm::report_fatal_error(os.str().c_str());

    return;
  }

  void ExpectTestPass(const test_result_map &trm) {
    static char ID;

    struct UtilityPass : public llvm::FunctionPass {
      UtilityPass(const test_result_map &trm)
          : llvm::FunctionPass(ID), m_trm(trm) {}

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
        // LI.print(llvm::outs());

        auto &CurLoop = *LI.begin();
        assert(CurLoop && "Loop ptr is invalid");

        test_result_map::const_iterator found;

        // subcase
        found = lookup("number of exits");

        const auto &rv = LoopExitStats::getExits(*CurLoop);
        const auto &ev =
            boost::apply_visitor(test_result_visitor(), found->second);
        EXPECT_EQ(ev, rv) << found->first;

        return false;
      }

      test_result_map::const_iterator lookup(const std::string &subcase) {
        auto found = m_trm.find(subcase);
        if (m_trm.end() == found) {
          llvm::errs() << "subcase: " << subcase << " test data not found\n";
          std::abort();
        }

        return found;
      }

      const test_result_map &m_trm;
    };

    static int init = UtilityPass::initialize();
    (void)init; // do not optimize away

    auto *P = new UtilityPass(trm);
    llvm::legacy::PassManager PM;

    PM.add(P);
    PM.run(*m_Module);

    return;
  }

protected:
  std::unique_ptr<llvm::Module> m_Module;
  const char *m_TestDataDir;
};

TEST_F(TestClassifyLoopExits, RegularLoopExits) {
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
                "}\n", AssemblyHolderType::STRING_TYPE);

  test_result_map trm;

  trm.insert({"number of exits", 1});
  ExpectTestPass(trm);
}

TEST_F(TestClassifyLoopExits, DefiniteInfiniteLoopExits) {
  ParseAssembly("define void @test() {\n"
                "%a = alloca i32, align 4\n"
                "store i32 0, i32* %a, align 4\n"
                "br label %1\n"

                "%2 = load i32, i32* %a, align 4\n"
                "%3 = add nsw i32 %2, 1\n"
                "store i32 %3, i32* %a, align 4\n"
                "br label %1\n"

                "ret void\n"
                "}\n", AssemblyHolderType::STRING_TYPE);

  test_result_map trm;

  trm.insert({"number of exits", 0});
  ExpectTestPass(trm);
}

TEST_F(TestClassifyLoopExits, DeadLoopExits) {
  ParseAssembly("define void @test() {\n"
                "%a = alloca i32, align 4\n"
                "store i32 0, i32* %a, align 4\n"
                "br label %1\n"
                "br i1 false, label %2, label %5\n"
                "%3 = load i32, i32* %a, align 4\n"
                "%4 = add nsw i32 %3, 1\n"
                "store i32 %4, i32* %a, align 4\n"
                "br label %1\n"
                "ret void\n"
                "}\n", AssemblyHolderType::STRING_TYPE);

  test_result_map trm;

  trm.insert({"number of exits", 1});
  ExpectTestPass(trm);
}

TEST_F(TestClassifyLoopExits, BreakConditionLoopExits) {
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
                "br i1 %4, label %5, label %.loopexit\n"

                "%6 = load i32, i32* %a, align 4\n"
                "%7 = add nsw i32 %6, 1\n"
                "store i32 %7, i32* %a, align 4\n"
                "%8 = load i32, i32* %a, align 4\n"
                "%9 = icmp eq i32 %8, 50\n"
                "br i1 %9, label %10, label %11\n"

                "br label %14\n"

                "%12 = load i32, i32* %a, align 4\n"
                "%13 = add nsw i32 %12, 1\n"
                "store i32 %13, i32* %a, align 4\n"
                "br label %1\n"

                ".loopexit:   \n"
                "br label %14\n"

                "ret void\n"
                "}\n", AssemblyHolderType::STRING_TYPE);

  test_result_map trm;

  trm.insert({"number of exits", 2});
  ExpectTestPass(trm);
}

TEST_F(TestClassifyLoopExits, ContinueConditionLoopExits) {
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
                "br i1 %4, label %5, label %14\n"

                "%6 = load i32, i32* %a, align 4\n"
                "%7 = add nsw i32 %6, 1\n"
                "store i32 %7, i32* %a, align 4\n"
                "%8 = load i32, i32* %a, align 4\n"
                "%9 = icmp eq i32 %8, 50\n"
                "br i1 %9, label %10, label %11\n"

                "br label %.backedge\n"

                ".backedge:\n"
                "br label %1\n"

                "%12 = load i32, i32* %a, align 4\n"
                "%13 = add nsw i32 %12, 1\n"
                "store i32 %13, i32* %a, align 4\n"
                "br label %.backedge\n"

                "ret void\n"
                "}\n", AssemblyHolderType::STRING_TYPE);

  test_result_map trm;

  trm.insert({"number of exits", 1});
  ExpectTestPass(trm);
}

TEST_F(TestClassifyLoopExits, ReturnStmtLoopExits) {
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
                "br i1 %4, label %5, label %.loopexit\n"

                "%6 = load i32, i32* %a, align 4\n"
                "%7 = add nsw i32 %6, 1\n"
                "store i32 %7, i32* %a, align 4\n"
                "%8 = load i32, i32* %a, align 4\n"
                "%9 = icmp eq i32 %8, 50\n"
                "br i1 %9, label %10, label %11\n"

                "br label %14\n"

                "%12 = load i32, i32* %a, align 4\n"
                "%13 = add nsw i32 %12, 1\n"
                "store i32 %13, i32* %a, align 4\n"
                "br label %1\n"

                ".loopexit:\n"
                "br label %14\n"

                "ret void\n"
                "}\n", AssemblyHolderType::STRING_TYPE);

  test_result_map trm;

  trm.insert({"number of exits", 2});
  ExpectTestPass(trm);
}

TEST_F(TestClassifyLoopExits, ExitCallLoopExits) {
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
                "br i1 %4, label %5, label %14\n"

                "%6 = load i32, i32* %a, align 4\n"
                "%7 = add nsw i32 %6, 1\n"
                "store i32 %7, i32* %a, align 4\n"
                "%8 = load i32, i32* %a, align 4\n"
                "%9 = icmp eq i32 %8, 50\n"
                "br i1 %9, label %10, label %11\n"

                "call void @exit(i32 1) #2\n"
                "unreachable\n"

                "%12 = load i32, i32* %a, align 4\n"
                "%13 = add nsw i32 %12, 1\n"
                "store i32 %13, i32* %a, align 4\n"
                "br label %1\n"

                "ret void\n"
                "}\n"

                "declare void @exit(i32)\n", AssemblyHolderType::STRING_TYPE);

  test_result_map trm;

  trm.insert({"number of exits", 2});
  ExpectTestPass(trm);
}

TEST_F(TestClassifyLoopExits, FuncCallLoopExits) {
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
                "br i1 %4, label %5, label %14\n"

                "%6 = load i32, i32* %a, align 4\n"
                "%7 = add nsw i32 %6, 1\n"
                "store i32 %7, i32* %a, align 4\n"
                "%8 = load i32, i32* %a, align 4\n"
                "%9 = icmp eq i32 %8, 50\n"
                "br i1 %9, label %10, label %11\n"

                "call void @potential_exit(i32 1)\n"
                "br label %11\n"

                "%12 = load i32, i32* %a, align 4\n"
                "%13 = add nsw i32 %12, 1\n"
                "store i32 %13, i32* %a, align 4\n"
                "br label %1\n"

                "ret void\n"
                "}\n"

                "declare void @potential_exit(i32)\n", AssemblyHolderType::STRING_TYPE);

  test_result_map trm;

  trm.insert({"number of exits", 1});
  ExpectTestPass(trm);
}

} // namespace anonymous end
} // namespace icsa end
