//===---------------------------- ConAnalysis.cpp -------------------------===//
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

#include "ConAnal/ConAnalysis.h"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

using namespace llvm;
using namespace ConAnal;

void ConAnalysis::clearClassDataMember() {
  corruptedVar_.clear();
  ins2int_.clear();
  sourcetoIRmap_.clear();
  corruptedIR_.clear();
  corruptedPtr_.clear();
  orderedcorruptedIR_.clear();
  callstack_.clear();
  corruptedMap_.clear();
}

const Function* ConAnalysis::findEnclosingFunc(const Value* V) {
  if (const Argument* Arg = dyn_cast<Argument>(V)) {
    return Arg->getParent();
  }
  if (const Instruction* I = dyn_cast<Instruction>(V)) {
    return I->getParent()->getParent();
  }
  return NULL;
}

const MDNode* ConAnalysis::findVar(const Value* V, const Function* F) {
  for (const_inst_iterator Iter = inst_begin(F), End = inst_end(F);
      Iter != End; ++Iter) {
    const Instruction* I = &*Iter;
    if (const DbgDeclareInst* DbgDeclare = dyn_cast<DbgDeclareInst>(I)) {
      if (DbgDeclare->getAddress() == V)
        return DbgDeclare->getVariable();
    } else if (const DbgValueInst* DbgValue = dyn_cast<DbgValueInst>(I)) {
      if (DbgValue->getValue() == V)
        return DbgValue->getVariable();
    }
  }
  return NULL;
}

StringRef ConAnalysis::getOriginalName(const Value* V) {
  const Function* F = findEnclosingFunc(V);
  if (!F)
    return V->getName();
  const MDNode* Var = findVar(V, F);
  if (!Var)
    return "tmp";
  return DIVariable(Var).getName();
}

void ConAnalysis::printList(std::list<Value *> &inputset) {
  errs() << "[ ";
  for (auto& iter : inputset) {
    if (isa<Instruction>(iter)) {
      errs() << ins2int_[cast<Instruction>(iter)] << " ";
    } else {
      errs() << iter->getName() << " ";
    }
  }
  errs() << "]\n";
}

void ConAnalysis::printSet(std::set<BasicBlock *> &inputset) {
  std::list<std::string> tmplist;
  for (auto& it : inputset) {
    tmplist.push_back(it->getName().str());
  }
  tmplist.sort();
  for (auto& it : tmplist) {
    errs() << it << " ";
  }
}

bool ConAnalysis::add2CrptList(Value * crptVal) {
  if (!corruptedIR_.count(&*crptVal)) {
    orderedcorruptedIR_.push_back(&*crptVal);
    corruptedIR_.insert(&*crptVal);
    return true;
  }
  return false;
}

void ConAnalysis::parseInput(std::string inputfile, FuncFileLineList &csinput) {
  char varName[300];
  std::ifstream ifs(inputfile);
  if (!ifs.is_open()) {
    errs() << "Runtime Error: Couldn't find race_report.txt\n";
    errs() << "Please check the file path\n";
    abort();
  }
  std::string line;
  //errs() << "Replaying input:\n";
  //errs() << "Read from file " << inputfile << "\n";
  std::getline(ifs, line);
  sscanf(line.c_str(), "%s\n", varName);
  corruptedVar_ = std::string(varName);
  errs() << "Corrupted Var Name:" << corruptedVar_ << "\n";
  while (std::getline(ifs, line)) {
    char fileName[300];
    char funcName[300];
    uint32_t lineNum;
    // Input format: funcName (fileName:lineNum)
    sscanf(line.c_str(), "%s (%[^ :]:%u)\n", funcName, fileName, &lineNum);
    std::string s1(fileName), s2(funcName);
    csinput.push_front(std::make_tuple(s1, s2, lineNum));
    //errs() << "Funcname:" << s2 << "\n";
    //errs() << "FileName:" << s1 << "\n";
    //errs() << "LineNum:" << lineNum << "\n\n";
    line.clear();
  }
}

void ConAnalysis::initializeCallStack(FuncFileLineList &csinput) {
  errs() << "---------------------------------------\n";
  errs() << "       Initializing call stack\n";
  errs() << "---------------------------------------\n";
  for (auto cs_it = csinput.begin(); cs_it != csinput.end(); ++cs_it) {
    std::string filename = std::get<0>(*cs_it);
    uint32_t line = std::get<2>(*cs_it);
    auto mapitr = sourcetoIRmap_.find(std::make_pair(filename, line));
    if (mapitr == sourcetoIRmap_.end()) {
      errs() << "ERROR: <" << std::get<0>(*cs_it) << " "
             << std::get<2>(*cs_it) << ">"
             << " sourcetoIRmap_ look up failed.\n";
      abort();
    }
    std::list<Instruction *>& insList = mapitr->second;
    if (insList.begin() == insList.end()) {
      errs() << "ERROR: <" << std::get<0>(*cs_it) << " "
             << std::get<2>(*cs_it) << ">"
             << " No matching instructions.\n";
      abort();
    }
    for (auto listit = insList.begin(); listit != insList.end(); ++listit) {
      errs() << "Begin:\n";
      (*listit)->print(errs());
      errs() << "\n";
      if (isa<GetElementPtrInst>(*listit)) {
        StringRef cvar = getOriginalName((*listit)->getOperand(0));
        StringRef cvar_compare = StringRef(corruptedVar_);
        errs() << "geteleptr: " << cvar << "\n";
        errs() << "corruptedVar_: " << corruptedVar_ << "\n";
        if (!cvar_compare.count(cvar))
          return;
        (*listit)->print(errs());
        Function * func = &*(((*listit)->getParent())->getParent());
        errs() << func->getName() << "\n";
        callstack_.push_front(std::make_pair(&*func, *listit));
        break;
      } else if (std::next(cs_it, 1) == csinput.end()) {
        (*listit)->print(errs());
        Function * func = &*(((*listit)->getParent())->getParent());
        errs() << func->getName() << "\n";
        callstack_.push_front(std::make_pair(&*func, *listit));
        break;
      } else if (isa<CallInst>(*listit) || isa<InvokeInst>(*listit)) {
        Function * func = &*(((*listit)->getParent())->getParent());
        errs() << func->getName() << "\n";
        callstack_.push_front(std::make_pair(&*func, *listit));
        break;
      }
    }
    errs() << "\n";
  }
  return;
}

bool ConAnalysis::runOnModule(Module &M) {
  clearClassDataMember();
  DOL &labels = getAnalysis<DOL>();
  FuncFileLineList raceReport;
  errs() << "---------------------------------------\n";
  errs() << "             ConAnalysis               \n";
  errs() << "---------------------------------------\n";
  createMaps(M);
  printMap(M);
  parseInput("race_report.txt", raceReport);
  initializeCallStack(raceReport);
  getCorruptedIRs(M);
  if (PtrDerefCheck) {
    errs() << "---------------------------------------\n";
    errs() << "   Pointer dereference check result    \n";
    errs() << "---------------------------------------\n";
    getDominators(M, labels.danPtrOps_);
  }
  if (DanFuncCheck) {
    errs() << "---------------------------------------\n";
    errs() << "   Dangerous function check result     \n"; 
    errs() << "---------------------------------------\n";
    getDominators(M, labels.danFuncOps_);
  }
  return false;
}

bool ConAnalysis::createMaps(Module &M) {
  for (auto funciter = M.getFunctionList().begin();
      funciter != M.getFunctionList().end(); funciter++) {
    Function *F = funciter;
    for (auto I = inst_begin(F), E = inst_end(F); I != E; ++I) {
      ins2int_[&(*I)] = ins_count_++;
      if (MDNode *N = I->getMetadata("dbg")) {
        DILocation Loc(N);
        uint32_t line = Loc.getLineNumber();
        StringRef file = Loc.getFilename();
        sourcetoIRmap_[std::make_pair(file.str(), line)].push_back(&*I);
      } else {
        if (isa<PHINode>(&*I) || isa<AllocaInst>(&*I) || isa<BranchInst>(&*I)) {
        } else if (isa<CallInst>(&*I) || isa<InvokeInst>(&*I)) {
        }
      }
    }
  }
  return false;
}

bool ConAnalysis::printMap(Module &M) {
  for (auto funcIter = M.getFunctionList().begin();
      funcIter != M.getFunctionList().end(); funcIter++) {
    Function *F = funcIter;
    std::cerr << "\nFUNCTION " << F->getName().str() << "\n";
    for (auto blk = F->begin(); blk != F->end(); ++blk) {
      errs() << "\nBASIC BLOCK " << blk->getName() << "\n";
      for (auto i = blk->begin(); i != blk->end(); ++i) {
        errs() << "%" << ins2int_[i] << ":\t";
        errs() << Instruction::getOpcodeName(i->getOpcode()) << "\t";
        for (uint32_t op_i = 0; op_i < i->getNumOperands(); op_i++) {
          Value * v = i->getOperand(op_i);
          if (isa<Instruction>(v)) {
            errs() << "%" << ins2int_[cast<Instruction>(v)] << " ";
          } else if (v->hasName()) {
            errs() << v->getName() << " ";
          } else {
            errs() << "XXX ";
          }
        }
        errs() << "\n";
      }
    }
  }
  return false;
}

bool ConAnalysis::getCorruptedIRs(Module &M) {
  errs() << "---------------------------------------\n";
  errs() << "       Get Corrupted IRs           \n";
  errs() << "---------------------------------------\n";
  while (!callstack_.empty()) {
    auto& loc = callstack_.front();
    CorruptedArgs coparams;
    errs() << "Original Callstack: Go into \"" << loc.first->getName()
           << "\"\n";
    bool addFuncRet = false;
    addFuncRet = intraDataflowAnalysis(loc.first, loc.second, coparams);
    errs() << "Callstack POP " << loc.first->getName() << "\n";
    callstack_.pop_front();
    if (addFuncRet && !callstack_.empty()) {
      auto& loc = callstack_.front();
      add2CrptList(&*loc.second);
    }
  }
  errs() << "---------------------------------------\n";
  errs() << "   Part 1: Dataflow Result             \n";
  errs() << "---------------------------------------\n";
  printList(orderedcorruptedIR_);
  errs() << "\n";
  return false;
}

bool ConAnalysis::intraDataflowAnalysis(Function * F, Instruction * ins,
                                        CorruptedArgs & corruptedparams) {
  bool rv = false;
  auto I = inst_begin(F);
  if (ins != nullptr) {
    for (; I != inst_end(F); ++I) {
      if (&*I == &*ins) {
        // Skip the previous call instruction after returned.
        if (isa<CallInst>(&*I)) {
          ++I;
        } else {
          if (!corruptedIR_.count(&*I)) {
            orderedcorruptedIR_.push_back(&*I);
            corruptedIR_.insert(&*I);
            errs() << "Adding corrupted variable: ";
            errs() << "%" << ins2int_[&*I] << "\n";
            if (isa<GetElementPtrInst>(&*I)) {
              int op_num = I->getNumOperands();
              GepIdxStruct * gep_idx = reinterpret_cast<GepIdxStruct *>
                  (malloc(sizeof(GepIdxStruct)));
              if (op_num == 3) {
                gep_idx->idxType = GEP_THREE_OP;
                gep_idx->gepIdx.array_idx = std::make_pair(I->getOperand(1),
                                                           I->getOperand(2));
              } else if (op_num == 2) {
                gep_idx->idxType = GEP_TWO_OP;
                gep_idx->gepIdx.idx = I->getOperand(1);
              } else {
                errs() << "Error: Cannot parse GetElementPtrInst\n";
                abort();
              }
              Value * v = I->getOperand(0);
              errs() << "Adding " << &*v << " to crptPtr list\n";
              corruptedPtr_[v].push_back(gep_idx);
            }
            rv = true;
          }
        }
        break;
      }
    }
    assert(I != inst_end(F) && "Couldn't find callstack instruction.");
  }
  if (I == inst_end(F)) {
    errs() << "Couldn't obtain the source code of function \""
           << F->getName() << "\"\n";
    return false;
  }
  // Handle the corrupted var passed in as function parameters
  uint32_t op_i = 0;
  for (Function::arg_iterator args = F->arg_begin();
       args != F->arg_end(); ++args, ++op_i) {
    if (corruptedparams.count(op_i)) {
      Value * calleeArgs = &*args;
      errs() << "Corrupted Arg: " << calleeArgs->getName() << "\n";
      if (corruptedIR_.count(corruptedparams[op_i])
          && !corruptedIR_.count(calleeArgs)) {
        errs() << "Add " << calleeArgs->getName() << " to list\n";
        orderedcorruptedIR_.push_back(calleeArgs);
        corruptedIR_.insert(calleeArgs);
      }
      Value * callerArgs = corruptedparams[op_i];
      if (corruptedPtr_.count(&*callerArgs)) {
        errs() << "Add arg " << &*calleeArgs << " to crptPtr list\n";
        corruptedPtr_[&*calleeArgs] = corruptedPtr_[&*callerArgs];
      }
    }
  }
  for (; I != inst_end(F); ++I) {
    if (isa<CallInst>(&*I)) {
      CallSite cs(&*I);
      Function * callee = cs.getCalledFunction();
      if (!callee) {
        errs() << "Couldn't get callee for instruction ";
        I->print(errs()); errs() << "\n";
        continue;
      }
      // Skip all the llvm debug function
      std::string fnname = callee->getName().str();
      if (fnname.compare(0, 5, "llvm.") == 0)
        continue;
      // Check for cycles
      bool cycle_flag = false;
      for (auto& csit : callstack_) {
        if ((std::get<0>(csit))->getName().str().compare(
                (callee->getName()).str()) == 0) {
          cycle_flag = true;
          break;
        }
      }
      if (cycle_flag) continue;
      CorruptedArgs coparams;
      // Iterate through all the parameters to find the corrupted ones
      for (uint32_t op_i = 0, op_num = I->getNumOperands(); op_i < op_num;
           op_i++) {
        Value * v = I->getOperand(op_i);
        if (isa<Instruction>(v)) {
          if (corruptedIR_.count(v) || corruptedPtr_.count(v)) {
            errs() << "Param No." << op_i << " %"
            << ins2int_[dyn_cast<Instruction>(v)] << " contains corruption.\n";
            coparams[op_i] = &*v;
          }
        }
      }
      errs() << "\"" << F->getName() << "\"" << " calls "
             << "\"" << callee->getName() << "\"\n";
      errs() << "Callstack PUSH " << callee->getName() << "\n";
      callstack_.push_front(std::make_pair(callee, nullptr));
      bool addFuncRet = false;
      addFuncRet = intraDataflowAnalysis(callee, nullptr, coparams);
      if (addFuncRet) {
        add2CrptList(&*I);
      }
      errs() << "Callstack POP " << callstack_.front().first->getName() << "\n";
      callstack_.pop_front();
    } else if (isa<GetElementPtrInst>(&*I)) {
      int op_ii = I->getNumOperands();
      if (corruptedPtr_.count(I->getOperand(0))) {
        auto coPtrList = corruptedPtr_[I->getOperand(0)];
        //I->print(errs()); errs() << "\n";
        if (op_ii == 2) {
          Value * gepIdx = I->getOperand(1);
          for (auto it : coPtrList) {
            if (it->idxType == GEP_TWO_OP) {
              if (it->gepIdx.idx == gepIdx) {
                errs() << "Add %" << ins2int_[&*I]<< " to crpt list\n";
                add2CrptList(&*I);
              }
            }
          }
        } else if (op_ii == 3) {
          auto gepIdxPair = std::make_pair(I->getOperand(1), I->getOperand(2));
          for (auto it : coPtrList) {
            if (it->idxType == GEP_THREE_OP) {
              if (it->gepIdx.array_idx == gepIdxPair) {
                errs() << "Add %" << ins2int_[&*I]<< " to crpt list\n";
                add2CrptList(&*I);
              }
            }
          }
        }
      }
      for (uint32_t op_i = 0, op_num = I->getNumOperands(); op_i < op_num;
           op_i++) {
        Value * v = I->getOperand(op_i);
        if (corruptedIR_.count(v)) {
          if (!corruptedIR_.count(&*I)) {
            orderedcorruptedIR_.push_back(&*I);
            corruptedIR_.insert(&*I);
            errs() << "Add %" << ins2int_[&*I] << " to crpt list\n";
            int op_ii = I->getNumOperands();
            GepIdxStruct * gep_idx = reinterpret_cast<GepIdxStruct *>
                (malloc(sizeof(GepIdxStruct)));
            if (op_ii == 3) {
              gep_idx->idxType = GEP_THREE_OP;
              gep_idx->gepIdx.array_idx = std::make_pair(I->getOperand(1),
                                                         I->getOperand(2));
            } else if (op_ii == 2) {
              gep_idx->idxType = 1;
              gep_idx->gepIdx.idx = I->getOperand(1);
            } else {
              errs() << "Error: Cannot parse GetElementPtrInst\n";
              abort();
            }
            corruptedPtr_[I->getOperand(1)].push_back(gep_idx);
          }
        }
      }
    } else {
      for (uint32_t op_i = 0, op_num = I->getNumOperands(); op_i < op_num;
           op_i++) {
        Value * v = I->getOperand(op_i);
        if (corruptedIR_.count(v)) {
          if (!corruptedIR_.count(&*I)) {
            orderedcorruptedIR_.push_back(&*I);
            corruptedIR_.insert(&*I);
            if (isa<StoreInst>(&*I)) {
              Value * v = I->getOperand(1);
              add2CrptList(v);
            }
            rv = true;
          }
          break;
        }
      }
    }
  }
  return rv;
}

bool ConAnalysis::getDominators(Module &M, FuncFileLineList &danOps) {
  for (auto FuncIter = M.getFunctionList().begin();
       FuncIter != M.getFunctionList().end(); FuncIter++) {
    Function *F = FuncIter;
    std::map<BasicBlock *, std::set<BasicBlock *>> dominators;
    // ffl means fileFuncLine
    for (auto fflTuple = danOps.begin(); fflTuple != danOps.end(); fflTuple++) {
      std::list<Value *> dominatorSubset;
      //errs() << F->getName() << " " << std::get<0>(*fflTuple) << "\n";
      if (F->getName().str().compare(std::get<0>(*fflTuple)) == 0) {
        computeDominators(*F, dominators);
        //printDominators(*F, dominators);
        // filename, lineNum -> Instruction *
        std::string filename = std::get<1>(*fflTuple);
        uint32_t line = std::get<2>(*fflTuple);
        auto mapitr = sourcetoIRmap_.find(std::make_pair(filename, line));
        if (mapitr == sourcetoIRmap_.end()) {
          errs() << "ERROR: <" << std::get<1>(*fflTuple) << " "
                 << std::get<2>(*fflTuple) << ">"
                 << " sourcetoIRmap_ look up failed.\n";
          abort();
        }
        auto fileLinePair = std::make_pair(filename, line);
        Instruction * danOpI = sourcetoIRmap_[fileLinePair].front();
        auto it = dominators[danOpI->getParent()].begin();
        auto it_end = dominators[danOpI->getParent()].end();
        for (; it != it_end; ++it) {
          for (auto i = (*it)->begin(); i != (*it)->end(); ++i) {
            dominatorSubset.push_back(&*i);
            if (&*i == danOpI)
              break;
          }
        }
        if (!getFeasiblePath(M, dominatorSubset))
          continue;
        errs() << "Dangerous Operation Basic Block & Instruction\n";
        errs() << danOpI->getParent()->getName() << " & "
               << ins2int_[&*danOpI] << "\n";
        if (MDNode *N = danOpI->getMetadata("dbg")) {
          DILocation Loc(N);
          errs() << F->getName().str() << " ("
              << Loc.getFilename().str() << ":"
              << Loc.getLineNumber() << ")\n";
        }
        errs() << "\n";
      }
      //errs() << "---------------------------------------\n";
      //errs() << "         Dominator Result              \n";
      //errs() << "---------------------------------------\n";
      //printList(dominantfrontiers);
      //errs() << "\n";
    }
  }
  return false;
}

bool ConAnalysis::getFeasiblePath(Module &M,
                                        std::list<Value *> &dominantfrontiers) {
  std::list<Value *> feasiblepath;
  for (auto& listitr : orderedcorruptedIR_) {
    for (auto& listitr2 : dominantfrontiers) {
      if (listitr == listitr2) {
        feasiblepath.push_back(listitr);
      }
    }
  }
  if (feasiblepath.empty())
    return false;
  errs() << "---------------------------------------\n";
  errs() << "   Part 3: Path Intersection           \n";
  errs() << "---------------------------------------\n";
  printList(feasiblepath);
  return true;
}

void ConAnalysis::computeDominators(Function &F, std::map<BasicBlock *,
                                    std::set<BasicBlock *>> & dominators) {
  std::vector<BasicBlock *> worklist;
  // For all the nodes but N0, initially set dom(N) = {all nodes}
  auto entry = F.begin();
  for (auto blk = F.begin(); blk != F.end(); blk++) {
    if (blk != F.begin()) {
      if (pred_begin(blk) != pred_end(blk)) {
        for (auto blk_p = F.begin(); blk_p != F.end(); ++blk_p) {
          dominators[&*blk].insert(&*blk_p);
        }
      } else {
        dominators[&*blk].insert(&*blk);
      }
    }
  }
  // dom(N0) = {N0} where N0 is the start node
  dominators[entry].insert(&*entry);
  // Push each node but N0 onto the worklist
  for (auto SI = succ_begin(entry), E = succ_end(entry); SI != E; ++SI) {
    worklist.push_back(*SI);
  }
  // Use the worklist algorithm to compute dominators
  while (!worklist.empty()) {
    BasicBlock * Z = worklist.front();
    worklist.erase(worklist.begin());
    std::set<BasicBlock *> intersects = dominators[*pred_begin(Z)];
    for (auto PI = pred_begin(Z), E = pred_end(Z);
        PI != E; ++PI) {
      std::set<BasicBlock *> newDoms;
      for (auto dom_it = dominators[*PI].begin(),
          dom_end = dominators[*PI].end();
          dom_it != dom_end; dom_it++) {
        if (intersects.count(*dom_it))
          newDoms.insert(*dom_it);
      }
      intersects = newDoms;
    }
    intersects.insert(Z);

    if (dominators[Z] != intersects) {
      dominators[Z] = intersects;
      for (auto SI = succ_begin(Z), E = succ_end(Z); SI != E; ++SI) {
        if (*SI == entry) {
          continue;
        } else if (std::find(worklist.begin(),
                            worklist.end(), *SI) == worklist.end()) {
          worklist.push_back(*SI);
        }
      }
    }
  }
}

void ConAnalysis::printDominators(Function &F, std::map<BasicBlock *,
                                  std::set<BasicBlock *>> & dominators) {
  errs() << "\nFUNCTION " << F.getName() << "\n";
  for (auto blk = F.begin(); blk != F.end(); ++blk) {
    errs() << "BASIC BLOCK " << blk->getName() << " DOM-Before: { ";
    dominators[&*blk].erase(&*blk);
    printSet(dominators[&*blk]);
    errs() << "}  DOM-After: { ";
    if (pred_begin(blk) != pred_end(blk) || blk == F.begin()) {
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
// after each call to runOnModule.
//**********************************************************************
void ConAnalysis::print(std::ostream &O, const Module *M) const {
    O << "This is Concurrency Bug Analysis.\n";
}

char ConAnalysis::ID = 0;

// register the ConAnalysis class:
//  - give it a command-line argument (ConAnalysis)
//  - a name ("Concurrency Bug Analysis")
//  - a flag saying that we don't modify the CFG
//  - a flag saying this is not an analysis pass
static RegisterPass<ConAnalysis> X("ConAnalysis",
                                   "concurrency bug analysis code",
                                    true, false);
