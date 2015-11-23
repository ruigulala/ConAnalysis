//===------------------------- DangerOpLabel.cpp --------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// Copyright (c) 2015 Columbia University. All rights reserved.
// This file is a skeleton of an implementation for the ConAnalysis
// pass of Columbia University in the City of New York. For this program,
// our goal is to find those particular concurrency bugs that will make
// the system vulnerable to malicious actions.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "DOL"

#include "../../include/ConAnal/DangerOpLabel.h"

using namespace llvm;
using namespace ConAnal;

bool DOL::findDangerousOp(Module &M) {
  for (auto funciter = M.getFunctionList().begin();
      funciter != M.getFunctionList().end(); funciter++) {
    Function *F = funciter;
    for (auto I = inst_begin(F), E = inst_end(F); I != E; ++I) {
      if (isa<GetElementPtrInst>(*I)) {
        if (MDNode *N = I->getMetadata("dbg")) {
          DILocation Loc(N);
          errs() << F->getName().str() << " ("
              << Loc.getFilename().str() << ":"
              << Loc.getLineNumber() << ")\n";
        }
      }
    }
  }
  return false;
}

bool DOL::runOnModule(Module &M) {
  findDangerousOp(M);
  return false;
}

//**********************************************************************
// print (do not change this method)
//
// If this pass is run with -f -analyze, this method will be called
// after each call to runOnModule.
//**********************************************************************
void DOL::print(std::ostream &O, const Module *M) const {
  O << "This is Concurrency Bug Analysis.\n";
}

char DOL::ID = 1;
// register the DOL class:
//  - give it a command-line argument (DOL)
//  - a name ("Dangerous Operation Labelling")
//  - a flag saying that we don't modify the CFG
//  - a flag saying this is not an analysis pass
static RegisterPass<DOL> X("DOL",
                           "dangerous operation labelling",
                           true, false);
