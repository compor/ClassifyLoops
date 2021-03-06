
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

#include <string>
// using std::string

#include "ClassifyLoops.hpp"

namespace icsa {
namespace {

using test_result_t = boost::variant<bool, int, std::string>;
using test_result_map = std::map<std::string, test_result_t>;

struct test_result_visitor : public boost::static_visitor<unsigned int> {
  unsigned int operator()(bool b) const { return b ? 1 : 0; }
  unsigned int operator()(int i) const { return i; }
  unsigned int operator()(const std::string &s) const {
    return s == "test" ? 1 : 0;
  }
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

        auto &CurLoop = *LI.begin();
        assert(CurLoop && "Loop ptr is invalid");

        test_result_map::const_iterator found;

        std::set<std::string> iocalls;
        std::set<std::string> nonlocalexitcalls;

        iocalls.insert("foo");
        nonlocalexitcalls.insert("bar");

        const auto &stats = calculate(LI, &iocalls, &nonlocalexitcalls);

        // subcase
        found = lookup("containing function");
        if (found != std::end(m_trm)) {
          const auto &rv = (stats[0].second.ContainingFunc == "test") ? 1 : 0;
          const auto &ev =
              boost::apply_visitor(test_result_visitor(), found->second);
          EXPECT_EQ(ev, rv) << found->first;
        }

        // subcase
        found = lookup("total number of exits");
        if (found != std::end(m_trm)) {
          const auto &rv = stats[0].second.NumHeaderExits +
                           stats[0].second.NumNonHeaderExits;
          const auto &ev =
              boost::apply_visitor(test_result_visitor(), found->second);
          EXPECT_EQ(ev, rv) << found->first;
        }

        // subcase
        found = lookup("number of header exits");
        if (found != std::end(m_trm)) {
          const auto &rv = stats[0].second.NumHeaderExits;
          const auto &ev =
              boost::apply_visitor(test_result_visitor(), found->second);
          EXPECT_EQ(ev, rv) << found->first;
        }

        // subcase
        found = lookup("number of inner loops");
        if (found != std::end(m_trm)) {
          const auto &rv = stats[0].second.NumInnerLoops;
          const auto &ev =
              boost::apply_visitor(test_result_visitor(), found->second);
          EXPECT_EQ(ev, rv) << found->first;
        }

        // subcase
        found = lookup("number of different exit landings");
        if (found != std::end(m_trm)) {
          const auto &rv = stats[0].second.NumDiffExitLandings;
          const auto &ev =
              boost::apply_visitor(test_result_visitor(), found->second);
          EXPECT_EQ(ev, rv) << found->first;
        }

        // subcase
        found = lookup("number of inner loop exits");
        if (found != std::end(m_trm)) {
          const auto &rv = stats[0].second.NumInnerLoopExits;
          const auto &ev =
              boost::apply_visitor(test_result_visitor(), found->second);
          EXPECT_EQ(ev, rv) << found->first;
        }

        // subcase
        found = lookup("number of inner loop toplevel exits");
        if (found != std::end(m_trm)) {
          const auto &rv = stats[0].second.NumInnerLoopTopLevelExits;
          const auto &ev =
              boost::apply_visitor(test_result_visitor(), found->second);
          EXPECT_EQ(ev, rv) << found->first;
        }

        // subcase
        found = lookup("has IO calls");
        if (found != std::end(m_trm)) {
          const auto &rv = stats[0].second.HasIOCalls;
          const auto &ev =
              boost::apply_visitor(test_result_visitor(), found->second);
          EXPECT_EQ(ev, rv) << found->first;
        }

        // subcase
        found = lookup("number of non local exit calls");
        if (found != std::end(m_trm)) {
          const auto &rv = stats[0].second.NumNonLocalExits;
          const auto &ev =
              boost::apply_visitor(test_result_visitor(), found->second);
          EXPECT_EQ(ev, rv) << found->first;
        }

        return false;
      }

      test_result_map::const_iterator lookup(const std::string &subcase,
                                             bool fatalIfMissing = false) {
        auto found = m_trm.find(subcase);
        if (fatalIfMissing && m_trm.end() == found) {
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
  ParseAssembly("test01.ll");
  test_result_map trm;

  trm.insert({"total number of exits", 1});
  trm.insert({"number of header exits", 1});
  trm.insert({"number of different exit landings", 1});
  trm.insert({"number of inner loops", 0});
  trm.insert({"number of inner loop exits", 0});
  trm.insert({"number of inner loop toplevel exits", 0});
  trm.insert({"has IO calls", false});
  trm.insert({"number of non local exit calls", 0});
  ExpectTestPass(trm);
}

TEST_F(TestClassifyLoopExits, DefiniteInfiniteLoopExits) {
  ParseAssembly("test02.ll");
  test_result_map trm;

  trm.insert({"total number of exits", 0});
  trm.insert({"number of header exits", 0});
  trm.insert({"number of different exit landings", 0});
  trm.insert({"number of inner loops", 0});
  trm.insert({"number of inner loop exits", 0});
  trm.insert({"number of inner loop toplevel exits", 0});
  trm.insert({"has IO calls", false});
  trm.insert({"number of non local exit calls", 0});
  ExpectTestPass(trm);
}

TEST_F(TestClassifyLoopExits, DeadLoopExits) {
  ParseAssembly("test03.ll");
  test_result_map trm;

  trm.insert({"total number of exits", 1});
  trm.insert({"number of header exits", 1});
  trm.insert({"number of different exit landings", 1});
  trm.insert({"number of inner loops", 0});
  trm.insert({"number of inner loop exits", 0});
  trm.insert({"number of inner loop toplevel exits", 0});
  ExpectTestPass(trm);
}

TEST_F(TestClassifyLoopExits, BreakConditionLoopExits) {
  ParseAssembly("test04.ll");
  test_result_map trm;

  trm.insert({"total number of exits", 2});
  trm.insert({"number of header exits", 1});
  trm.insert({"number of inner loops", 0});
  trm.insert({"number of different exit landings", 2});
  trm.insert({"number of inner loop exits", 0});
  trm.insert({"number of inner loop toplevel exits", 0});
  trm.insert({"has IO calls", false});
  trm.insert({"number of non local exit calls", 0});
  ExpectTestPass(trm);
}

TEST_F(TestClassifyLoopExits, ContinueConditionLoopExits) {
  ParseAssembly("test05.ll");
  test_result_map trm;

  trm.insert({"total number of exits", 1});
  trm.insert({"number of header exits", 1});
  trm.insert({"number of different exit landings", 1});
  trm.insert({"number of inner loops", 0});
  trm.insert({"number of inner loop exits", 0});
  trm.insert({"number of inner loop toplevel exits", 0});
  ExpectTestPass(trm);
}

TEST_F(TestClassifyLoopExits, ReturnStmtLoopExits) {
  ParseAssembly("test06.ll");
  test_result_map trm;

  trm.insert({"total number of exits", 2});
  trm.insert({"number of header exits", 1});
  trm.insert({"number of different exit landings", 2});
  trm.insert({"number of inner loops", 0});
  trm.insert({"number of inner loop exits", 0});
  trm.insert({"number of inner loop toplevel exits", 0});
  trm.insert({"has IO calls", false});
  trm.insert({"number of non local exit calls", 0});
  ExpectTestPass(trm);
}

TEST_F(TestClassifyLoopExits, ExitCallLoopExits) {
  ParseAssembly("test07.ll");
  test_result_map trm;

  trm.insert({"total number of exits", 2});
  trm.insert({"number of header exits", 1});
  trm.insert({"number of different exit landings", 2});
  trm.insert({"number of inner loops", 0});
  trm.insert({"number of inner loop exits", 0});
  trm.insert({"number of inner loop toplevel exits", 0});
  trm.insert({"has IO calls", false});
  trm.insert({"number of non local exit calls", 0});
  ExpectTestPass(trm);
}

TEST_F(TestClassifyLoopExits, FuncCallLoopExits) {
  ParseAssembly("test08.ll");
  test_result_map trm;

  trm.insert({"total number of exits", 1});
  trm.insert({"number of header exits", 1});
  trm.insert({"number of different exit landings", 1});
  trm.insert({"number of inner loops", 0});
  trm.insert({"number of inner loop exits", 0});
  trm.insert({"number of inner loop toplevel exits", 0});
  trm.insert({"has IO calls", false});
  trm.insert({"number of non local exit calls", 0});
  ExpectTestPass(trm);
}

TEST_F(TestClassifyLoopExits, ReturnInnerLoopExits) {
  ParseAssembly("test09.ll");
  test_result_map trm;

  trm.insert({"total number of exits", 2});
  trm.insert({"number of header exits", 1});
  trm.insert({"number of different exit landings", 2});
  trm.insert({"number of inner loops", 1});
  trm.insert({"number of inner loop exits", 2});
  trm.insert({"number of inner loop toplevel exits", 1});
  trm.insert({"has IO calls", false});
  trm.insert({"number of non local exit calls", 0});
  ExpectTestPass(trm);
}

TEST_F(TestClassifyLoopExits, SpecialCallsLoop) {
  ParseAssembly("test10.ll");
  test_result_map trm;

  trm.insert({"total number of exits", 1});
  trm.insert({"number of header exits", 1});
  trm.insert({"number of different exit landings", 1});
  trm.insert({"number of inner loops", 0});
  trm.insert({"number of inner loop exits", 0});
  trm.insert({"number of inner loop toplevel exits", 0});
  trm.insert({"has IO calls", true});
  trm.insert({"number of non local exit calls", 1});
  ExpectTestPass(trm);
}

TEST_F(TestClassifyLoopExits, RegularInnerLoops) {
  ParseAssembly("test11.ll");
  test_result_map trm;

  trm.insert({"total number of exits", 1});
  trm.insert({"number of header exits", 1});
  trm.insert({"number of different exit landings", 1});
  trm.insert({"number of inner loops", 1});
  trm.insert({"number of inner loop exits", 1});
  trm.insert({"number of inner loop toplevel exits", 0});
  trm.insert({"has IO calls", false});
  trm.insert({"number of non local exit calls", 0});
  ExpectTestPass(trm);
}

} // namespace anonymous end
} // namespace icsa end
