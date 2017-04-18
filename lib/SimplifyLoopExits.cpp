//
//
//

#include "SimplifyLoopExits.hpp"

#include "llvm/Pass.h"
// using llvm::RegisterPass

#include "llvm/IR/Type.h"
// using llvm::Type

#include "llvm/IR/Instruction.h"
// using llvm::Instruction

#include "llvm/IR/Function.h"
// using llvm::Function

#include "llvm/Support/Casting.h"
// using llvm::dyn_cast

#include "llvm/IR/LegacyPassManager.h"
// using llvm::PassManagerBase

#include "llvm/Transforms/IPO/PassManagerBuilder.h"
// using llvm::PassManagerBuilder
// using llvm::RegisterStandardPasses

#include "llvm/Support/raw_ostream.h"
// using llvm::raw_ostream

#include "llvm/Support/CommandLine.h"
// using llvm::cl::opt
// using llvm::cl::desc

#define DEBUG_TYPE "SimplifyLoopExits"

#include "llvm/Support/Debug.h"
// using DEBUG macro
// using llvm::dbgs

// plugin registration for opt

#define STRINGIFY_UTIL(x) #x
#define STRINGIFY(x) STRINGIFY_UTIL(x)

#define PRJ_CMDLINE_STRING                                                     \
  "simplify loop exits (version: " STRINGIFY(VERSION_STRING) ")"

char SimplifyLoopExits::ID = 0;
static llvm::RegisterPass<SimplifyLoopExits>
    X("simplifyloopexits", PRJ_CMDLINE_STRING, false, false);

// plugin registration for clang

// the solution was at the bottom of the header file
// 'llvm/Transforms/IPO/PassManagerBuilder.h'
// create a static free-floating callback that uses the legacy pass manager to
// add an instance of this pass and a static instance of the
// RegisterStandardPasses class

static void registerSimplifyLoopExits(const llvm::PassManagerBuilder &Builder,
                                      llvm::legacy::PassManagerBase &PM) {
  PM.add(new SimplifyLoopExits());

  return;
}

static llvm::RegisterStandardPasses
    RegisterSimplifyLoopExits(llvm::PassManagerBuilder::EP_EarlyAsPossible,
                              registerSimplifyLoopExits);

//

namespace {

bool SimplifyLoopExits::runOnFunction(llvm::Function &f) {
  return false;
}

} // namespace unnamed end
