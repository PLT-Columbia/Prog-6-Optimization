//
// Created by saikatc on 11/20/20.
//
#include "dead-instruction-analyzer.h"
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

bool VariableLivenessUtil::isDeadInstruction(Instruction* &inst) {
  set<Value *> DEF = DEF_MAP[inst];
  set<Value *> USE = USE_MAP[inst];
  set<Value *> LIVE_IN = LIVE_IN_MAP[inst];
  set<Value *> LIVE_OUT = LIVE_OUT_MAP[inst];
  bool deadInstruction = false;
  /*
   * TODO: Analyze whether an instruction (given by Instruction* inst)
   *       is dead or not. A dead instruction defines a variable that is
   *       not used by any other part of the code.
   */

  return deadInstruction;
}

/*
 * We call this function to analyze whether any instruction in a function is
 * dead or not. We collect all such instructions and remove them from their
 * parent. Note that, since these instructions are defining variables that 
 * are unused by any other part of the code, we DO NOT have to update the
 * uses (in fact, there are none).
 */
void VariableLivenessUtil::removeUnused(OptimizationResultWriter &writer) {
  vector<Instruction *> deadInstructions;
  for(auto ins_it = inst_begin(function); ins_it != inst_end(function); ins_it++){
      Instruction *instPtr = &(*ins_it);
      if (isDeadInstruction(instPtr)) {
        if(!isa<BranchInst>(instPtr) && !isa<ReturnInst>(instPtr) &&
            !isa<CallInst>(instPtr)) {
          deadInstructions.push_back(instPtr);
        }
      }
  }
  if(!deadInstructions.empty()) {
    writer.printUnusedInstruction(function, deadInstructions);
    for (auto instr : deadInstructions) {
      instr->eraseFromParent();
    }
  }
}
