//===- Conflicts.cpp - Example code from "Writing an LLVM Pass"
//---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements two versions of the LLVM "Conflicts World" pass
// described in docs/WritingAnLLVMPass.html
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
//#include "llvm/Transforms/Instrumentation.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
using namespace llvm;

#define DEBUG_TYPE "conflicts"


namespace {
void InsertProfilingInitCall(Function *MainFn, const char *FnName,
                             unsigned NumCalls) {
  LLVMContext &C = MainFn->getContext();
  Type *UIntTy = Type::getInt32Ty(C);
  FunctionType *FTy =
      FunctionType::get(Type::getVoidTy(C), UIntTy, /*isVarArg=*/false);
  Module &M = *MainFn->getParent();
  Constant *ProfFn = M.getOrInsertFunction(FnName, FTy);

  auto Entry = MainFn->begin();
  BasicBlock::iterator InsertPos = Entry->begin();
  while (isa<AllocaInst>(InsertPos))
    ++InsertPos;

  CallInst::Create(ProfFn, {ConstantInt::get(UIntTy, NumCalls)}, "",
                   &*InsertPos);
}

void InsertProfilingCall(Module::iterator Fn, const char *FnName, Value *Addr1,
                         Value *Addr2,
                         unsigned CallNO, Instruction *Inst) {
  LLVMContext &Context = Fn->getContext();
  Type *VoidTy = Type::getVoidTy(Context);
  Type *UIntTy = Type::getInt32Ty(Context);
  Module &M = *Fn->getParent();
  Constant *ProfFn = M.getOrInsertFunction(FnName, VoidTy, Addr1->getType(),
                                           Addr2->getType(), UIntTy);

  CallInst::Create(ProfFn, {Addr1, Addr2, ConstantInt::get(UIntTy, CallNO)}, "",
                   Inst);
}

void profileInstruction(Module::iterator &FIt,
                        BasicBlock::iterator &CurrentInst,
                        BasicBlock::iterator &NextInst, unsigned &NumCalls) {
  if (auto *BO = dyn_cast<BinaryOperator>(CurrentInst)) {
    DEBUG(dbgs() << "\t\tadding a profiling function\n");
    if (BO->getOpcode() != Instruction::Mul &&
        BO->getOpcode() != Instruction::Add)
      return;
    if (isa<LoadInst>(BO->getOperand(0)) && isa<LoadInst>(BO->getOperand(1))) {
      auto Addr1 = dyn_cast<LoadInst>(BO->getOperand(0))->getPointerOperand();
      auto Addr2 = dyn_cast<LoadInst>(BO->getOperand(1))->getPointerOperand();
      InsertProfilingCall(FIt, "llvm_conflict_profiling", Addr1, Addr2, NumCalls,
                          &*NextInst);
      ++NumCalls;
      DEBUG(dbgs() << "\t\tinserted llvm_conflict_profiling");
    }
  }
  // Looks like there is no need to split BB
  // auto splitBB = SplitBlock(&(*CurrentBB), &(*NextInst));
  // CurrentBB = splitBB->getIterator();
  // IE = CurrentBB->end();
  // DEBUG(dbgs() << "\t\tadd a profiling function and generate a new BB "
  //             << CurrentBB->getName() << "\n");
}

// Conflicts pass
struct Conflicts : public ModulePass {
  static char ID;
  Conflicts() : ModulePass(ID) {}

  bool runOnModule(Module &M) override {
    DEBUG(dbgs() << "I am in Conflicts\n");

    Function *Main = M.getFunction("main"); // for C/C++ programs
    if (Main == 0) {
      errs() << "WARNING: cannot insert memory profiling into a module"
             << " with no main function!\n";
      return false; // No main, no instrumentation!
    }

    unsigned NumCalls = 0;
    for (Module::iterator FIt = M.begin(), FE = M.end(); FIt != FE; ++FIt) {
      if (FIt->isDeclaration())
        continue;
      DEBUG(dbgs() << FIt->getName() << "\n");
      for (Function::iterator BB = FIt->begin(), BBE = FIt->end(); BB != BBE;) {
        DEBUG(dbgs() << "\t" << BB->getName() << "\n");
        auto CurrentBB = BB;
        auto NextBB = ++BB;
        for (BasicBlock::iterator I = CurrentBB->begin(), IE = CurrentBB->end();
             I != IE;) {
          auto CurrentInst = I;
          auto NextInst = ++I;
          profileInstruction(FIt, CurrentInst, NextInst, NumCalls);
          I = NextInst;
        }
        BB = NextBB;
      }
    }

    errs() << "The total number of places of potential conflicts: " << NumCalls
           << "\n";

    // Add the initialization call to main.
    InsertProfilingInitCall(Main, "llvm_start_conflict_profiling", NumCalls);
    return true;
  }
};
} // namespace

char Conflicts::ID = 0;


INITIALIZE_PASS(Conflicts, "conflicts", "conflicts", false, false)

// ModulePass *llvm::createConflictsPass() { return new Conflicts(); } // see
// LinkPasses.h
