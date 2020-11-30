#include "dead-instruction-analyzer.h"
#include "hw6-util.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"

#include <algorithm>
#include <map>
#include <set>
#include <stack>
#include <vector>

using namespace llvm;
using namespace std;

struct HW6Optimizer : public ModulePass {
  static char ID; // Pass identification, replacement for typeid
  HW6Optimizer() : ModulePass(ID) {}

  /*
   * We are assuming that "main" function is the entry point of a module.
   * This functions extracts the function pointer that is named "main".
   */
  Function *extractEntryFunction(vector<Function *> &allFunctions) {
    for (auto function : allFunctions) {
      if (function->getName().str() == "main") {
        return function;
      }
    }
    return nullptr;
  }

  /*
   * Function for extracting call graphs.
   * Given a list of all functions, we iterate over the body of the function.
   * And check for `CallInst`. We extract the callee from a CallInst.
   * When A callee is found, we add an edge from caller to callee.
   */
  map<Function *, vector<Function *>>
  getCallGraph(vector<Function *> &allFunctions) {
    map<Function *, vector<Function *>> callGraph;
    for (Function *f : allFunctions) {
      vector<Function *> callee;
      for (BasicBlock &basicBlocks : *f) {
        for (Instruction &instruction : basicBlocks) {
          Instruction *instPtr = &instruction;
          if (isa<CallInst>(instPtr)) {
            CallInst *callInst = dyn_cast<CallInst>(instPtr);
            callee.push_back(callInst->getCalledFunction());
          }
        }
      }
      callGraph[f] = callee;
    }
    return callGraph;
  }


  /*
   * Extract list of unused functions, given the callGraph and
   * the Function * entryFunction.
   */
  vector<Function *> getUnusedFunction(vector<Function *> &allFunctions,
                    map<Function *, vector<Function *>> &callGraph,
                    Function *entryFunction) {
    vector<Function *> unused;
    /*
     * TODO : Extract the unused functions. If a function is unreachable from
     * The entryFunction, that function will be deemed unused.
     * You have to extract all such unused functions and put those in the
     * Vector `unused`.
     */

    return unused;
  }

  void removeUnusedFunction(Module &M, vector<Function *> &unusedFunctions) {
    /*
     * TODO: Remove all the unused functions from Module M.
     * Remember, unused functions are function that cannot be reached from
     * The entryFunction. That doesn't mean no other function will call one
     * Of those unused functions. If you do not take necessary
     * Steps before removing such a function, your optimizer might crash.
     * TAs will provide no further hint about what necessary steps you should
     * Take and how to remove a function. You should do research on necessary
     * APIs for implementing this function.
     */

  }

  bool runOnModule(Module &M) override {
    OptimizationResultWriter writer(M);
    vector<Function *> allFunctions;
    for (Function &F : M) {
      allFunctions.push_back(&F);
    }
    map<Function *, vector<Function *>> callGraph = getCallGraph(allFunctions);
    writer.printCallGraph(callGraph);
    Function *entryFunction = extractEntryFunction(allFunctions);
    if (entryFunction != nullptr) {
      vector<Function *> unusedFunstions =
          getUnusedFunction(allFunctions, callGraph, entryFunction);
      writer.printUnusedFunctions(unusedFunstions);
      removeUnusedFunction(M, unusedFunstions);
      for (auto function : allFunctions) {
        if (std::find(unusedFunstions.begin(), unusedFunstions.end(),
                      function) == unusedFunstions.end()) {
          VariableLivenessUtil instructionAnalyzer(function);
          instructionAnalyzer.removeUnused(writer);
        }
      }
      writer.writeModifiedModule(M);
    }
    return true;
  }
};

char HW6Optimizer::ID = 0;
static RegisterPass<HW6Optimizer> X("optimize", "Optimization Pass");
