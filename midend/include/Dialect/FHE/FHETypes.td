#ifndef LIB_DIALECT_FHE_FHETYPES_TD_
#define LIB_DIALECT_FHE_FHETYPES_TD_

include "FHEDialect.td"

include "mlir/IR/DialectBase.td"
include "mlir/IR/AttrTypeBase.td"


def Int : TypeDef<FHE_Dialect, "Int"> {

  let description = "A type for Integer";

  let mnemonic = "int";

  let summary = "int";
}

def Float : TypeDef<FHE_Dialect, "Float"> {

  let description = "A type for float";

  let mnemonic = "f64";

  let summary = "f64";
}

def FloatVector : TypeDef<FHE_Dialect, "FloatVector"> {

  let description = "A type for vector of float";

  let mnemonic = "f64vector";

  let summary = "f64vector";
}

def Plain : TypeDef<FHE_Dialect, "Plain"> {

  let description = "A type for encoded Plaintext";

  let mnemonic = "plain";

  let summary = "plain";
}

def PlainVector : TypeDef<FHE_Dialect, "PlainVector"> {

  let description = "A type for vector of plain";

  let mnemonic = "plainvector";

  let summary = "plainvector";
}

def PlainMatrix : TypeDef<FHE_Dialect, "PlainMatrix"> {

  let description = "A type for vector of plain";

  let mnemonic = "plainmatrix";

  let summary = "plainmatrix";
}

def LWECipher : TypeDef<FHE_Dialect, "LWECipher"> {

  let description = "A type for LWE Cipher";

  let mnemonic = "lwecipher";

  let summary = "lwecipher";

  let parameters = (ins "Type":$plaintextType);

  let assemblyFormat = "`<` $plaintextType `>`";
}

def LWECipherVector : TypeDef<FHE_Dialect, "LWECipherVector"> {

  let description = "A type for vector of LWE Cipher";

  let mnemonic = "lweciphervec";

  let summary = "lweciphervec";

  let parameters = (ins
      "Type":$plaintextType,
      "int":$size
  );

  let assemblyFormat = "`<` $size`x`$plaintextType `>`";
}


def LWECipherMatrix: TypeDef<FHE_Dialect, "LWECipherMatrix"> {

  let description = "A type for matrix of LWE Cipher";

  let mnemonic = "lweciphermat";

  let summary = "lweciphermat";

  let parameters = (ins
      "Type":$plaintextType,
      "int":$row,
      "int":$col
  );

  let assemblyFormat = "`<` $row`x`$col`x`$plaintextType `>`";
}

def RLWECipher : TypeDef<FHE_Dialect, "RLWECipher"> {

  let description = "A type for RLWE Cipher";

  let mnemonic = "rlwecipher";

  let summary = "rlwecipher";

  let parameters = (ins
      "Type":$plaintextType,
      "int":$size
  );

  let assemblyFormat = "`<` $size `x` $plaintextType `>`";
}

def RLWECipherGrid: TypeDef<FHE_Dialect, "RLWECipherGrid"> {

  let description = "A type for RLWE Cipher, representing an encrypted grid (matrix) of plaintext values.";

  let mnemonic = "rlweciphermat";

  let summary = "rlweciphermat";

  let parameters = (ins
      "Type":$plaintextType,
      "int":$row,
      "int":$col
  );

  let assemblyFormat = "`<` $row`x`$col`x`$plaintextType `>`";
}


// Provide a definition of the types used in the FHE dialect.
def FHE_Type : AnyTypeOf<[Int, Float, FloatVector, Plain, PlainVector, 
                            LWECipher, LWECipherVector, LWECipherMatrix, 
                            RLWECipher, RLWECipherGrid]>;


#endif
