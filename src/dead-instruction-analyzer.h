//
// Created by saikatc on 11/20/20.
//

#ifndef LLVM_LIVENESS_UTIL_H
#define LLVM_LIVENESS_UTIL_H
#include "hw6-util.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"

#include <algorithm>
#include <map>
#include <set>
#include <vector>
using namespace llvm;
using namespace std;

class VariableLivenessUtil {
private:
  Function *function;
  map<Instruction *, set<Value *>> DEF_MAP;
  map<Instruction *, set<Value *>> USE_MAP;
  map<Instruction *, set<Value *>> LIVE_IN_MAP;
  map<Instruction *, set<Value *>> LIVE_OUT_MAP;

public:
  VariableLivenessUtil(Function *f) {
    function = f;
    variableLivenessAnalysis();
  }

  void populateInitialValues(
      vector<Instruction *> &allInstructions,
      map<Instruction *, set<Value *>> &def,
  map<Instruction *, set<Value *>> &use,
  map<Instruction *, set<Value *>> &in,
  map<Instruction *, set<Value *>> &out,
  map<PHINode *, map<BasicBlock *, set<Value *>>> &phiUse,
  map<PHINode *, map<BasicBlock *, set<Value *>>> &phiIn) {
    for (Instruction *pI : allInstructions) {
      Value *p = dyn_cast<Value>(pI);
      set<Value *> s;
      if (!isa<BranchInst>(pI) && !isa<ReturnInst>(pI)) {
        s.insert(p);
      }
      def[pI] = s;
      in[pI] = set<Value *>();
      out[pI] = set<Value *>();

      set<Value *> s2;
      for (auto opnd = pI->op_begin(); opnd != pI->op_end(); ++opnd) {
        Value *val = *opnd;
        if (isa<Instruction>(val) || isa<Argument>(val)) {
          s2.insert(val);
        }
      }
      use[pI] = s2;
      
      // Handle Phi nodes
      if (isa<PHINode>(pI)) {
        PHINode *phiInstruction = dyn_cast<PHINode>(pI);
        map<BasicBlock *, set<Value *>> tempUseMap;
        map<BasicBlock *, set<Value *>> tempInMap;
        for (unsigned int ind = 0; ind < phiInstruction->getNumIncomingValues();
             ind++) {
          set<Value *> tempSet;
          Value *val = phiInstruction->getIncomingValue(ind);
          if (isa<Instruction>(val) || isa<Argument>(val)) {
            BasicBlock *valBlock = phiInstruction->getIncomingBlock(ind);
            if (tempInMap.find(valBlock) == tempInMap.end()) {
              tempInMap[valBlock] = set<Value *>();
            }
            if (tempUseMap.find(valBlock) == tempUseMap.end()) {
              tempSet.insert(val);
              tempUseMap[valBlock] = tempSet;
            } else {
              tempUseMap[valBlock].insert(val);
            }
          }
        }
        phiUse[phiInstruction] = tempUseMap;
        phiIn[phiInstruction] = tempInMap;
      }
    }
  }

  vector<Instruction *> getAllInstructions(){
    vector<Instruction *> allInstructions;
    for (BasicBlock &basicBlocks : *function) {
      for (Instruction &instruction : basicBlocks) {
        Instruction *instPtr = &instruction;
        allInstructions.emplace_back(instPtr);
      }
    }
    return allInstructions;
  }

  map<Instruction *, vector<Instruction *>>
  extractControlFlowGraph(vector<Instruction *> &allInstructions) {
    map<Instruction *, vector<Instruction *>> controlFlowGraph;
    int nIsnt = allInstructions.size();
    for (int i = 0; i < nIsnt; i++) {
      controlFlowGraph[allInstructions[i]] = vector<Instruction *>();
      if (allInstructions[i]->isTerminator()) {
        int nSuccessors = allInstructions[i]->getNumSuccessors();
        for (int j = 0; j < nSuccessors; j++) {
          auto successor = allInstructions[i]->getSuccessor(j);
          controlFlowGraph[allInstructions[i]].push_back(&successor->front());
        }
      } else {
        controlFlowGraph[allInstructions[i]].push_back(allInstructions[i + 1]);
      }
    }
    return controlFlowGraph;
  }

  void variableLivenessAnalysis(){
    vector<Instruction *> allInstructions = getAllInstructions();
    map<Instruction *, vector<Instruction *>> controlFlowGraph =
                           extractControlFlowGraph(allInstructions);
    map<Instruction *, set<Value *>> USE;
    map<Instruction *, set<Value *>> DEF;
    map<PHINode *, map<BasicBlock *, set<Value *>>> PHI_USE;
    map<Instruction *, set<Value *>> LIVE_IN;
    map<Instruction *, set<Value *>> LIVE_OUT;
    map<PHINode *, map<BasicBlock *, set<Value *>>> PHI_IN;
    populateInitialValues(allInstructions, DEF, USE, LIVE_IN,
                          LIVE_OUT, PHI_USE, PHI_IN);
    bool possibleToUpdate = true;
    std::reverse(allInstructions.begin(), allInstructions.end());
    while (possibleToUpdate) {
      possibleToUpdate = false;
      int instSz = allInstructions.size();
      for (int iid = instSz - 1; iid >=0; iid--) {
        Instruction* pI = allInstructions[iid];
        std::set<Value *> newOUT;
        for (auto &sI : controlFlowGraph[pI]) {
          std::set<Value *> temp(LIVE_IN[sI]);
          if (isa<PHINode>(sI)) {
            PHINode *phi_insn = dyn_cast<PHINode>(sI);
            temp = PHI_IN[phi_insn][pI->getParent()];
          }
          std::set<Value *> newOutCopy(newOUT);
          newOUT = setUnion(temp, newOutCopy);
        }
        if (LIVE_OUT[pI] != newOUT) {
          possibleToUpdate = true;
        }
        LIVE_OUT[pI] = newOUT;

        if (!isa<PHINode>(pI)) {
          std::set<Value *> diff = setDifference(LIVE_OUT[pI], DEF[pI]);
          std::set<Value *> newIN = setUnion(USE[pI], diff);
          if (LIVE_IN[pI] != newIN) {
            possibleToUpdate = true;
          }
          LIVE_IN[pI] = newIN;
        }
        else {
          PHINode *phiInsn = dyn_cast<PHINode>(pI);
          for (pair<BasicBlock *, set<Value *>> temp : PHI_IN[phiInsn]) {
            set<Value *> temp_use = PHI_USE[phiInsn][temp.first];
            set<Value *> temp_in = temp.second;
            std::set<Value *> diff = setDifference(LIVE_OUT[pI], DEF[pI]);
            std::set<Value *> newIN = setUnion(temp_use, diff);
            if (temp_in != newIN) {
              possibleToUpdate = true;
            }
            PHI_IN[phiInsn][temp.first] = newIN;
          }
        }
      }
    }
    for(auto instruction : allInstructions){
      if(isa<PHINode>(instruction)){
        auto inMaps = PHI_IN[dyn_cast<PHINode>(instruction)];
        for(auto values : inMaps){
          LIVE_IN[instruction] = setUnion(LIVE_IN[instruction], values.second);
        }
      }
    }
    DEF_MAP = DEF;
    USE_MAP = USE;
    LIVE_IN_MAP = LIVE_IN;
    LIVE_OUT_MAP = LIVE_OUT;
  }

  template <typename T> static set<T> setUnion(set<T> &a, set<T> &b) {
    set<T> result;
    std::set_union(a.begin(), a.end(), b.begin(), b.end(),
                   std::inserter(result, result.begin()));
    return result;
  }

  template <typename T> static set<T> setIntersection(set<T> &a, set<T> &b) {
    set<T> result;
    std::set_intersection(a.begin(), a.end(), b.begin(), b.end(),
                          std::inserter(result, result.begin()));
    return result;
  }

  template <typename T> static set<T> setDifference(set<T> &a, set<T> &b) {
    set<T> result;
    std::set_difference(a.begin(), a.end(), b.begin(), b.end(),
                        std::inserter(result, result.begin()));
    return result;
  }

  bool isDeadInstruction(Instruction* &inst);

  void removeDeadInstructions(OptimizationResultWriter &writer);
};

#endif // LLVM_LIVENESS_UTIL_H
