//===--------------- printCode.cpp - Project 1 for CS 701 ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is a skeleton of an implementation for the printCode
// pass of Univ. Wisconsin-Madison's CS 701 Project 1.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "printCode"
#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/raw_ostream.h"
//#include "llvm/Support/InstIterator.h"
//#include "llvm/User.h"
#include <iostream>
#include <vector>
using namespace llvm;

namespace {
  class printCode : public FunctionPass {
    private:
      std::map<Instruction *, int> ins2int;
      int ins_count;
    public:
    static char ID; // Pass identification, replacement for typeid
    printCode() : FunctionPass(ID) 
    {
      ins_count = 1;
    }

    void addToMap(Function &F)
    {
      for (Function::iterator blk = F.begin(), blk_end = F.end(); blk != blk_end; ++blk)
        for (BasicBlock::iterator i = blk->begin(), e = blk->end(); i != e; ++i)
        {
          ins2int[i] = ins_count++;
        }
    }

    //**********************************************************************
    // runOnFunction
    //**********************************************************************
    virtual bool runOnFunction(Function &F) {
      addToMap(F);
      // print fn name
      std::cerr << "\nFUNCTION " << F.getName().str() << "\n";
      // MISSING: Add code here to do the following:
      //          1. Iterate over the instructions in F, creating a
      //             map from instruction address to unique integer.
      //             (It is probably a good idea to put this code in
      //             a separate, private method.)
      //          2. Iterate over the basic blocks in the function and
      //             print each instruction in that block using the format
      //             given in the assignment.

      //func is a pointer to a Function instance
      for (Function::iterator blk = F.begin(), blk_end = F.end(); blk != blk_end; ++blk)
      {
        // blk is a pointer to a BasicBlock instance
        errs() << "\nBASIC BLOCK " << blk->getName() << "\n";
        for (BasicBlock::iterator i = blk->begin(), e = blk->end(); i != e; ++i)
        {
          // The next statement works since operator<<(ostream&,...)
          // is overloaded for Instruction&
          errs() << "%" << ins2int[i] << ":\t";
          errs() << Instruction::getOpcodeName(i->getOpcode()) << "\t";
          for (int operand_i = 0, operand_num = i->getNumOperands(); operand_i < operand_num; operand_i++)
          {
            Value * v = i->getOperand(operand_i);
            if (isa<Instruction>(v))
            {
              errs() << "%" << ins2int[cast<Instruction>(v)] << " ";
            }
            else
            {
              if (v->hasName())
                errs() << v->getName() << " ";
              else
                errs() << "XXX ";
            }
          }
          errs() << "\n";
        }
      }
      return false;  // because we have NOT changed this function
    }

    //**********************************************************************
    // print (do not change this method)
    //
    // If this pass is run with -f -analyze, this method will be called
    // after each call to runOnFunction.
    //**********************************************************************
    virtual void print(std::ostream &O, const Module *M) const {
        O << "This is printCode.\n";
    }

    //**********************************************************************
    // getAnalysisUsage
    //**********************************************************************

    // We don't modify the program, so we preserve all analyses
    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.setPreservesAll();
    };

  };
  char printCode::ID = 0;

  // register the printCode class: 
  //  - give it a command-line argument (printCode)
  //  - a name ("print code")
  //  - a flag saying that we don't modify the CFG
  //  - a flag saying this is not an analysis pass
  RegisterPass<printCode> X("printCode", "print code",
			   true, false);
}
