#ifndef LIB_DIALECT_SECRET_SECRETTYPES_TD_
#define LIB_DIALECT_SECRET_SECRETTYPES_TD_

include "SecretDialect.td"

include "mlir/IR/DialectBase.td"
include "mlir/IR/AttrTypeBase.td"


class Secret_Type<string name, string typeMnemonic>
        : TypeDef<Secret_Dialect, name> {
  let mnemonic = typeMnemonic;
}

def Secret : Secret_Type<"Secret", "secret"> {
  let summary = "A secret value";

  let description = [{
    A generic wrapper around another MLIR type, representing an encrypted value
    but not specifying the manner of encryption. 
  }];

  let parameters = (ins "Type":$valueType);
  let assemblyFormat = "`<` $valueType `>`";
}


def BatchedSecret : TypeDef<Secret_Dialect, "BatchedSecret">  {
  let summary = "Batched Secret";

  let description = [{
    A generic wrapper around another MLIR type, representing an batch encrypted value
    but not specifying the manner of encryption. 
  }];

  // Types are written as "!<dialect_name>.<type_mnemonic>" in textal MLIR representation
  let mnemonic = "batched_secret";


  let parameters = (ins
      "Type":$plaintextType,
      "int":$size
  );

  let extraClassDeclaration = [{
        SecretType getCorrespondingSecretType() const;
        static BatchedSecretType get(::mlir::MLIRContext *context, ::mlir::Type plaintextType);
  }];

  let assemblyFormat = "`<` $size `x` $plaintextType `>`";
}


#endif
