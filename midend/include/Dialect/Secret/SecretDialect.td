#ifndef LIB_DIALECT_SECRET_DIALECT_IR_TD_
#define LIB_DIALECT_SECRET_DIALECT_IR_TD_


include "mlir/IR/DialectBase.td"
include "mlir/IR/OpBase.td"

def Secret_Dialect : Dialect {
  let name = "secret";
  let description = [{
    Secret is a dialect for computations that operate on encrypted data.
    Secret is intended to serve as a scheme-agnostic front-end for the Aegis
    ecosystem of dialects. 
  }];


  let extraClassDeclaration = [{
    /// Name of the attribute indicate whether an argument of a function is a secret.
    constexpr const static ::llvm::StringLiteral
        kArgSecretAttrName = "secret.secret";
  }];

  let cppNamespace = "::mlir::aegis::secret";
  let useDefaultTypePrinterParser = 1;
}


#endif
