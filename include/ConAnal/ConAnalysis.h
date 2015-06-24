//===---------------------------- ConAnalysis.h ---------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is a skeleton of an implementation for the ConAnalysis
// pass of Columbia University in the City of New York. For this program,
// our goal is to find those particular concurrency bugs that will make
// the system vulnerable to malicious actions.
//
//===----------------------------------------------------------------------===//

#ifndef CONANAL_H
#define CONANAL_H

#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>
#include <fstream>
#include <list>
#include <set>
#include <vector>

using namespace llvm;

typedef struct {
  char fileName[200];
  char funcName[100];
  int lineNum;
  char varName[100];
  // TODO: Add a container that stores the initial input of callStack
} part1_input;

typedef struct {
  char fileName[200];
  char funcName[100];
  int lineNum;
  Instruction * danOpI;
} part2_input;

namespace ConAnal {
  class ConAnalysis : public ModulePass {
    private:
    part1_input p1_input;
    part2_input p2_input;

    std::map<Instruction *, int> ins2int;
    int ins_count = 1;
    //std::set<Instruction *> corruptedIR;
    std::set<int> corruptedIR;
    //std::set<Instruction *> dominantFrontiers;
    std::set<int> dominantFrontiers;
    //std::set<Instruction *> feasiblePath;
    std::set<int> feasiblePath;
    // TODO: Add a runtime callStack instance for analysis
    //std::stack<pair<Function *, Instruction *> > callStack;
    
    public:
    static char ID; // Pass identification, replacement for typeid
    ConAnalysis() : ModulePass(ID) {
    }

    ///
    void printSet(std::set<int> &inputSet);
    ///
    void printSet(std::set<BasicBlock *> inputSet);
    ///
    void parseInput();
    ///
    void mapSourceToIR(Module &M);
    ///
    virtual bool runOnModule(Module &M);
    ///
    virtual bool mapInsToNum(Module &M);
    ///
    virtual bool printMap(Module &M);
    /// 
    virtual bool part1_getCorruptedIRs(Module &M);
    ///
    virtual bool part2_getDominantFrontiers(Module &M);
    ///
    virtual bool part3_getFeasiblePath(Module &M);
    ///
    void computeDominators(Function &F, std::map<BasicBlock *,
                           std::set<BasicBlock *> > & dominators);
    ///
    void printDominators(Function &F, std::map<BasicBlock *,
                         std::set<BasicBlock *> > & dominators);

    //**********************************************************************
    // print (do not change this method)
    //
    // If this pass is run with -f -analyze, this method will be called
    // after each call to runOnFunction.
    //**********************************************************************
    virtual void print(std::ostream &O, const Module *M) const;

    //**********************************************************************
    // getAnalysisUsage
    //**********************************************************************

    // We don't modify the program, so we preserve all analyses
    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.setPreservesAll();
    };

  };
}

#endif
