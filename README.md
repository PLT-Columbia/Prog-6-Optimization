# COMS W4115: Programming Assignment 6 (Optimization)

## Course Summary

Course: COMS 4115 Programming Languages and Translators (Fall 2020)  
Website: https://www.rayb.info/fall2020  
University: Columbia University.  
Instructor: Prof. Baishakhi Ray


## Logistics
* **Announcement Date:** Wednesday, December 2, 2020
* **Due Date:** Wednesday, December 22, 2020 by 11:59 PM.
* **Total Points:** 100


## Assignment Objectives

From this assignment:

1. You **will learn** how to write a **Transformation Pass** and a **Module Pass** in LLVM.
2. You **will learn** how to apply **optimizations for control flow analysis**.
3. You **will learn** how to apply **optimizations for data flow analysis**. 

## Grading Breakdown
* **Task 1 (Identification of Dead Functions)**: 50
* **Task 2 (Removal of Dead Functions)**: 25
* **Task 3 (Identification of Dead Instructions)**: 25


## Assignment

In class and in previous programming assignments, we learned about control flow analysis and data flow analysis. In this assignment, we will use such analysis techniques to optimize code. Particularly in this assignment, we want to remove unnecessary code from an IR. We divide the assignment into two parts:

1. Dead function elimination
2. Dead instruction elimination

### Background

#### Dead Function Elimination
Often, we want to determine whether or not it is possible for a specific function to be **executed** in the code. In order to do that, we must first assume that every program will have an entry function (in C, this is typically `main`). Consider the following program:

```c++
 1. int add(int n, int m) {
 2.     return n + m;
 3. }
 4. int mult(int m, int n) {
 5.     int res = 0, t = 5, x = m * n;
 6.     while(res != x) {
 7.         res = res + m;
 8.     }
 9.     return res;
10. }
11. int fact(int n) {
12.     if (n == 0) return 1;
13.     else {
14.         return mult(n, fact(add(n, -1)));
15.     }
16. }
17. int main() {
18.     int n = 9, m = 8;
19.     print(mult(m, n));
20.     return 0;
21. }
```  

Here, the entry function is `main`. Among the other functions, `fact` and `add` are never executed when starting from `main`. Thus, our objective for this optimization is to remove such functions that are never used or considered *dead* in the code.

To begin identifying dead functions, we need to analyze a [call graph](https://en.wikipedia.org/wiki/Call_graph). A call graph is a graph that shows the relationships among direct function calls in a program. Every node of a call graph represents a function call, and an edge from a node `A` to a node `B` in the call graph indicates that function `B` is invoked by function `A`, *i.e.*, a function call to `B` is made inside of function `A`'s body. Note that analyzing indirect function calls through function pointers requires a more sophisticated analysis technique, so for simplicity, assume that all function calls are direct function calls.

Once we identify all functions that are never called starting from `main`, our objective is to them remove those from the code.

#### Dead Instruction Elimination
For this part of the assignment, we will do more micro-level optimization. In particular, we will eliminate any instructions that define variables that are never used later; we refer to such instructions as *dead* instructions (No surprises!). Consider the following IR code:

```ll
 1. define dso_local i32 @mult(i32 %m, i32 %n) #0 {
 2. entry:
 3.   %add = add nsw i32 %m, %n
 4.   %mul = mul nsw i32 %m, %n
 5.   br label %while.cond
 6. while.cond:               
 7.   %res.0 = phi i32 [ 0, %entry ], [ %add1, %while.body ]
 8.   %cmp = icmp ne i32 %res.0, %mul
 9.   br i1 %cmp, label %while.body, label %while.end
10. while.body:                                      
11.   %add1 = add nsw i32 %res.0, %m
12.   br label %while.cond
13. while.end:
14.   ret i32 %res.0
15. }
``` 
Note that, in the code above, there is no control path that uses `%add`. Thus, the instruction that defined `%add` (line 3) does not have any impact on the rest of the code. This instruction is an example of a dead instruction, and we can safely remove it from our IR. As you can imagine with dead functions, our objective for this optimization is to remove such instructions. *Hint: to analyze whether a defined variable is used later in the code, you can revisit the concepts of `LIVE_IN`/`LIVE_OUT` sets from Programming Assignment 5.*


### Getting Started

1. Convert the `example.c` C program (from the `examples` directory) to an IR by running the following, just as you did in the previous assignment:

```
export LLVM_HOME="<the absolute path to llvm-project>";
export PATH="$LLVM_HOME/build/bin:$PATH";

clang -O0 -Xclang -disable-O0-optnone -emit-llvm -c example.c
llvm-dis example.bc
```
You have now generated an `example.bc` file, which contains the IR in binary format. You will also see an `example.ll` file, which contains the IR in human-readable format.

2. Convert the `example.bc` file to single static assignment form (this is very important for generating suitable inputs, so please do not forget to perform this step):

```
opt -mem2reg example.bc -o ssa.bc
llvm-dis ssa.bc
```

Note that the `ssa.ll` file that is created will be the input for this assignment.

3. Create a directory `clang-hw6` in `$LLVM_HOME/llvm/lib/Transforms` for this assignment, and copy the files from the `src` directory to this new directory, as follows:

```
cp -r ./src/* "$LLVM_HOME/llvm/lib/Transforms/clang-hw6/"
```

4. Append `add_subdirectory(clang-hw6)` to the `$LLVM_HOME/llvm/lib/Transforms/CMakeLists.txt` file.

5. Build `clang-hw6` by running the following commands:

```
cd "$LLVM_HOME/build"
make
```

After you successfully run `make` once, you can rebuild the project using `make LLVMOptimizer`.

6. Whenever you are running the pass for this assignment, please run the following command:

```
opt -load $LLVM_HOME/build/lib/LLVMOptimizer.so -optimize ssa.bc
```

Read through the output you see in the terminal for additional hints. Additionally, take a look at the different files generated by the pass. 

We are implementing a [`ModulePass`](https://llvm.org/doxygen/classllvm_1_1ModulePass.html) in this assignment. [`runOnModule`](src/hw6-optimizer.cpp#L95) is the entry point for the pass, much like `runOnFunction` was the entry point for the `FunctionPass`. In `runOnModule` function, we extracted for you [`allFunctions`](src/hw6-optimizer.cpp#L97-L100) (a vector of all the functions), [`callGraph`](src/hw6-optimizer.cpp#L101) (the call graph), and [entryFunction](src/hw6-optimizer.cpp#L103) (the entry function) structures. We have also taken care of all the inputs and outputs.

### Task 1: Identification of Dead Functions (50 points)
In this task, you will implement the [`getDeadFunctions`](src/hw6-optimizer.cpp#L67) function and return a vector of `Function *` denoting the unused/dead functions (_i.e._, functions that are not reachable from `main`). This function takes in a `vector<Function *>` containing all functions in the module, `map<Function *, vector<Function *>>` containing the call graph, and a `Function *` indication the pointer to the entry function. The `TODO` comment also summarizes what needs to be done.

### Task 2: Removal of Dead Functions (25 points)
Once you find all the dead functions, you will implement the [`removeDeadFunctions`](src/hw6-optimizer.cpp#L81) function to actually perform the removal of all these dead functions. Note that our definition of "dead" _does NOT mean_ a function is not called from anywhere in the code. It simply refers to the fact that a function is not reachable from `main`. Thus, while you are removing a dead function, make sure that you properly update any possible call sites of that function. Given this hint, you will probably need to do some research to find suitable APIs to accomplish this task.

#### Brain Teaser
We also did analysis of ASTs based on function calls back in Programming Assignment 2. Tasks 1 and 2 can easily be done with AST analysis (in the front-end). What are the pros and cons of doing this analysis in the compiler back-end? In other words, what benefit does performing the analysis/transformation on the LLVM IR provide in comparison to the analysis on the AST?

### Task 3: Identification of Dead Instructions (25 points)
Once we remove all dead functions, you will analyze the body of all remaining functions to identify which instructions are dead. In order to do that, for every function, we instantiate an analyzer to extract data flow variables (_i.e._, `DEF`, `USE`, `LIVE_IN`, and `LIVE_OUT`). We call the [`VariableLivenessUtil::removeDeadInstructions`](src/dead-instruction-analyzer.cpp#L43) function to check for and remove any dead instructions. Your task is to decide whether an instruction should or should not be removed.

You will need to implement the [`VariableLivenessUtil::isDeadInstruction`](src/dead-instruction-analyzer.cpp#L21) function, which decides whether an instruction should or should not be removed. You do not have to actually remove the instruction; we have already done that for you, as indicated above. Please read through the comments in the code for further instructions and hints.   

### Important Notes
1. Please **DO NOT** remove or modify any of the existing code.
2. Place all of your code in the sections that we have outlined for you. You may include helper functions as necessary, but please make sure to put them inside the [`src/hw6-optimizer.cpp`](src/hw6-optimizer.cpp) or [`src/dead-instruction-analyzer.cpp`](src/dead-instruction-analyzer.cpp) files as necessary. Keep in mind that we will only use these two files from your submission for grading. 
3. Carefully read through the steps highlighted in this README, as well as the TODOs and other comments in the code. You will find useful hints.
4. Once you successfully run the pass, you will notice that the `ssa-modified.ll` file should reflect all of your optimizations. Compare your original `ssa.ll` file with the `ssa-modified.ll` file to see the output of your first compiler optimizations. _Trust me! It feels amazing!_

## Submission
We will only consider [`src/hw6-optimizer.cpp`](src/hw6-optimizer.cpp) and [`src/dead-instruction-analyzer.cpp`](src/dead-instruction-analyzer.cpp) from your submission. Please make sure all relevant code is **only** in these two files. Make sure to properly commit your code and push it to the `main` branch.

## Piazza
If you have any questions about this programming assignment, please post them in the Piazza forum for the course, and an instructor will reply to them as soon as possible. Any updates to the assignment itself will be available in Piazza.

## Disclaimer
This assignment belongs to Columbia University. It may be freely used for educational purposes.
