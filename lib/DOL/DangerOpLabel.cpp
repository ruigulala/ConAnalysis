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

#include "ConAnal/DangerOpLabel.h"

#include <fstream>

using namespace llvm;
using namespace ConAnal;

void DOL::clearClassDataMember() {
  danPtrOps_.clear();
  danFuncOps_.clear();
  danFuncs_.clear();
}

bool DOL::findDangerousOp(Module &M) {
  for (auto funciter = M.getFunctionList().begin();
      funciter != M.getFunctionList().end(); funciter++) {
    Function *F = funciter;
    for (auto I = inst_begin(F), E = inst_end(F); I != E; ++I) {
      if (isa<GetElementPtrInst>(*I)) {
        if (MDNode *N = I->getMetadata("dbg")) {
          DILocation Loc(N);
          std::string fileName = Loc.getFilename().str();
          std::string funcName = F->getName();
          uint32_t lineNum = Loc.getLineNumber();
          FuncFileLine opEntry = std::make_tuple(funcName, fileName, lineNum);
          if (std::find(danPtrOps_.begin(), danPtrOps_.end(), opEntry) ==
              danPtrOps_.end()) {
            DEBUG(errs() << funcName << " (" << fileName
                << ":" << lineNum << ")\n");
            danPtrOps_.push_front(opEntry);
          }
        }
      } else if (isa<CallInst>(&*I) || isa<InvokeInst>(&*I)) {
        CallSite cs(&*I);
        Function * callee = cs.getCalledFunction();
        if (!callee) {
          continue;
        }
        errs() << callee->getName().str() << "\n";
        if (danFuncs_.count(callee->getName().str())) {
          if (MDNode *N = I->getMetadata("dbg")) {
            DILocation Loc(N);
            std::string fileName = Loc.getFilename().str();
            std::string funcName = F->getName();
            uint32_t lineNum = Loc.getLineNumber();
            FuncFileLine opEntry = std::make_tuple(funcName, fileName, lineNum);
            if (std::find(danFuncOps_.begin(), danFuncOps_.end(), opEntry) ==
              danFuncOps_.end()) {
                errs() << funcName << " (" << fileName
                    << ":" << lineNum << ")\n";
                danFuncOps_.push_front(opEntry);
            }
          }
        }
      }
    }
  }
  return false;
}

void DOL::parseInput(std::string inputfile) {
  char funcName[300];
  std::ifstream ifs(inputfile);
  if (!ifs.is_open()) {
    errs() << "Runtime Error: Couldn't find dangerous_func_list.txt\n";
    abort();
  }
  std::string line;
  DEBUG(errs() << "Replaying the input file:\n");
  DEBUG(errs() << "Read from file " << inputfile << "\n");
  std::getline(ifs, line);

  while (std::getline(ifs, line)) {
    // Input format: funcName
    sscanf(line.c_str(), "%s\n", funcName);
    danFuncs_.insert(std::string(funcName));
    line.clear();
  }
}

bool DOL::runOnModule(Module &M) {
  clearClassDataMember();
  parseInput("dangerous_func_list.txt");
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

char DOL::ID = 0;
// register the DOL class:
//  - give it a command-line argument (DOL)
//  - a name ("Dangerous Operation Labelling")
//  - a flag saying that we don't modify the CFG
//  - a flag saying this is not an analysis pass
static RegisterPass<DOL> X("DOL",
                           "dangerous operation labelling",
                           true, true);
