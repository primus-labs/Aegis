#ifndef DIALECT_FHE_FHEOPS_TD_
#define DIALECT_FHE_FHETOPS_TD_

include "FHEDialect.td"
include "FHETypes.td"
include "mlir/IR/BuiltinAttributeInterfaces.td"
include "mlir/Interfaces/ControlFlowInterfaces.td"
include "mlir/Interfaces/InferTypeOpInterface.td"
include "mlir/Interfaces/SideEffectInterfaces.td"

// Base class for toy dialect operations. This operation inherits from the base
// `Op` class in OpBase.td, and provides:
//   * The parent dialect of the operation.
//   * The mnemonic for the operation, or the name without the dialect prefix.
//   * A list of traits for the operation.
class FHE_Op<string mnemonic, list<Trait> traits = []> :
    Op<FHE_Dialect, mnemonic, !listconcat(traits, [])> {
    let cppNamespace = "::mlir::aegis::secret";
  
    let assemblyFormat = [{`(` operands `)` attr-dict `:`  `(` type(operands) `)` `->` type(results)}];
}

#endif
