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
#define DEBUG_TYPE "con-analysis"

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

static cl::opt<bool> PtrDerefCheck("ptrderef",
    cl::desc("Enable ptr deref check"));
static cl::opt<bool> DanFuncCheck("danfunc",
    cl::desc("do dangerous function check"));
static cl::opt<std::string> RaceReportInput("raceReport",
    cl::desc("race report input file"), cl::Required);

void ConAnalysis::clearClassDataMember() {
  // Rummor says there will be funky problems if we don't clear these contains.
  ins2int_.clear();
  sourcetoIRmap_.clear();
  corruptedIR_.clear();
  finishedVars_.clear();
  corruptedPtr_.clear();
  orderedcorruptedIR_.clear();
  callStack_.clear();
  callStackHead_.clear();
  callStackBody_.clear();
  corruptedMap_.clear();
  funcEnterExitValMap_.clear();
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
  for (auto& iter : inputset) {
    printMappedInstruction(iter); 
  }
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
  std::ifstream ifs(inputfile);
  if (!ifs.is_open()) {
    errs() << "Runtime Error: Couldn't find " << inputfile << "\n";
    errs() << "Please check the file path\n";
    abort();
  }
  std::string line;
  while (std::getline(ifs, line)) {
    char fileName[300];
    char funcName[300];
    uint32_t lineNum;
    int rv = 0;
    // Input format: funcName (fileName:lineNum)
    rv = sscanf(line.c_str(), "%s (%[^ :]:%u)\n", funcName, fileName, &lineNum);
    assert(rv > 0 && "Invalid input line!\n");
    std::string s1(funcName), s2(fileName);
    csinput.push_back(std::make_tuple(s1, s2, lineNum));
    line.clear();
  }
}

void ConAnalysis::initializeCallStack(FuncFileLineList &csinput) {
  errs() << "---- Replaying call stack input ----\n";
  for (auto cs_it = csinput.begin(); cs_it != csinput.end(); ++cs_it) {
    std::string filename = std::get<1>(*cs_it);
    uint32_t line = std::get<2>(*cs_it);
    errs() << "(" << filename << " : " << line << ")\n";
    auto mapitr = sourcetoIRmap_.find(std::make_pair(filename, line));
    if (mapitr == sourcetoIRmap_.end()) {
      errs() << "ERROR: <" << std::get<1>(*cs_it) << " "
             << std::get<2>(*cs_it) << ">"
             << " sourcetoIRmap_ look up failed.\n";
      continue;
    }
    std::list<Instruction *>& insList = mapitr->second;
    if (insList.begin() == insList.end()) {
      errs() << "ERROR: <" << std::get<1>(*cs_it) << " "
             << std::get<2>(*cs_it) << ">"
             << " No matching instructions.\n";
      abort();
    }
    for (auto listit = insList.begin(); listit != insList.end(); ++listit) {
      // This makes sure that other than the first one, all the other callstack
      // layers are filled with call instruction.
      if (isa<GetElementPtrInst>(*listit)) {
        if (cs_it != csinput.begin())
          continue;
        // This flag checks if the corrupted variable is already added
        // to the head.
        bool flagAlreadyAddedVar = false;
        for (uint32_t op_i = 0; op_i < (*listit)->getNumOperands(); op_i++) {
          if (finishedVars_.count((*listit)->getOperand(op_i))) {
            flagAlreadyAddedVar = true;
            break;
          }
        }
        if (flagAlreadyAddedVar)
          break;
        Function * func = &*(((*listit)->getParent())->getParent());
        DEBUG(errs() << func->getName() << " contains the above ins\n");
        callStackHead_.push_back(std::make_pair(&*func, *listit));
        finishedVars_.insert(*listit);
        //errs() << "-------------------------\n";
        //(*listit)->print(errs());errs() << "\n"; 
        //errs() << "-------------------------\n";
      } else if (isa<CallInst>(*listit) || isa<InvokeInst>(*listit)) {
        CallSite cs(*listit);
        Function * callee = cs.getCalledFunction();
        if (callee != NULL) {
          // Skip llvm.dbg function
          std::string fnname = callee->getName().str();
          if (fnname.compare(0, 5, "llvm.") == 0)
            continue;
        }
        if (cs_it == csinput.begin() && listit == insList.begin()) {
          errs() << "Warning: Call Inst %" << ins2int_[*listit]
                 << " is the first one in the call stack!\n";
          continue;
        }
        Function * func = &*(((*listit)->getParent())->getParent());
        DEBUG((*listit)->print(errs()); errs() << "\n";);
        DEBUG(errs() << func->getName() << " contains the above ins\n");
        callStackBody_.push_back(std::make_pair(&*func, *listit));
        break;
      } else {
        if (cs_it != csinput.begin())
          continue;
        Function * func = &*(((*listit)->getParent())->getParent());
        bool flagAlreadyAddedVar = false;
        for (uint32_t op_i = 0; op_i < (*listit)->getNumOperands(); op_i++) {
          if (finishedVars_.count((*listit)->getOperand(op_i))) {
            flagAlreadyAddedVar = true;
            break;
          }
        }
        if (flagAlreadyAddedVar)
          break;
        callStackHead_.push_back(std::make_pair(&*func, *listit));
        finishedVars_.insert(*listit);
        //errs() << "-------------------------\n";
        //(*listit)->print(errs());errs() << "\n"; 
        //errs() << "-------------------------\n";
      }
    }
    DEBUG(errs() << "\n");
  }
  errs() << "\n";
  return;
}

bool ConAnalysis::runOnModule(Module &M) {
  clearClassDataMember();
  DOL &labels = getAnalysis<DOL>();
  FuncFileLineList raceReport;
  errs() << "---------------------------------------\n";
  errs() << "       Start ConAnalysis Pass          \n";
  errs() << "---------------------------------------\n";
  createMaps(M);
  printMap(M);
  parseInput(RaceReportInput, raceReport);
  initializeCallStack(raceReport);
  getCorruptedIRs(M, labels);
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
        // There might be a lot of generated LLVM IRs couldn't map to any
        // line of the source code.
        //errs() << "Warning: Couldn't dbg Metadata for LLVM IR\n";
        //I->print(errs()); errs() << "\n";
        if (isa<PHINode>(&*I) || isa<AllocaInst>(&*I) || isa<BranchInst>(&*I)) {
        } else if (isa<CallInst>(&*I) || isa<InvokeInst>(&*I)) {
        }
      }
    }
  }
  return false;
}

bool ConAnalysis::printMappedInstruction(Value * v) {
  if (isa<Instruction>(v)) {
    Instruction * i = dyn_cast<Instruction>(v);
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
  } else {
    errs() << "Function args: " << v->getName();
  }
  errs() << "\n";
  return true;
}

bool ConAnalysis::printMap(Module &M) {
  for (auto funcIter = M.getFunctionList().begin();
      funcIter != M.getFunctionList().end(); funcIter++) {
    Function *F = funcIter;
    DEBUG(errs() << "\nFUNCTION " << F->getName().str() << "\n");
    for (auto blk = F->begin(); blk != F->end(); ++blk) {
      DEBUG(errs() << "\nBASIC BLOCK " << blk->getName() << "\n");
      for (auto i = blk->begin(); i != blk->end(); ++i) {
        DEBUG(errs() << "%" << ins2int_[i] << ":\t");
        DEBUG(errs() << Instruction::getOpcodeName(i->getOpcode()) << "\t");
        for (uint32_t op_i = 0; op_i < i->getNumOperands(); op_i++) {
          Value * v = i->getOperand(op_i);
          if (isa<Instruction>(v)) {
            DEBUG(errs() << "%" << ins2int_[cast<Instruction>(v)] << " ");
          } else if (v->hasName()) {
            DEBUG(errs() << v->getName() << " ");
          } else {
            DEBUG(errs() << "XXX ");
          }
        }
        DEBUG(errs() << "\n");
      }
    }
  }
  return false;
}

bool ConAnalysis::getCorruptedIRs(Module &M, DOL &labels) {
  DEBUG(errs() << "---- Getting Corrupted LLVM IRs ----\n");
  for (auto cs_itr : callStackHead_) {
    // Each time, we create a call stack using one element from callStackHead
    // and the whole callStackBody
    callStack_.clear();
    callStack_ = callStackBody_;
    callStack_.push_front(cs_itr);
    orderedcorruptedIR_.clear();
    corruptedIR_.clear();

    while (!callStack_.empty()) {
      auto& loc = callStack_.front();
      CorruptedArgs coparams;
      DEBUG(errs() << "Original Callstack: Go into \"" << loc.first->getName()
             << "\"\n");
      bool addFuncRet = false;
      addFuncRet = intraDataflowAnalysis(loc.first, loc.second, coparams);
      DEBUG(errs() << "Callstack POP " << loc.first->getName() << "\n");
      callStack_.pop_front();
      if (addFuncRet && !callStack_.empty()) {
        auto& loc = callStack_.front();
        add2CrptList(&*loc.second);
      }
    }
    DEBUG(errs() << "\n");
    errs() << "---- Part 1: Dataflow Result ---- \n";
    printList(orderedcorruptedIR_);
    errs() << "\n";

    std::set<Function *> corruptedIRFuncSet;
    for (auto IR = corruptedIR_.begin(); IR != corruptedIR_.end(); IR++) {
      if (!isa<Instruction> (*IR))
        continue;
      Function * func = dyn_cast<Instruction>(*IR)->getParent()->getParent();
      corruptedIRFuncSet.insert(func);
    }

    if (PtrDerefCheck) {
      errs() << "*********************************************************\n";
      errs() << "     Pointer Dereference Analysis Result                 \n";
      errs() << "   # of static pointer deference statements: "
        << labels.danPtrOps_.size() << "\n";
      uint32_t vulNum = getDominators(M, labels.danPtrOps_, corruptedIRFuncSet);
      errs() << "\n   # of detected potential vulnerabilities: "
        << vulNum << "\n";
      errs() << "*********************************************************\n";
      errs() << "\n";
    }
    if (DanFuncCheck) {
      errs() << "*********************************************************\n";
      errs() << "     Dangerous Function Analysis Result                  \n";
      errs() << "   # of static dangerous function statements: "
        << labels.danFuncs_.size() << "\n";
      uint32_t vulNum = getDominators(M, labels.danFuncOps_,
          corruptedIRFuncSet);
      errs() << "\n   # of detected potential vulnerabilities: "
        << vulNum << "\n";
      errs() << "*********************************************************\n";
      errs() << "\n";
    }
  }
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
            DEBUG(errs() << "Adding corrupted variable: ");
            DEBUG(errs() << "%" << ins2int_[&*I] << "\n");
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
              DEBUG(errs() << "Adding " << &*v << " to crptPtr list\n");
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
    DEBUG(errs() << "Couldn't obtain the source code of function \""
        << F->getName() << "\"\n");
  }
  // Handle the corrupted var passed in as function parameters
  uint32_t op_i = 0;
  for (Function::arg_iterator args = F->arg_begin();
       args != F->arg_end(); ++args, ++op_i) {
    if (corruptedparams.count(op_i)) {
      // Notice: Here, we relax the contraint of our data flow analysis a
      // little bit. If the arguments of a call instruction is corrupted and
      // we couldn't obtain its function body(external function), we'll treat
      // the return value of the call instruction as corrupted.
      if (I == inst_end(F))
        return true;
      Value * calleeArgs = &*args;
      DEBUG(errs() << "Corrupted Arg: " << calleeArgs->getName() << "\n");
      if (corruptedIR_.count(corruptedparams[op_i])
          && !corruptedIR_.count(calleeArgs)) {
        DEBUG(errs() << "Add " << calleeArgs->getName() << " to list\n");
        orderedcorruptedIR_.push_back(calleeArgs);
        corruptedIR_.insert(calleeArgs);
      }
      Value * callerArgs = corruptedparams[op_i];
      if (corruptedPtr_.count(&*callerArgs)) {
        DEBUG(errs() << "Add arg " << &*calleeArgs << " to crptPtr list\n");
        corruptedPtr_[&*calleeArgs] = corruptedPtr_[&*callerArgs];
      }
    }
  }
  for (; I != inst_end(F); ++I) {
    if (isa<CallInst>(&*I)) {
      CallSite cs(&*I);
      Function * callee = cs.getCalledFunction();
      if (!callee) {
        DEBUG(errs() << "Couldn't get callee for instruction ");
        DEBUG(I->print(errs()); errs() << "\n");
        continue;
      }
      // Skip all the llvm debug function
      std::string fnname = callee->getName().str();
      if (fnname.compare(0, 5, "llvm.") == 0)
        continue;
      // Check for cycles
      bool cycle_flag = false;
      for (auto& csit : callStack_) {
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
            DEBUG(errs() << "Param No." << op_i << " %"
            << ins2int_[dyn_cast<Instruction>(v)] << " contains corruption.\n");
            coparams[op_i] = &*v;
          }
        }
      }
      if (funcEnterExitValMap_.count(callee) != 0) {
        EnterExitVal funcVal = funcEnterExitValMap_[callee];
        if (funcVal.enterVal == funcVal.exitVal) {
          if (funcEnterExitValMap_[callee].enterVal < corruptedIR_.size()) {
            funcEnterExitValMap_[callee].enterVal = corruptedIR_.size();
          } else if (funcEnterExitValMap_[callee].enterVal ==
              corruptedIR_.size()) {
            DEBUG(errs() << "Skip function " << fnname << "\n");
            continue;
          }

        }
      } else {
        funcEnterExitValMap_[callee].enterVal = corruptedIR_.size();
      }
      DEBUG(errs() << "\"" << F->getName() << "\"" << " calls "
             << "\"" << callee->getName() << "\"\n");
      DEBUG(errs() << "Callstack PUSH " << callee->getName() << "\n");
      callStack_.push_front(std::make_pair(callee, nullptr));
      bool addFuncRet = false;
      addFuncRet = intraDataflowAnalysis(callee, nullptr, coparams);
      if (addFuncRet) {
        add2CrptList(&*I);
      }
      DEBUG(errs() << "Callstack POP " << callStack_.front().first->getName() 
          << "\n");
      funcEnterExitValMap_[callee].exitVal = corruptedIR_.size();
      callStack_.pop_front();
    } else if (isa<GetElementPtrInst>(&*I)) {
      int op_ii = I->getNumOperands();
      if (corruptedPtr_.count(I->getOperand(0))) {
        auto coPtrList = corruptedPtr_[I->getOperand(0)];
        if (op_ii == 2) {
          Value * gepIdx = I->getOperand(1);
          for (auto it : coPtrList) {
            if (it->idxType == GEP_TWO_OP) {
              if (it->gepIdx.idx == gepIdx) {
                DEBUG(errs() << "Add %" << ins2int_[&*I]<< " to crpt list\n");
                add2CrptList(&*I);
              }
            }
          }
        } else if (op_ii == 3) {
          auto gepIdxPair = std::make_pair(I->getOperand(1), I->getOperand(2));
          for (auto it : coPtrList) {
            if (it->idxType == GEP_THREE_OP) {
              if (it->gepIdx.array_idx == gepIdxPair) {
                DEBUG(errs() << "Add %" << ins2int_[&*I]<< " to crpt list\n");
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
            DEBUG(errs() << "Add %" << ins2int_[&*I] << " to crpt list\n");
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

uint32_t ConAnalysis::getDominators(Module &M, FuncFileLineList &danOps,
    std::set<Function *> &corruptedIRFuncSet) {
  uint32_t rv = 0;
  // ffl means FuncFileLine
  for (auto fflTuple = danOps.begin(); fflTuple != danOps.end(); fflTuple++) {
    BB2SetMap dominators;
    std::list<Value *> dominatorSubset;
    std::string funcName = std::get<0>(*fflTuple);
    std::string fileName = std::get<1>(*fflTuple);
    uint32_t line = std::get<2>(*fflTuple);
    InstructionList iList = sourcetoIRmap_[std::make_pair(fileName, line)];
    Function *F = iList.front()->getParent()->getParent();
    if (!corruptedIRFuncSet.count(F)) {
      continue;
    }
    assert(F != NULL && "Couldn't obtain Function * for dangerous op");
    if (F->getName().str().compare(std::get<0>(*fflTuple)) == 0) {
      if (dominatorMap_.count(F) == 0) {
        computeDominators(*F, dominators);
        dominatorMap_[F] = dominators;
      } else {
        dominators = dominatorMap_[F];
      }
      //printDominators(*F, dominators);
      std::string filename = std::get<1>(*fflTuple);
      uint32_t line = std::get<2>(*fflTuple);
      // filename, lineNum -> Instruction *
      auto mapitr = sourcetoIRmap_.find(std::make_pair(filename, line));
      if (mapitr == sourcetoIRmap_.end()) {
        errs() << "ERROR: <" << std::get<0>(*fflTuple) << " "
               << std::get<2>(*fflTuple) << ">"
               << " sourcetoIRmap_ look up failed.\n";
        abort();
      }
      auto fileLinePair = std::make_pair(filename, line);
      Instruction * danOpI = sourcetoIRmap_[fileLinePair].front();
      auto it = dominators[danOpI->getParent()].begin();
      auto it_end = dominators[danOpI->getParent()].end();
      bool corruptBranchFlag = false;
      for (; it != it_end; ++it) {
        for (auto i = (*it)->begin(); i != (*it)->end(); ++i) {
          if (isa<BranchInst>(&*i) && corruptedIR_.count(&*i)) {
            corruptBranchFlag = true;
          }
          dominatorSubset.push_back(&*i);
          if (&*i == danOpI)
            break;
        }
      }
      if (!corruptBranchFlag || !getFeasiblePath(M, dominatorSubset))
        continue;
      DEBUG(errs() << "Dangerous Operation Basic Block & Instruction\n");
      DEBUG(errs() << danOpI->getParent()->getName() << " & "
             << ins2int_[&*danOpI] << "\n");
      if (MDNode *N = danOpI->getMetadata("dbg")) {
        DILocation Loc(N);
        errs() << "Function: " << F->getName().str() << "(...)"
            << " Location: " << "(" << Loc.getFilename().str() << ":"
            << Loc.getLineNumber() << ")\n";
      }
      rv++;
    }
    //errs() << "---------------------------------------\n";
    //errs() << "         Dominator Result              \n";
    //errs() << "---------------------------------------\n";
    //printList(dominantfrontiers);
    //errs() << "\n";
  }
  return rv;
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
  errs() << "\n---- Part 3: Path Intersection ----\n";
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
