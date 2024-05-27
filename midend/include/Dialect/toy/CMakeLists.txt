# Most dialects should use add_mlir_dialect().  See examples/standalone.
# set(LLVM_TARGET_DEFINITIONS Ops.td)
# mlir_tablegen(toyOps.h.inc -gen-op-decls)
# mlir_tablegen(toyOps.cpp.inc -gen-op-defs)
# mlir_tablegen(toyDialect.h.inc -gen-dialect-decls)
# mlir_tablegen(toyDialect.cpp.inc -gen-dialect-defs)

# add_public_tablegen_target(midend-ToyOpsIncGen)

# Most dialects should use add_mlir_interfaces().
set(LLVM_TARGET_DEFINITIONS toyShapeInferenceInterface.td)
mlir_tablegen(toyShapeInferenceOpInterfaces.h.inc -gen-op-interface-decls)
mlir_tablegen(toyShapeInferenceOpInterfaces.cpp.inc -gen-op-interface-defs)

set(LLVM_TARGET_DEFINITIONS ToyCombine.td)
mlir_tablegen(ToyCombine.inc -gen-rewriters)

add_mlir_dialect(toyOps toy)