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

1. You **will learn** how to write **Transformation Pass** and **Module Pass** in LLVM.
2. You **will learn** how to use **Control Flow Analysis based optimization**. 
3. You **will learn** how to use **Data Flow Analysis based optimization**. 

## Grading Breakdown
* Identification of Unreachable Function - 50
* Removal of Unreachable Functions - 25
* Identification of Unused instruction - 25


## Assignment

In class, and in the previous programming assignments, we learned control flow analysis, and data flow analysis. 
In this assignment,  we will use such analyses for optimizing code. In particular, in this assignment, 
we want to remove unnecessary code from a IR file. We divide the assignment in 2 parts,

1. Unused Functions Elimination.
2. Unused Instructions Elimination.

#### Unused function elimination.
We have to determine whether there is a possibility of a function to be **executed** in the code. 
In order for doing that, we assume that every code will have an entry function (typically in c it is `main`). 
Consider the following code, 

```c++
 1. int add(int n, int m){
 2.     return n + m;
 3. }
 4. int mult(int m, int n){
 5.     int res = 0;
 6.     for(int i = 0; i < n; i++){
 7.         res = res + m;
 8.     }
 9. }
10. int fact(int n){
11.     if (n == 0) return 1;
12.     else {
13.         return mult(n, fact(add(n, -1)));
14.     }
15. }
16. int main(){
17.     int n = 9, m = 8;
18.     print(mult(m, n));
19.     return 0;
20. }
```  

Here, the entry function is `main`. Among the other functions, `fact` and `add` are never executed, 
when you start from `main`. Thus, our goal in this optimization is to remove such functions. 

In order for doing that, we have the analyze the [Call Graph](https://en.wikipedia.org/wiki/Call_graph). 
A Call Graph shows the interaction between functions in terms of function calls. 
Every node of the Call Graph is a function, and an edge from node `A` to node `B` indicates function `B` is called
from the body of function `A`.

Once we find out all such functions that are never called starting from `main`, 
our goal is to remove those from the module (_i.e._, file).

#### Unused Instructions Elimination.
For this part of the assignment, we will do more micro level optimization. In particular, we will eliminate any instruction
which generates/defined a value that is never used. Consider the following IR code. 
```asm
 1. define dso_local i32 @main() #0 {
 2. entry:
 3.   %0 = load i32, i32* @x, align 4
 4.   call void @foo(i32 %0)
 5.   %1 = load i32, i32* @x, align 4
 6.   %cmp = icmp eq i32 %1, 1
 7.   br i1 %cmp, label %if.then, label %if.end
 8. if.then:      
 9.   %2 = load i32, i32* @g, align 4
10.   br label %if.end
11. if.end:      
12.   %3 = load i32, i32* @x, align 4
13.   %4 = load i32, i32* @h, align 4
14.   ret i32 %3
15. }
``` 
Note that, in the code above, there is no control path that used `%2` or `%4`. 
Thus, the instructions that defined those values (line 9 for `%2`, and line 13 for `%4`) do not 
have any impact on the code. We can safely remove such instruction. 
Our goal in this assignment is to remove such instructions. To analyze whether a defined value is used
later in the code, you can revisit the concept of `LIVE_IN`/`LIVE_OUT`  set from the Programmins Assignment - 5.

### Getting Started

1. Convert the `example.c` C program (from the `examples` directory) to an IR by running the following, just as you did in the previous assignment:
```
export LLVM_HOME="<the absolute path to llvm-project>";
export PATH="$LLVM_HOME/build/bin:$PATH";

clang -O0 -Xclang -disable-O0-optnone -emit-llvm -c example.c
llvm-dis example.bc
```
You have now generated an `example.bc` file, which contains the IR in binary format. You will also see an `example.ll` file, 
which contains the IR in human-readable format.

2. Convert the `example.bc` file to Single Static Assignment format 
(this is very important for generating suitable input, please do not forget to do this)
```
opt -mem2reg example.bc -o ssa.bc;
llvm-dis ssa.bc
```
this `ssa.ll` file is the input for this assignment.

3. Create a directory `clang-hw6` in `$LLVM_HOME/llvm/lib/Transforms` for this assignment, 
and copy the files from the `src` directory to this new directory, as follows:

```
cp -r ./src/* "$LLVM_HOME/llvm/lib/Transforms/clang-hw6/"
```

4. Append `add_subdirectory(clang-hw6)` to the `$LLVM_HOME/llvm/lib/Transforms/CMakeLists.txt` file.

5. Build `clang-hw6` by running the following commands (you should do this every time you make changes):

```
cd "$LLVM_HOME/build"
make
```

After you successfully run `make` once, you can rebuild the project using `make LLVMOptimizer`.

6. Whenever you are running the pass for this assignment,
```
opt -load $LLVM_HOME/build/lib/LLVMOptimizer.so -optimize < ssa.bc
```
Read through the output you can see in the terminal for additional hints. 

We are implementing a [`ModulePass`](https://llvm.org/doxygen/classllvm_1_1ModulePass.html)
for this assignment. [`runOnModule`](src/hw6-optimizer.cpp#L95) is the entry point for the pass. 
In that function, we extracted a [`vector` of all the function](src/hw6-optimizer.cpp#L97-L100), 
the [`Call graph`](src/hw6-optimizer.cpp#L101), and [the entry function](src/hw6-optimizer.cpp#L103). 
We also have taken care of all the inputs and outputs. 
2.  

### Task-1. Identification of Unreachable Function (50 points)
In this task, you have to implement the [`getUnusedFunction`](src/hw6-optimizer.cpp#L67), and return a vector
of `Function *` denoting the unused function (_i.e._ functions that are not reachable from main). 
The function takes a `vector<Function *>` containing all functions in the module, 
`map<Function *, vector<Function *>>` containing the call graph, and a `Function *` indication the pointer to
entry function. The `TODO` comment also summarizes what is to be done.

### Task-2. Removal of Unreachable Functions (25 points)
Once you find unused functions, you need to implement [`removeUnusedFunctions`](src/hw6-optimizer.cpp#L81) 
to remove all such unused function. Note that, our definition of **Unused** _doesn't mean_ a function is 
not called from anywhere in the code. It simply mean a function is not reachable from `main`. Thus, while
removing a function, make sure to update any possible call site of an unused function. Given this hint, 
you have to research for suitable api for doing the job.

### Task-3. Identification of Unused instruction (25 point)
Once we remove all unused functions, we analyze the body of function to identify which instructions are used
which are not according to our definition above. In order for doing that, for every function, we instantiate 
an analyzer to extract data flow variables (_i.e._ `DEF`, `USE`, `LIVE_IN`, `LIVE_OUT`). 
We call [`VariableLivenessUtil::removeUnused`](src/dead-instruction-analyzer.cpp#L42) to check and remove any
unused instructions. Your job for this assignment is to decide whether an instruction should be removed or not.
You have to implement [`VariableLivenessUtil::isDeadInstruction`](src/dead-instruction-analyzer.cpp#L21) which 
decides whether an instruction should be removed. You do not have to actually remove the instruction, we had
already done that for you.  
Please read through the comments in the code for further instructions and hints.   

### Important Notes
1. Please **DO NOT** remove or modify any of the existing code.
2. Place all of your code in the sections that we have outlined for you. You may include helper functions 
as necessary, but please make sure to put them inside [`src/hw6-optimizer.cpp`](src/hw6-optimizer.cpp), or 
[`src/dead-instruction-analyzer.cpp`](src/dead-instruction-analyzer.cpp) file. 
Keep in mind that we will only use these two files from your submission for grading. 
3. Carefully read through the steps highlighted in this README, as well as the TODOs and other 
comments in the code. You will find useful hints.
4. Once you successfully run the pass, you will see `ssa-modified.ll` file reflecting all your 
optimization. Compare your original `ssa.ll` and `ssa-modified.ll` file to see the output
of your first compiler optimization. _Trust me! it feels amazing!_

## Submission
We will only consider [`src/hw6-optimizer.cpp`](src/hw6-optimizer.cpp), and 
[`src/dead-instruction-analyzer.cpp`](src/dead-instruction-analyzer.cpp) from your submission. Please make
sure all the relevant code are **only** in these two files. 

## Piazza
If you have any questions about this programming assignment, please post them in the Piazza forum for the course, and an instructor will reply to them as soon as possible. Any updates to the assignment itself will be available in Piazza.

## Disclaimer
This assignment belongs to Columbia University. It may be freely used for educational purposes.
