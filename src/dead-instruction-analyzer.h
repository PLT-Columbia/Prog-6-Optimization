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
    analyzeVariableLiveness();
  }

  void analyzeVariableLiveness() {
    map<Instruction *, set<int>> USE;
    map<Instruction *, set<int>> DEF;
    map<Instruction *, set<int>> LIVE_IN;
    map<Instruction *, set<int>> LIVE_OUT;
    map<Instruction *, set<int>>::iterator it;
    map<Instruction *, set<int>>::iterator kit;
    map<PHINode *, map<BasicBlock *, set<int>>> PHI_USE;
    map<PHINode *, map<BasicBlock *, set<int>>> PHI_IN;
    map<Value *, int> value2int;
    map<int, Value *> int2value;
    Function &F = *function;
    for (inst_iterator II = inst_begin(F); II != inst_end(F); ++II) {
      Instruction &insn(*II);
      for (User::op_iterator OI = insn.op_begin(), OE = insn.op_end(); OI != OE;
           ++OI) {
        Value *val = *OI;
        if (isa<Instruction>(val) || isa<Argument>(val)) {
          if (value2int.find(val) == value2int.end()) {
            value2int.insert(pair<Value *, int>(val, value2int.size()));
            int2value.insert(pair<int, Value *>(int2value.size(), val));
          }
        }
      }
    }

    for (BasicBlock &BB : F) {
      for (Instruction &I : BB) {
        Instruction *pI = &I;
        Value *p = cast<Value>(pI);
        set<int> s;
        if (!isa<BranchInst>(pI) && !isa<ReturnInst>(pI) &&
            !isa<CallInst>(pI)) {
          s.insert(value2int[p]);
        }
        DEF.insert({&I, s});
        LIVE_IN.insert({&I, set<int>()});
        LIVE_OUT.insert({&I, set<int>()});

        set<int> s2;
        for (User::op_iterator opnd = I.op_begin(), opE = I.op_end();
             opnd != opE; ++opnd) {
          Value *val = *opnd;
          if (isa<Instruction>(val) || isa<Argument>(val)) {
            s2.insert(value2int[val]);
          }
        }
        USE.insert({&I, s2});

        // handle PHI NODE
        if (PHINode *phi_insn = dyn_cast<PHINode>(&I)) {
          map<BasicBlock *, set<int>> temp_use_map;
          map<BasicBlock *, set<int>> temp_in_map;
          int sz = phi_insn->getNumIncomingValues();
          for (int ind = 0; ind < sz; ind++) {
            set<int> temp_set;
            Value *val = phi_insn->getIncomingValue(ind);
            if (isa<Instruction>(val) || isa<Argument>(val)) {
              BasicBlock *valBlock = phi_insn->getIncomingBlock(ind);
              if (temp_in_map.find(valBlock) == temp_in_map.end()) {
                temp_in_map.insert({valBlock, set<int>()});
              }
              if (temp_use_map.find(valBlock) == temp_use_map.end()) {
                temp_set.insert(value2int[val]);
                temp_use_map.insert({valBlock, temp_set});
              } else {
                temp_use_map[valBlock].insert(value2int[val]);
              }
            }
          }
          PHI_USE.insert({phi_insn, temp_use_map});
          PHI_IN.insert({phi_insn, temp_in_map});
        }
      }
    }

    map<Instruction *, vector<Instruction *>> SuccessorMap;
    Instruction *preI = nullptr;
    vector<Instruction *> all_instructions;
    for (BasicBlock &BB : F) {
      const Instruction *TInst = BB.getTerminator();
      for (Instruction &I : BB) {
        Instruction *pI = &I;
        all_instructions.push_back(pI);
        if (preI != nullptr) {
          vector<Instruction *> successors;
          if (preI == TInst) {
            for (int i = 0, NSucc = TInst->getNumSuccessors(); i < NSucc; i++) {
              BasicBlock *succ = TInst->getSuccessor(i);
              successors.push_back(&succ->front());
            }
          } else {
            successors.push_back(pI);
            SuccessorMap.insert({preI, successors});
          }
        }
        preI = pI;
      }
    }

    std::reverse(all_instructions.begin(), all_instructions.end());
    bool exist_update = true;
    while (exist_update) {
      exist_update = false;
      for (auto &pI : all_instructions) {
        // in[n] = use[n] ∪ (out[n] – def[n])
        Instruction &I(*pI);
        if (!isa<PHINode>(&I)) {
          std::set<int> diff = setDifference(LIVE_OUT[pI], DEF[pI]);
          std::set<int> newIN = setUnion(USE[pI], diff);
          if (LIVE_IN[pI] != newIN) {
            exist_update = true;
          }
          LIVE_IN[pI] = newIN;
        } else {
          PHINode *phi_insn = dyn_cast<PHINode>(&I);
          for (pair<BasicBlock *, set<int>> temp : PHI_IN[phi_insn]) {
            set<int> temp_use = PHI_USE[phi_insn][temp.first];
            set<int> temp_in = temp.second;
            std::set<int> diff = setDifference(LIVE_OUT[pI], DEF[pI]);
            std::set<int> newIN = setUnion(temp_use, diff);
            if (temp_in != newIN) {
              exist_update = true;
            }
            PHI_IN[phi_insn][temp.first] = newIN;
          }
        }
        // out[n] = ∪(s in succ) {in[s]}
        std::set<int> newOUT;
        for (auto &sI : SuccessorMap[pI]) {
          std::set<int> temp(LIVE_IN[sI]);
          if (isa<PHINode>(&*sI)) {
            PHINode *phi_insn = dyn_cast<PHINode>(&*sI);
            temp = PHI_IN[phi_insn][pI->getParent()];
          }
          std::set<int> temp2(newOUT);
          newOUT = setUnion(temp, temp2);
        }
        if (LIVE_OUT[pI] != newOUT) {
          exist_update = true;
        }
        LIVE_OUT[pI] = newOUT;
      }
    }
    for(auto inst : DEF){
      vector<Value *>defs = vector<Value *>();
      for(auto vi : inst.second){
        defs.push_back(int2value[vi]);
      }
      DEF_MAP[inst.first] = set<Value *>(defs.begin(), defs.end());
    }
    for(auto inst : LIVE_OUT){
      vector<Value *>louts = vector<Value *>();
      for(auto vi : inst.second){
        louts.push_back(int2value[vi]);
      }
      LIVE_OUT_MAP[inst.first] = set<Value *>(louts.begin(), louts.end());
    }

    for(auto inst : USE){
      vector<Value *>uses = vector<Value *>();
      for(auto vi : inst.second){
        uses.push_back(int2value[vi]);
      }
      USE_MAP[inst.first] = set<Value *>(uses.begin(), uses.end());
    }
    for(auto inst : LIVE_IN){
      vector<Value *>lins = vector<Value *>();
      for(auto vi : inst.second){
        lins.push_back(int2value[vi]);
      }
      LIVE_IN_MAP[inst.first] = set<Value *>(lins.begin(), lins.end());
    }
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

  void removeUnused(OptimizationResultWriter &writer);
};

#endif // LLVM_LIVENESS_UTIL_H
