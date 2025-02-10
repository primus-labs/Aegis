#ifndef LIB_DIALECT_FHE_DIALECT_IR_TD_
#define LIB_DIALECT_FHE_DIALECT_IR_TD_


include "mlir/IR/DialectBase.td"
include "mlir/IR/OpBase.td"


def FHE_Dialect : Dialect {
  let name = "fhe";

  let description = [{
      This dialect represents a common fhe abstraction for all RLWE/LWE operations.
  }];

  let cppNamespace = "::mlir::aegis::fhe";

  let useDefaultTypePrinterParser = 1;
}

#endif 