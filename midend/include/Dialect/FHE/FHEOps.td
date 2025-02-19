#ifndef DIALECT_FHE_FHEOPS_TD_
#define DIALECT_FHE_FHETOPS_TD_

include "mlir/IR/OpBase.td"
include "mlir/IR/AttrTypeBase.td"
include "mlir/IR/BuiltinAttributeInterfaces.td"
include "mlir/Interfaces/ControlFlowInterfaces.td"
include "mlir/Interfaces/InferTypeOpInterface.td"
include "mlir/Interfaces/SideEffectInterfaces.td"
include "mlir/Dialect/Arith/IR/ArithBase.td"
include "mlir/Dialect/Arith/IR/ArithOpsInterfaces.td"
include "mlir/IR/EnumAttr.td"
include "FHEDialect.td"
include "FHETypes.td"



// Base class for fhe dialect operations. This operation inherits from the base `Op` class in OpBase.td, and provides:
//   * The parent dialect of the operation.
//   * The mnemonic for the operation, or the name without the dialect prefix.
//   * A list of traits for the operation.
class FHE_Op<string mnemonic, list<Trait> traits = []> :
    Op<FHE_Dialect, mnemonic, !listconcat(traits, [])> {
    let cppNamespace = "::mlir::aegis::fhe";
  
    let assemblyFormat = [{`(` operands `)` attr-dict `:`  `(` type(operands) `)` `->` type(results)}];
}


def FHE_DefineOp : FHE_Op<"define", [Pure]> {
  let summary = "Define a new ciphertext";

  let description = [{
    Convert variable assignment in C program to ciphertext definition in FHE porgram
  }];

  // let arguments = (ins AnyType:$input);

  let results = (outs FHE_Type:$output);
}

def FHE_EncodeOp : FHE_Op<"encode", [Pure]> {
  let summary = "LWE Encryption";
  let description = [{
    LWE Encryption fuction
  }];

  let arguments = (ins F32Attr:$message);
  let results = (outs AnyType:$cipher);
}

def FHE_ExtractOp : FHE_Op<"extract",  [Pure]> {
  let summary = "Extract a single value from plain vector";
  let description = [{
    Extract a single value from plain vector.
  }];

  let arguments = (ins AnyType:$vector, Variadic<AnyType>:$i);
  let results = (outs AnyType:$result);
  // let assemblyFormat = "$vector `[` $i `]` attr-dict `:` type($result)";
}

def FHE_ExtractexOp : FHE_Op<"extractex", [Pure]> {
  let summary = "Extract a single value from plain vector";
  let description = [{
    Extract a single value from plain vector.
  }];

  let arguments = (ins AnyTypeOf<[LWECipherVector, LWECipherMatrix, RLWECipher, RLWECipherGrid]>:$vector,
                    AnyAttr:$col, OptionalAttr<AnyAttr>:$row);

  let results = (outs LWECipher:$result);
  // let assemblyFormat = [{ $vector `[` $i `]`  attr-dict `:` type($result) }];
}

def FHE_InsertOp : FHE_Op<"insert", []> {
  let summary = "Insert a single value to plain vector";
  let description = [{
    Insert a single value to plain vector.
  }];

  let arguments = (ins AnyType:$value, AnyType:$memref, Variadic<AnyType>:$index);
  // let results = (outs AnyType:$result);
}

def FHE_InsertexOp : FHE_Op<"insertex", []> {
  let summary = "Insert a single value to plain vector";
  let description = [{
    Insert a single value to plain vector.
  }];

  let arguments = (ins AnyType:$value, AnyType:$memref,  AnyAttr:$col, OptionalAttr<AnyAttr>:$row);
  // let results = (outs AnyType:$result);
}


def FHE_LWEAddOp : FHE_Op<"lweadd", [Pure]> {
  let summary = "LWECipher addition operation";
  let description = [{
    The "add" operation performs element-wise addition between two tensors.
    The shapes of the tensor operands are expected to match.
  }];

  // let arguments = (ins LWECipher:$lhs, LWECipher:$rhs);
  let arguments = (ins Variadic<AnyType>:$x);

  let results = (outs AnyType:$output);
}

def FHE_LWESubOp : FHE_Op<"lwesub", [Pure]> {
  let summary = "LWECipher addition operation";
  let description = [{
    The "sub" operation performs element-wise addition between two tensors.
    The shapes of the tensor operands are expected to match.
  }];

  let arguments = (ins Variadic<AnyType>:$x);
  let results = (outs AnyType:$output);
}

def FHE_LWEMulOp : FHE_Op<"lwemul", [Pure]> {
  let summary = "LWECipher multiplication operation";
  let description = [{
    The "mul" operation performs element-wise multiplication between two tensors.
    The shapes of the tensor operands are expected to match.
  }];

  let arguments = (ins Variadic<AnyType>:$x);

  let results = (outs AnyType:$output);
}

def FHE_RLWEMulOp : FHE_Op<"rlwemul", [Pure]> {
  let summary = "LWECipher multiplication operation";
  let description = [{
    The "mul" operation performs element-wise multiplication between two tensors.
    The shapes of the tensor operands are expected to match.
  }];

  let arguments = (ins Variadic<AnyType>:$x);

  let results = (outs AnyType:$output);
}

def FHE_RotateOp: FHE_Op<"rotate", [Pure]> {
  let summary = "Rotate the RLWECipher in slot form.";

  let arguments = (ins 
    AnyTypeOf<[RLWECipher, LWECipherVector]>:$cipher,
    SI32Attr:$i
  );

  let results = (outs 
    AnyTypeOf<[RLWECipher, LWECipherVector]>:$output
  );

  let hasFolder = 1;
}

def FHE_CmpOp: FHE_Op<"compare", []> {
  let summary = "LWE comparing operation";

  let arguments = (ins Arith_CmpFPredicateAttr:$predicate,
                       AnyType:$lhs,
                       AnyType:$rhs);
  
  let results = (outs AnyType:$result);
}

def FHE_SelectOp: FHE_Op<"select", []> {
  let summary = "Replace arith.select with this operation";

  let arguments = (ins AnyType:$condition,
                       AnyType:$true_value,
                       AnyType:$false_value);
  
  let results = (outs AnyType:$result);
}

def FHE_CastOp: FHE_Op<"cast", [Pure]> {
  let summary = "No-op operation used to preserve consistency of type system during type conversion";

  let arguments = (ins AnyType:$input);
  let results = (outs AnyType:$result);

  let hasFolder = 1;
}

def FHE_VectorLoadOp: FHE_Op<"vector_load", []> {
  let summary = "load a vector/slice from a matrix";

  let arguments = (ins AnyType:$memref, AnyType:$indices);

  let results = (outs AnyType:$result);
}

def FHE_VectorLoadExOp: FHE_Op<"vector_load_ex", []> {
  let summary = "Same as FHE_VectorLoadOp, but replace Value Type indices with Attr Type";

  let arguments = (ins AnyType:$memref, AnyAttr:$index);
  
  let results = (outs AnyType:$result);
}

def FHE_LUTForAddOp: FHE_Op<"lut_half", []> {
  let summary = "LUT Operation for AddOp in Min Value";

  let arguments = (ins AnyType:$input);

  let results = (outs AnyType:$result);
}

def FHE_LUTForSubOp: FHE_Op<"lut_abshalf", []> {
  let summary = "LUT Operation for SubOp in Min Value";

  let arguments = (ins AnyType:$input);

  let results = (outs AnyType:$result);
}

def FHE_LUTForLTZeroOp: FHE_Op<"lut_lt_zero", []> {
  let summary = "LUT Opeartion for < 0";

  let arguments = (ins AnyType:$input);

  let results = (outs AnyType:$result);
} 

def FHE_LUTForGTZeroOp: FHE_Op<"lut_gt_zero", []> {
  let summary = "LUT Opeartion for > 0";

  let arguments = (ins AnyType:$input);

  let results = (outs AnyType:$result);
}

def FHE_LUTOp: FHE_Op<"lut", []> {
  let summary = "LUT Opeartion without function evaluation";

  let arguments = (ins AnyType:$input);

  let results = (outs AnyType:$result);
} 

def FHE_FuncCallOp: FHE_Op<"call", []> {
  let summary = "Function call to replace Func::CallOp";

  let description = [{
    To replace Func::CallOp, since type conflict between function call 
    and function definition is occured in FuncToHEIR pass.
  }];
  
  let arguments = (ins
    Arg<StrAttr, "the homomorphic function to call">:$callee,
    Arg<OptionalAttr<ArrayAttr>, "the order of operands and further attributes">:$args,
    Arg<OptionalAttr<ArrayAttr>, "template arguments">:$template_args,
    Variadic<AnyType>:$operands
  );

  let results = (outs AnyType);
}


#endif
