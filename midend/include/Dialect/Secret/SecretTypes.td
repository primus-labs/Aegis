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

  let parameters = (ins "Type":$plaintextType);
  let assemblyFormat = "`<` $plaintextType `>`";
}


def SecretVector : TypeDef<Secret_Dialect, "SecretVector">  {
  let summary = "Batched Secret";

  let description = [{
    A generic wrapper around another MLIR type, representing an batch(vector) encrypted value
    but not specifying the manner of encryption. 
  }];

  // Types are written as "!<dialect_name>.<type_mnemonic>" in textal MLIR representation
  let mnemonic = "secret_vector";


  let parameters = (ins
      "Type":$plaintextType,
      "int":$size
  );

  //let extraClassDeclaration = [{
  //      SecretType getCorrespondingSecretType() const;
  //      static SecretVectorType get(::mlir::MLIRContext *context, ::mlir::Type plaintextType);
  //}];

  let assemblyFormat = "`<` $size `x` $plaintextType `>`";
}


def SecretMatrix : TypeDef<Secret_Dialect, "SecretMatrix">  {
  let summary = "Batched Secret";

  let description = [{
    A generic wrapper around another MLIR type, representing an batch(matrix) encrypted value
    but not specifying the manner of encryption. 
  }];

  // Types are written as "!<dialect_name>.<type_mnemonic>" in textal MLIR representation
  let mnemonic = "secret_matrix";

  let parameters = (ins
      "Type":$plaintextType,
      "int":$row,
      "int":$col
  );

  let assemblyFormat = "`<` $row `x` $col `x` $plaintextType `>`";
}


#endif
