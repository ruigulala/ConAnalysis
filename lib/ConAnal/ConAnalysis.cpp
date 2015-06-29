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
// pass of Columbia University in the City of New York. For this program,
// our goal is to find those particular concurrency bugs that will make
// the system vulnerable to malicious actions.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "ConAnalysis"

#include "../../include/ConAnal/ConAnalysis.h"

using namespace llvm;
using namespace ConAnal;


void ConAnalysis::printSet(std::set<uint64_t> &inputset) {
  errs() << "[ ";
  for (auto& iter : inputset)
    errs() << iter << " "; 
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

void ConAnalysis::parseInput(std::string inputfile, CallStackInput &csinput) {
  std::ifstream ifs(inputfile);
  std::string line;
  errs() << "Replaying input:\n";
  errs() << "Read from file " << inputfile << "\n";
  while (std::getline(ifs, line)) {
    char fileName[300];
    char funcName[300];
    uint32_t lineNum;
    sscanf(line.c_str(), "%s (%[^ :]:%u)\n", funcName, fileName, &lineNum);
    std::string s1(fileName), s2(funcName);
    csinput.push_front(std::make_tuple(s1, s2, lineNum));
    errs() << "Funcname:" << s2 << "\n";
    errs() << "FileName:" << s1 << "\n";
    errs() << "LineNum:" << lineNum << "\n\n";
    line.clear();
  }
}

void ConAnalysis::initializeCallStack(CallStackInput &csinput) {
  errs() << "---------------------------------------\n";
  errs() << "       Initializing call stack.\n";
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
      (*listit)->print(errs());
      errs() << "\n";
      if (std::next(cs_it, 1) == csinput.end()) {
        Function * func = &*(((*listit)->getParent())->getParent());
        errs() << func->getName() << "\n";
        callstack_.push(std::make_pair(&*func, *listit));
        break;
      } else {
        if (isa<CallInst>(*listit) || isa<InvokeInst>(*listit)) {
          Function * func = &*(((*listit)->getParent())->getParent());
          errs() << func->getName() << "\n";
          callstack_.push(std::make_pair(&*func, *listit));
          break;
        } else {
          
        }
      }
    }
    errs() << "\n";
  }
  return;
}

bool ConAnalysis::runOnModule(Module &M) {
  CallStackInput p1_input, p2_input;
  errs() << "---------------------------------------\n";
  errs() << "             ConAnalysis               \n";
  errs() << "---------------------------------------\n";
  //CallGraph CG = CallGraph(M);
  //for (CallGraph::const_iterator I = CG.begin(), E = CG.end(); I != E; ++I) {
    //errs() << "  CS<" << (I->first)->getName() << "> calls ";
    //if (Function *FI = I->second->getFunction())
      //errs() << "function '" << FI->getName() <<"'\n";
    //else
      //errs() << "external node\n";
  //}
  // Put dying into container
  createMaps(M);
  //printMap(M);
  parseInput("part1_loc.txt", p1_input);
  initializeCallStack(p1_input);
  // TODO : TEMPORARY HACK FOR REAL LIBSAFE
  //corruptedIR.insert(23);
  // TODO : TEMPORARY HACK FOR REAL APACHE-25520
  //corruptedIR.insert(1841);
  part1_getCorruptedIRs(M);
  //parseInput("part2_loc.txt", p2_input);
  //part2_getDominantFrontiers(M);
  //part3_getFeasiblePath(M);

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
        for (int op_i = 0; op_i < i->getNumOperands(); op_i++) {
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

bool ConAnalysis::part1_getCorruptedIRs(Module &M) {
  errs() << "---------------------------------------\n";
  errs() << "       part1_getCorruptedIRs           \n";
  errs() << "---------------------------------------\n";
  while (!callstack_.empty()) {
    auto& loc = callstack_.top();
    errs() << "Outter while loop: Go into " << loc.first->getName() << "\n";
    intra_dataflow_analysis(loc.first, loc.second);
    callstack_.pop();
  }
  errs() << "---------- Part 1 Result --------------\n";
  printSet(corruptedIR_);
  return false;
}

bool ConAnalysis::intra_dataflow_analysis(Function * F, Instruction * ins) {
  bool rv = false;
  auto I = inst_begin(F);
  if (ins != nullptr) {
    for (; I != inst_end(F); ++I) {
      if (&*I == &*ins) {
        // Skip the previous call instruction after returned.
        if (isa<CallInst>(&*I)) {
          ++I;
        }
        corruptedIR_.insert(ins2int_[&*I]);
        break;
      }
    }
    assert(I != inst_end(F) && "Couldn't find callstack instruction.");
  }
  // TODO: Add related library functions into our .bc file.
  //assert(I == inst_end(F) && "Couldn't obtain the source code of function.");
  // When we found the instruction, we pop the stack.
  for (; I != inst_end(F); ++I) {
    for (int op_i = 0, op_num = I->getNumOperands(); op_i < op_num; op_i++) {
      Value * v = I->getOperand(op_i);
      if (isa<Instruction>(v)) {
        if (corruptedIR_.count(ins2int_[cast<Instruction>(v)])) {
          corruptedIR_.insert(ins2int_[&*I]);
          break;
        }
      }
    }
    if (isa<ReturnInst>(&*I)) {
      rv = true;
    } else if (isa<CallInst>(&*I)) {
      CallSite cs(&*I);
      Function * callee = cs.getCalledFunction();
      if (!callee) {
        errs() << "Couldn't get callee for instruction ";
        I->print(errs());
        errs() << "\n";
        continue;
      }
      std::string fnname = callee->getName().str();
      if (fnname.compare(0, 5, "llvm.") == 0)
        continue;
      errs() << F->getName() << " calls " << callee->getName() << "\n";
      intra_dataflow_analysis(callee, nullptr);
    }
  }
  return rv;
}

bool ConAnalysis::part2_getDominantFrontiers(Module &M) {
  //for (auto FuncIter = M.getFunctionList().begin();
      //FuncIter != M.getFunctionList().end(); FuncIter++) {
    //Function *F = FuncIter;
    //std::map<BasicBlock *, std::set<BasicBlock *>> dominators;
    //if (F->getName().str().compare(p2_input.funcName) == 0) {
      //computeDominators(*F, dominators);
      ////printDominators(*F, dominators);
      //auto it = dominators[(p2_input.danOpI)->getParent()].begin();
      //auto it_end = dominators[(p2_input.danOpI)->getParent()].end();
      //for (; it != it_end; ++it) {
        //for (auto i = (*it)->begin(); i != (*it)->end(); ++i) {
          //dominantFrontiers.insert(ins2int_[i]);
          //if (&*i == p2_input.danOpI)
            //break;
        //}
      //}
      //break;
    //}
  //}
  //errs() << "---------- Part 2 Result --------------\n";
  //printSet(dominantFrontiers);
  return false;
}

bool ConAnalysis::part3_getFeasiblePath(Module &M) {
  std::set_intersection(corruptedIR_.begin(), corruptedIR_.end(),
                        dominantfrontiers_.begin(),
                        dominantfrontiers_.end(),
                        std::inserter(feasiblepath_,
                                      feasiblepath_.end()));
  errs() << "---------- Part 3 Result --------------\n";
  printSet(feasiblepath_);
  return false;
}

// TODO: Change to C++11
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
        } else if(std::find(worklist.begin(),
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
