add_llvm_library(LLVMOptimizer MODULE BUILDTREE_ONLY
        hw6-optimizer.cpp hw6-util.h tee.h dead-instruction-analyzer.h dead-instruction-analyzer.cpp
        DEPENDS
        intrinsics_gen
        PLUGIN_TOOL
        opt
)
