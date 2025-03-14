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

def FHE_AllocOp : FHE_Op<"alloc", []> {
  let summary = "allocation ciphertext operation on heap memory";

  let description = [{
    The `alloc` operation allocates a region of memory, as specified by its memref type.

    Example:
      ```mlir
      %0 = fhe.alloc() : !Fhe.LWEChiperVector<64xi32>
      ```
  }];

  let results = (outs FHE_Type:$output);
}

def FHE_AllocaOp : FHE_Op<"alloca", []> {
  let summary = "allocation ciphertext operation on stack memory";

  let description = [{
    The `alloca` operation allocates memory on the stack, to be automatically released.

    Example:
      ```mlir
      %0 = fhe.alloca() : !Fhe.LWEChiperVector<64xi32>
      ```
  }];

  // let arguments = (ins AnyType:$input);

  let results = (outs FHE_Type:$output);
}

def FHE_DeallocOp : FHE_Op<"dealloc", []> {
    let summary = "memory deallocation operation";

    let description = [{
      The `dealloc` operation frees the region of memory referenced.
      Example:
      ```mlir
      fhe.dealloc %0 : !he.LWEChiperVector<64xi32>
      ```
    }];

    let arguments = (ins FHE_Type:$memref);
}

def FHE_LoadOp : FHE_Op<"load", []> {
    let summary = "read an element from a LWECipherVector.";

    let description = [{
        Example:
        ```mlir
        %0 = "fhe.load" %A[%1]: (!fhe.LWECipherVector<i32>)
        %3 = "fhe.load" %A[%1, %2] : (!fhe.LWECipherVector<i32>)
        ```
    }];

    let arguments = (ins AnyType:$memref, Variadic<AnyType>:$indices);
    let results = (outs AnyType:$result);
}

def FHE_StoreOp : FHE_Op<"store", []> {
    let summary = "Store a value to a LWECipherVector memory location given by indices.";

    let description = [{
        Example:
        ```mlir
        "fhe.store" %0, %A[%1]: (!fhe.LWECipherVectori32>) 
        "fhe.store" %0, %A[%1, 5]: (!fhe.LWECipherVector<i32>) 
        ```
    }];

    let arguments = (ins AnyType:$valueToStore, AnyType:$memref, Variadic<AnyType>:$indices);
}

def FHE_EncodeOp : FHE_Op<"encode", []> {
  let summary = "LWE Encryption";
  let description = [{
    LWE Encryption fuction
  }];

  let arguments = (ins F32Attr:$message);
  let results = (outs AnyType:$cipher);
}

def FHE_LWEAddOp : FHE_Op<"lweadd", [Pure]> {
  let summary = "LWECipher addition operation";
  let description = [{
    The "add" operation performs element-wise addition between two lweciphers.
  }];

  // let arguments = (ins LWECipher:$lhs, LWECipher:$rhs);
  let arguments = (ins Variadic<AnyType>:$x);

  let results = (outs AnyType:$output);
}

def FHE_LWEAddPlainOp : FHE_Op<"lweaddplain", [Pure]> {
  let summary = "LWECipher addition operation(lwecipher vs plain)";
  let description = [{
    The "add" operation performs element-wise addition between lwecipher and plain.
  }];

  let arguments = (ins Variadic<AnyType>:$x);

  let results = (outs AnyType:$output);
}

def FHE_LWESubOp : FHE_Op<"lwesub", [Pure]> {
  let summary = "LWECipher subtraction operation";
  let description = [{
    The "sub" operation performs element-wise subtraction between two lweciphers.
  }];

  let arguments = (ins Variadic<AnyType>:$x);
  let results = (outs AnyType:$output);
}

def FHE_LWESubPlainOp : FHE_Op<"lwesubplain", [Pure]> {
  let summary = "LWECipher subtraction operation(lwecipher vs plain)";
  let description = [{
    The "sub" operation performs element-wise subtraction between lwecipher and plain.
    The shapes of the tensor operands are expected to match.
  }];

  let arguments = (ins Variadic<AnyType>:$x);
  let results = (outs AnyType:$output);
}

def FHE_LWENegOp : FHE_Op<"lweneg", [Pure]> {
  let summary = "LWECipher negative operation";
  let description = [{
    The "neg" operation applies element-wise negation for lweciphers.
  }];

  let arguments = (ins AnyType:$x);
  let results = (outs AnyType:$output);
}

def FHE_LWEMulOp : FHE_Op<"lwemul", [Pure]> {
  let summary = "LWECipher multiplication operation";
  let description = [{
    The "mul" operation performs element-wise multiplication between two lweciphers.
  }];

  let arguments = (ins Variadic<AnyType>:$x);

  let results = (outs AnyType:$output);
}

def FHE_LWEMulPlainOp : FHE_Op<"lwemulplain", [Pure]> {
  let summary = "LWECipher multiplication operation(lwecipher vs plain)";
  let description = [{
    The "mul" operation performs element-wise multiplication between lwecipher and plain.
  }];

  let arguments = (ins Variadic<AnyType>:$x);

  let results = (outs AnyType:$output);
}

def FHE_RLWEAddOp : FHE_Op<"rlweadd", [Pure]> {
  let summary = "RLWECipher addition operation";
  let description = [{
    The "add" operation performs element-wise addition between two rlweciphers.
  }];

  let arguments = (ins Variadic<AnyType>:$x);

  let results = (outs AnyType:$output);
}

def FHE_RLWEAddPlainOp : FHE_Op<"rlweaddplain", [Pure]> {
  let summary = "RLWECipher addition operation(rlwecipher vs plain)";
  let description = [{
    The "add" operation performs element-wise addition between rlwecipher and plain.
  }];

  let arguments = (ins Variadic<AnyType>:$x);

  let results = (outs AnyType:$output);
}

def FHE_RLWESubOp : FHE_Op<"rlwesub", [Pure]> {
  let summary = "RLWECipher subtraction operation";
  let description = [{
    The "sub" operation performs element-wise subtraction between two rlweciphers.
    The shapes of the tensor operands are expected to match.
  }];

  let arguments = (ins Variadic<AnyType>:$x);
  let results = (outs AnyType:$output);
}

def FHE_RLWESubPlainOp : FHE_Op<"rlwesubplain", [Pure]> {
  let summary = "RLWECipher subtraction operation(rlwecipher vs plain)";
  let description = [{
    The "sub" operation performs element-wise subtraction between rlwecipher and plain.
  }];

  let arguments = (ins Variadic<AnyType>:$x);
  let results = (outs AnyType:$output);
}

def FHE_RLWEMulOp : FHE_Op<"rlwemul", [Pure]> {
  let summary = "RLWECipher multiplication operation";
  let description = [{
    The "mul" operation performs element-wise multiplication between two rlweciphers.
  }];

  let arguments = (ins Variadic<AnyType>:$x);

  let results = (outs AnyType:$output);
}

def FHE_RLWEMulPlainOp : FHE_Op<"rlwemulplain", [Pure]> {
  let summary = "RLWECipher multiplication operation(rlwecipher vs plain)";
  let description = [{
    The "mul" operation performs element-wise multiplication between rlwecipher and plain.
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

  let arguments = (ins AnyType:$memref, AnyAttr:$indices);
  
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
