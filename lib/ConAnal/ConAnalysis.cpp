//===---------------------------- ConAnalysis.cpp -------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is a skeleton of an implementation for the ConAnalysis
// pass of Columbia University in the City of New York.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "ConAnalysis"
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
} part1_input;

typedef struct {
  char fileName[200];
  char funcName[100];
  int lineNum;
  Instruction * danOpI;
} part2_input;

namespace {
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
    
    public:
    static char ID; // Pass identification, replacement for typeid
    ConAnalysis() : ModulePass(ID) {
    }

    void printSet(std::set<int> &inputSet) {
      std::set<int>::iterator iter;
      errs() << "[ ";
      for(iter=inputSet.begin(); iter!=inputSet.end(); ++iter) {
        errs() << *iter << " ";
      }
      errs() << "]\n";
    }

    void printSet(std::set<BasicBlock *> inputSet) {
      std::list<std::string> tmpList;
      for(std::set<BasicBlock *>::iterator it = inputSet.begin();
          it != inputSet.end(); ++it) {
        tmpList.push_back((*it)->getName().str());
      }
      tmpList.sort();
      for(std::list<std::string>::iterator it = tmpList.begin();
          it != tmpList.end(); ++it) {
        errs() << *it << " ";
      }
    }

    void parseInput() {
      // Read input from loc.txt
      std::ifstream ifs("loc.txt");
      std::vector<std::string> lines;

      std::string line;
      std::getline(ifs, line);
      sscanf(line.c_str()," %s %s %d %s\n", p1_input.fileName,
             p1_input.funcName,
             &p1_input.lineNum,
             p1_input.varName);
      line.clear();
      std::getline(ifs, line);
      sscanf(line.c_str()," %s %s %d\n", p2_input.fileName,
             p2_input.funcName,
             &p2_input.lineNum);

      errs() << "Replaying input:\n";
      errs() << "Part 1:\n";
      errs() << "Filename:" << p1_input.fileName << "\n";
      errs() << "FuncName:" << p1_input.funcName << "\n";
      errs() << "LineNum:" << p1_input.lineNum << "\n\n";
      errs() << "VarName:" << p1_input.varName << "\n";

      errs() << "Part 2:\n";
      errs() << "Filename:" << p2_input.fileName << "\n";
      errs() << "FuncName:" << p2_input.funcName << "\n";
      errs() << "LineNum:" << p2_input.lineNum << "\n\n";

    }

    void mapSourceToIR(Module &M) {
      bool getPart1 = false;
      bool getPart2 = false;
      for(Module::iterator FuncIter = M.getFunctionList().begin();
          FuncIter != M.getFunctionList().end(); FuncIter++) {
        Function *F = FuncIter;
        for(inst_iterator I = inst_begin(F), E = inst_end(F); 
            I != E; ++I) {
          if(MDNode *N = I->getMetadata("dbg")) {
            DILocation Loc(N);
            unsigned Line = Loc.getLineNumber();
            StringRef File = Loc.getFilename();
            StringRef Dir = Loc.getDirectory();
            if(File.str().compare(p1_input.fileName) == 0) {
              if(Line == p1_input.lineNum && isa<LoadInst>(*I)) {
                errs() << "Done mapping part1." << "\n";
                //std::string tmp;
                //llvm::raw_string_ostream rso(tmp);
                //I->print(rso);
                //errs() << tmp << "\n";
                corruptedIR.insert(ins2int[&(*I)]);
              }
            }
            if(!getPart2 && 
               File.str().compare(p2_input.fileName) == 0) {
              if(Line == p2_input.lineNum) {
                getPart2 = true;
                p2_input.danOpI = &*I;
                errs() << "Done mapping part2" << "\n";
              }
            }
          }
        }
      }
      return;
    }

    virtual bool runOnModule(Module &M) {

      //CallGraph CG = CallGraph(M);
      //for (CallGraph::const_iterator I = CG.begin(), E = CG.end(); I != E; ++I) {
        //errs() << "  CS<" << (I->first)->getName() << "> calls ";
        //if (Function *FI = I->second->getFunction())
          //errs() << "function '" << FI->getName() <<"'\n";
        //else
          //errs() << "external node\n";
      //}
      // Put dying into container

      mapInsToNum(M);
      printMap(M);
      parseInput();
      mapSourceToIR(M);
      // TODO : TEMPORARY HACK FOR REAL LIBSAFE
      corruptedIR.insert(23);
      part1_getCorruptedIRs(M);
      part2_getDominantFrontiers(M);
      part3_getFeasiblePath(M);
   
      return false;
    }

    virtual bool mapInsToNum(Module &M) {
      for(Module::iterator FuncIter = M.getFunctionList().begin();
          FuncIter != M.getFunctionList().end(); FuncIter++) {
        Function *F = FuncIter;
        for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
          ins2int[&(*I)] = ins_count++;
        }
      }
      return false;
    }

    virtual bool printMap(Module &M) {
      for(Module::iterator FuncIter = M.getFunctionList().begin();
          FuncIter != M.getFunctionList().end(); FuncIter++) {
        Function *F = FuncIter;
        std::cerr << "\nFUNCTION " << F->getName().str() << "\n";
      
        for(Function::iterator blk = F->begin(), blk_end = F->end();
             blk != blk_end; ++blk)
        {
          errs() << "\nBASIC BLOCK " << blk->getName() << "\n";
          for(BasicBlock::iterator i = blk->begin(), e = blk->end(); 
               i != e; ++i) {
            errs() << "%" << ins2int[i] << ":\t";
            errs() << Instruction::getOpcodeName(i->getOpcode()) << "\t";
            for(int operand_i = 0, operand_num = i->getNumOperands(); 
                operand_i < operand_num; operand_i++)
            {
              Value * v = i->getOperand(operand_i);
              if (isa<Instruction>(v)) {
                errs() << "%" << ins2int[cast<Instruction>(v)] << " ";
              }
              else if(v->hasName())
                errs() << v->getName() << " ";
              else
                errs() << "XXX ";
            }
            errs() << "\n";
          }
        }
      }
      return false;
    }

    virtual bool part1_getCorruptedIRs(Module &M) {
      for(Module::iterator FuncIter = M.getFunctionList().begin();
          FuncIter != M.getFunctionList().end(); FuncIter++) {
        Function *F = FuncIter;
        if(F->getName().str().compare(p1_input.funcName) != 0)
          continue;
        errs() << "------------Function Name :" << F->getName() << "-----\n";
        for(inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
          for(int op_i = 0, op_num = I->getNumOperands();
              op_i < op_num; op_i++) {
            Value * v = I->getOperand(op_i);
            if (isa<Instruction>(v)) {
              if(corruptedIR.count(ins2int[cast<Instruction>(v)])) {
                corruptedIR.insert(ins2int[&*I]);
                continue;
              }
            }
            else if (v->hasName()) {
              //errs() << v->getName() << " ";
            } else {
              //errs() << "XXX ";
            }
          }
        }
      }
      errs() << "------------- Part 1 Result ----------------\n";
      printSet(corruptedIR);
      return false;
    }

    virtual bool part2_getDominantFrontiers(Module &M) {

      for(Module::iterator FuncIter = M.getFunctionList().begin();
          FuncIter != M.getFunctionList().end(); FuncIter++) {
        Function *F = FuncIter;
        std::map<BasicBlock *, std::set<BasicBlock *> > dominators;
        if(F->getName().str().compare(p2_input.funcName) == 0) {
          computeDominators(*F, dominators);
          printDominators(*F, dominators);
          std::set<BasicBlock *>::iterator it, it_end;
          it = dominators[(p2_input.danOpI)->getParent()].begin();
          it_end = dominators[(p2_input.danOpI)->getParent()].end();
          for(; it != it_end; ++it) {
            for(BasicBlock::iterator i = (*it)->begin(), 
                e = (*it)->end(); i != e; ++i) {
              dominantFrontiers.insert(ins2int[i]);  
              if(&*i == p2_input.danOpI)
                break;
            }
          }
          break;
        }
      }
      errs() << "------------- Part 2 Result ----------------\n";
      printSet(dominantFrontiers); 
      return false;
    }

    virtual bool part3_getFeasiblePath(Module &M) {
      std::set_intersection(corruptedIR.begin(), corruptedIR.end(), 
                            dominantFrontiers.begin(),
                            dominantFrontiers.end(),
                            std::inserter(feasiblePath, 
                                          feasiblePath.end()));  
      errs() << "------------- Part 3 Result ----------------\n";
      printSet(feasiblePath);
      return false;
    }

    void computeDominators(Function &F, std::map<BasicBlock *,
                           std::set<BasicBlock *> > & dominators) {
      std::vector<BasicBlock *> worklist;
      // For all the nodes but N0, initially set dom(N) = {all nodes}
      Function::iterator entry = F.begin();
      for(Function::iterator blk = F.begin(), blk_end = F.end(); 
          blk != blk_end; blk++) {
        if(blk != F.begin()) {
          if(pred_begin(blk) != pred_end(blk)) {
            for(Function::iterator blk_p = F.begin(), blk_p_end = F.end();
                blk_p != blk_p_end; ++blk_p) {
              dominators[&*blk].insert(&*blk_p);
            }
          }
          else
            dominators[&*blk].insert(&*blk);
        }
      }
      // dom(N0) = {N0} where N0 is the start node
      dominators[entry].insert(&*entry);
      // Push each node but N0 onto the worklist
      for(succ_iterator SI = succ_begin(entry), E = succ_end(entry); 
          SI != E; ++SI) {
        worklist.push_back(*SI);
      }
      // Use the worklist algorithm to compute dominators
      while(!worklist.empty()) {
        BasicBlock * Z = worklist.front();
        worklist.erase(worklist.begin());

        std::set<BasicBlock *> intersects = dominators[*pred_begin(Z)];

        for(pred_iterator PI = pred_begin(Z), E = pred_end(Z); 
            PI != E; ++PI) {
          std::set<BasicBlock *>::iterator dom_it, dom_end;
          std::set<BasicBlock *> newDoms;
          for(dom_it = dominators[*PI].begin(), 
              dom_end = dominators[*PI].end();
              dom_it != dom_end; dom_it++) {
            if(intersects.count(*dom_it))
              newDoms.insert(*dom_it);
          }
          intersects = newDoms;
        }
        intersects.insert(Z);

        if(dominators[Z] != intersects) {
          dominators[Z] = intersects;

          for(succ_iterator SI = succ_begin(Z), E = succ_end(Z);
              SI != E; ++SI) {
            if(*SI == entry)
              continue;
            else if(std::find(worklist.begin(),
                              worklist.end(), *SI) == worklist.end()) {
              worklist.push_back(*SI);
            }
          }
        }
      }
    }

    void printDominators(Function &F, std::map<BasicBlock *,
                         std::set<BasicBlock *> > & dominators) {
      errs() << "\nFUNCTION " << F.getName() << "\n";
      for(Function::iterator blk = F.begin(), blk_end = F.end();
          blk != blk_end; ++blk) {
        errs() << "BASIC BLOCK " << blk->getName() << " DOM-Before: { ";
        dominators[&*blk].erase(&*blk);
        printSet(dominators[&*blk]);
        errs() << "}  DOM-After: { ";
        if(pred_begin(blk) != pred_end(blk) || blk == F.begin()) {
          dominators[&*blk].insert(&*blk);
        }
        printSet(dominators[&*blk]);
        errs() << "}\n";
      }
    }

    //**********************************************************************
    // print (do not change this method)
    //
    // If this pass is run with -f -analyze, this method will be called
    // after each call to runOnFunction.
    //**********************************************************************
    virtual void print(std::ostream &O, const Module *M) const {
        O << "This is Concurrency Bug Analysis.\n";
    }

    //**********************************************************************
    // getAnalysisUsage
    //**********************************************************************

    // We don't modify the program, so we preserve all analyses
    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.setPreservesAll();
    };

  };
  char ConAnalysis::ID = 0;

  // register the ConAnalysis class: 
  //  - give it a command-line argument (ConAnalysis)
  //  - a name ("Concurrency Bug Analysis")
  //  - a flag saying that we don't modify the CFG
  //  - a flag saying this is not an analysis pass
  RegisterPass<ConAnalysis> X("ConAnalysis", "concurrency bug analysis code",
			   true, false);
}
