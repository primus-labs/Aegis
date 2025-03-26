#ifndef DIALECT_SECRET_SECRETOPS_TD_
#define DIALECT_SECRET_SECRETOPS_TD_

include "SecretDialect.td"
include "SecretTypes.td"
include "mlir/IR/BuiltinAttributeInterfaces.td"
include "mlir/Interfaces/ControlFlowInterfaces.td"
include "mlir/Interfaces/InferTypeOpInterface.td"
include "mlir/Interfaces/SideEffectInterfaces.td"
include "mlir/Dialect/Arith/IR/ArithBase.td"
include "mlir/Dialect/Arith/IR/ArithOpsInterfaces.td"


class Secret_Op<string mnemonic, list<Trait> traits = []> :
        Op<Secret_Dialect, mnemonic, traits> {
    let cppNamespace = "::mlir::aegis::secret";
}


def Secret_ConcealOp : Secret_Op<"conceal", [Pure]> {
    let summary = "Convert a non-secret value into a secret";
    let description = [{
        Convert a value to a secret containing the same value.

        Examples:

        ```mlir
        %Y = secret.conceal %value : i32 -> !secret.secret<i32>
        ```
    }];

    let arguments = (ins AnyType:$cleartext);
    let results = (outs Secret:$output);
    let assemblyFormat = "$cleartext attr-dict `:` type($cleartext) `->` type($output)";

    let builders = [
        // Builder to infer output type from the input type
        OpBuilder<(ins "Value":$cleartext)>
    ];
}

def Secret_RevealOp : Secret_Op<"reveal", []> {
    let summary = "Convert a secret value into a non-secret";
    let description = [{
        Convert a secret into a non-secret containing the same value.

        Examples:

        ```mlir
        %Y = secret.reveal %secret_value : !secret.secret<i32> -> i32
        ```
    }];

    let arguments = (ins Secret:$input);
    let results = (outs AnyType:$cleartext);
    let assemblyFormat = "$input attr-dict `:` type($input) `->` type($cleartext)";

    let builders = [
        // Builder to infer output type from the input type
        OpBuilder<(ins "Value":$secret)>
    ];
}

def Secret_CastOp: Secret_Op<"cast", [Pure]> {
  let summary = "No-op operation used to preserve consistency of type system during type conversion";

  let arguments = (ins AnyType:$input);
  let results = (outs AnyType:$result);

  let hasFolder = 1;
}


/**********************************************************/
/*********        arithmetic operator          ************/
/**********************************************************/
def Secret_AddOp : Secret_Op<"add", [Pure, Commutative]> {
    let summary = "Adds two secret values";

    let description = [{
        Example:
        ```mlir
        "secret.add"(%a, %b): (!Secete.Secret<i32>, !Secete.Secret<i32>) -> (!ecete.Secret<i32>)
        ```
    }];

    let arguments = (ins
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$lhs,
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$rhs
    );

    let results = (outs
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output
    );
}

def Secret_AddPlainOp : Secret_Op<"add_plain", [Pure]> {
    let summary = "Adds an secret value and a clear value";

    let description = [{
        Example:
        ```mlir
        "secret.add_plain"(%a, %b): (!Secete.Secret<i32>, !i32) -> (!ecete.Secret<i32>)
        ```
    }];

    let arguments = (ins AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$a, AnyTypeOf<[AnyInteger, AnyFloat]>:$b);

    let results = (outs
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output
    );

    //let hasFolder = 1;
}

def Secret_SubOp : Secret_Op<"sub", [Pure]> {
    let summary = "subtraction two secret values";

    let description = [{
        Example:
        ```mlir
        "secret.sub"(%a, %b): (!Secete.Secret<i32>, !Secete.Secret<i32>) -> (!ecete.Secret<i32>)
        ```
    }];

    let arguments = (ins
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$lhs,
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$rhs
    );

    let results = (outs
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output
    );
}

def Secret_SubPlainOp : Secret_Op<"sub_plain", [Pure]> {
    let summary = "subtraction an secret value and a clear value";

    let description = [{
        Example:
        ```mlir
        "secret.sub_plain"(%a, %b): (!Secete.Secret<i32>, !i32) -> (!ecete.Secret<i32>)
        ```
    }];

    let arguments = (ins AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$a, AnyTypeOf<[AnyInteger, AnyFloat]>:$b);

    let results = (outs
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output
    );

    //let hasFolder = 1;
}

def Secret_NegOp : Secret_Op<"neg", [Pure]> {
    let summary = "Negates an secret value";

    let description = [{
        Example:
        ```mlir
        "secret.neg"(%a): (!Secete.Secret<i32>) -> (!Secete.Secret<i32>)
        ```
    }];

    let arguments = (ins AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$a);
    let results = (outs
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output
    );

}

def Secret_MulOp : Secret_Op<"mul", [Pure, Commutative]> {
    let summary = "Multiply two secret values";

    let description = [{
        The "mul" operation performs element-wise multiply between two secret tensors.
        The shapes of the tensor operands are expected to match.

        Example:
        ```mlir
        "secret.mul"(%a, %b): (!Secete.Secret<i32>, !Secete.Secret<i32>) -> (!ecete.Secret<i32>)
        ```
    }];

    let arguments = (ins
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$lhs,
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$rhs
    );

    let results = (outs
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output
    );
}

def Secret_MulPlainOp : Secret_Op<"mul_plain", [Pure]> {
    let summary = "Multiply an secret value and a clear value";

    let description = [{
        The "mul_plain" operation performs element-wise multiply between secret value and clear value.
        The shapes of the two value operands are expected to match.

        Example:
        ```mlir
        "secret.mul_plain"(%a, %b): (!Secete.Secret<i32>, !i32) -> (!ecete.Secret<i32>)
        ```
    }];

    let arguments = (ins AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$a, AnyTypeOf<[AnyInteger, AnyFloat]>:$b);

    let results = (outs
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output
    );

    //let hasFolder = 1;
}

def Secret_DivOp : Secret_Op<"div", [Pure, Commutative]> {
    let summary = "Divide two secret values";

    let description = [{
        The "div" operation performs element-wise divide between two secret tensors.
        The shapes of the tensor operands are expected to match.

        Example:
        ```mlir
        "secret.div"(%a, %b): (!Secete.Secret<i32>, !Secete.Secret<i32>) -> (!ecete.Secret<i32>)
        ```
    }];

    let arguments = (ins
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$lhs,
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$rhs
    );

    let results = (outs
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output
    );
}

def Secret_DivPlainOp : Secret_Op<"div_plain", [Pure]> {
    let summary = "Divide an secret value and a clear value";

    let description = [{
        The "div_plain" operation performs element-wise divide between secret value and clear value.
        The shapes of the two value operands are expected to match.

        Example:
        ```mlir
        "secret.div_plain"(%a, %b): (!Secete.Secret<i32>, !i32) -> (!ecete.Secret<i32>)
        ```
    }];

    let arguments = (ins AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$a, AnyTypeOf<[AnyInteger, AnyFloat]>:$b);

    let results = (outs
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output
    );

    //let hasFolder = 1;
}

def Secret_EqOp : Secret_Op<"eq", [Pure, Commutative]> {
    let summary = "Checking the equality of two secret values";

    let description = [{
        Example:
        ```mlir
        "secret.eq"(%a, %b): (!Secete.Secret<i32>, !Secete.Secret<i32>) -> (!Secete.Secret<i32>)
        ```
    }];

    let arguments = (ins
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$lhs,
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$rhs
    );

    let results = (outs
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output
    );
}

def Secret_EqPlainOp : Secret_Op<"eq_plain", [Pure]> {
    let summary = "Checking the equality between the secret value and the clear value.";

    let description = [{
        Example:
        ```mlir
        "secret.eq_plain"(%a, %b): (!Secete.Secret<i32>, !i32) -> (!Secete.Secret<i32>)
        ```
    }];

    let arguments = (ins AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$a, AnyTypeOf<[AnyInteger, AnyFloat]>:$b);

    let results = (outs
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output
    );

    //let hasFolder = 1;
}

def Secret_NotEqOp : Secret_Op<"not_eq", [Pure, Commutative]> {
    let summary = "Checking the inequality of two secret values";

    let description = [{
        Example:
        ```mlir
        "secret.not_eq"(%a, %b): (!Secete.Secret<i32>, !Secete.Secret<i32>) -> (!Secete.Secret<i32>)
        ```
    }];

    let arguments = (ins
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$lhs,
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$rhs
    );

    let results = (outs
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output
    );
}

def Secret_NotEqPlainOp : Secret_Op<"not_eq_plain", [Pure]> {
    let summary = "Checking the inequality between the secret value and the clear value.";

    let description = [{
        Example:
        ```mlir
        "secret.not_eq_plain"(%a, %b): (!Secete.Secret<i32>, !i32) -> (!Secete.Secret<i32>)
        ```
    }];

    let arguments = (ins AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$a, AnyTypeOf<[AnyInteger, AnyFloat]>:$b);

    let results = (outs
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output
    );

    //let hasFolder = 1;
}

def Secret_GreaterOp : Secret_Op<"greater", [Pure]> {
    let summary = "Returns the truth secret value of (x > y) element-wise.";

    let description = [{
        Example:
        ```mlir
        "secret.greater"(%a, %b): (!Secete.Secret<i32>, !Secete.Secret<i32>) -> (!Secete.Secret<i32>)
        ```
    }];

    let arguments = (ins
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$lhs,
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$rhs
    );

    let results = (outs
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output
    );
}

def Secret_GreaterPlainOp : Secret_Op<"greater_plain", [Pure]> {
    let summary = "Returns the truth secret value of (x > y) element-wise.";

    let description = [{
        Example:
        ```mlir
        "secret.greater_plain"(%a, %b): (!Secete.Secret<i32>, !i32) -> (!Secete.Secret<i32>)
        ```
    }];

    let arguments = (ins AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$a, AnyTypeOf<[AnyInteger, AnyFloat]>:$b);

    let results = (outs
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output
    );

    //let hasFolder = 1;
}

def Secret_GreaterEqOp : Secret_Op<"greater_eq", [Pure]> {
    let summary = "Returns the truth secret value of (x >= y) element-wise.";

    let description = [{
        Example:
        ```mlir
        "secret.greater_eq"(%a, %b): (!Secete.Secret<i32>, !Secete.Secret<i32>) -> (!Secete.Secret<i32>)
        ```
    }];

    let arguments = (ins
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$lhs,
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$rhs
    );

    let results = (outs
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output
    );
}

def Secret_GreaterEqPlainOp : Secret_Op<"greater_eq_plain", [Pure]> {
    let summary = "Returns the truth secret value of (x >= y) element-wise.";

    let description = [{
        Example:
        ```mlir
        "secret.greater_eq_plain"(%a, %b): (!Secete.Secret<i32>, !i32) -> (!Secete.Secret<i32>)
        ```
    }];

    let arguments = (ins AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$a, AnyTypeOf<[AnyInteger, AnyFloat]>:$b);

    let results = (outs
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output
    );

    //let hasFolder = 1;
}

def Secret_MaxOp : Secret_Op<"max", [Pure]> {
    let summary = "Returns the max of secret value x and secret value y element-wise.";

    let description = [{
        Example:
        ```mlir
        "secret.max"(%a, %b): (!Secete.Secret<i32>, !Secete.Secret<i32>) -> (!Secete.Secret<i32>)
        ```
    }];

    let arguments = (ins
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$lhs,
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$rhs
    );

    let results = (outs
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output
    );
}

def Secret_MaxPlainOp : Secret_Op<"max_plain", [Pure]> {
    let summary = "Returns the max of secret value x and clear value y element-wise.";

    let description = [{
        Example:
        ```mlir
        "secret.max_plain"(%a, %b): (!Secete.Secret<i32>, !i32) -> (!Secete.Secret<i32>)
        ```
    }];

    let arguments = (ins AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$a, AnyTypeOf<[AnyInteger, AnyFloat]>:$b);

    let results = (outs
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output
    );

    //let hasFolder = 1;
}

def Secret_MinOp : Secret_Op<"min", [Pure]> {
    let summary = "Returns the min of secret value x and secret value y element-wise.";

    let description = [{
        Example:
        ```mlir
        "secret.min"(%a, %b): (!Secete.Secret<i32>, !Secete.Secret<i32>) -> (!Secete.Secret<i32>)
        ```
    }];

    let arguments = (ins
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$lhs,
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$rhs
    );

    let results = (outs
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output
    );
}

def Secret_MinPlainOp : Secret_Op<"min_plain", [Pure]> {
    let summary = "Returns the min of secret value x and clear value y element-wise.";

    let description = [{
        Example:
        ```mlir
        "secret.min_plain"(%a, %b): (!Secete.Secret<i32>, !i32) -> (!Secete.Secret<i32>)
        ```
    }];

    let arguments = (ins AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$a, AnyTypeOf<[AnyInteger, AnyFloat]>:$b);

    let results = (outs
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output
    );

    //let hasFolder = 1;
}

def Secret_CmpOp: Secret_Op<"compare", []> {
  let summary = "The comparing operation";

  let arguments = (ins Arith_CmpFPredicateAttr:$predicate,
                       AnyType:$lhs,
                       AnyType:$rhs);
  
  let results = (outs AnyType:$result);
}

def Secret_SelectOp: Secret_Op<"select", []> {
  let summary = "The select operation chooses one value based on a binary condition supplied as its first operand.";

  let description = [{
        Example:
        ```mlir
        "secret.select"(%cond, %a, %b): (!AnyType, !AnyType, !AnyType) -> (!AnyType)
        ```
    }];

  let arguments = (ins AnyType:$condition,
                       AnyType:$true_value,
                       AnyType:$false_value);
  
  let results = (outs AnyType:$output);
}

def Secret_BitwiseAndOp : Secret_Op<"bitwise_and", [Pure, Commutative]> {
    let summary = "Elementwise computes the bitwise AND of secret value x and y.";

    let description = [{
        Example:
        ```mlir
        "secret.bitwise_and"(%a, %b): (!Secete.Secret<i32>, !Secete.Secret<i32>) -> (!Secete.Secret<i32>)
        ```
    }];

    let arguments = (ins
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$lhs,
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$rhs
    );

    let results = (outs
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output
    );
}

def Secret_BitwiseOrOp : Secret_Op<"bitwise_or", [Pure, Commutative]> {
    let summary = "Elementwise computes the bitwise OR of secret value x and y.";

    let description = [{
        Example:
        ```mlir
        "secret.bitwise_or"(%a, %b): (!Secete.Secret<i32>, !Secete.Secret<i32>) -> (!Secete.Secret<i32>)
        ```
    }];

    let arguments = (ins
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$lhs,
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$rhs
    );

    let results = (outs
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output
    );
}

def Secret_BitwiseXorOp : Secret_Op<"bitwise_xor", [Pure, Commutative]> {
    let summary = "Elementwise computes the bitwise XOR of secret value x and y.";

    let description = [{
        Example:
        ```mlir
        "secret.bitwise_xor"(%a, %b): (!Secete.Secret<i32>, !Secete.Secret<i32>) -> (!Secete.Secret<i32>)
        ```
    }];

    let arguments = (ins
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$lhs,
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$rhs
    );

    let results = (outs
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output
    );
}

def Secret_BitwiseInvertOp : Secret_Op<"bitwise_invert", [Pure]> {
    let summary = "Invert (flip) each bit of the secret value.";

    let description = [{
        Example:
        ```mlir
        "secret.bitwise_invert"(%a): (!Secete.Secret<i32>) -> (!Secete.Secret<i32>)
        ```
    }];

    let arguments = (ins AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$a);
    let results = (outs
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output
    );
}

def Secret_ShlPlainOp : Secret_Op<"shl_plain", [Pure]> {
    let summary = "The shl_plain operation logical shifts the secret value of the first operand to the left by the clear value of the second operand.";

    let description = [{
        Example:
        ```mlir
        "secret.shl_plain"(%a, %b): (!Secete.Secret<i32>, !i32) -> (!Secete.Secret<i32>)
        ```
    }];

    let arguments = (ins AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$a, AnyInteger:$b);

    let results = (outs
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output
    );

    //let hasFolder = 1;
}

def Secret_ShrPlainOp : Secret_Op<"shr_plain", [Pure]> {
    let summary = "The shr_plain operation logical shifts the secret value of the first operand to the right by the clear value of the second operand.";

    let description = [{
        Example:
        ```mlir
        "secret.shr_plain"(%a, %b): (!Secete.Secret<i32>, !i32) -> (!Secete.Secret<i32>)
        ```
    }];

    let arguments = (ins AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$a, AnyInteger:$b);

    let results = (outs
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output
    );

    //let hasFolder = 1;
}

def Secret_SarPlainOp : Secret_Op<"sar_plain", [Pure]> {
    let summary = "The sar_plain operation arithmetic shifts the secret value of the first operand to the right by the clear value of the second operand.";

    let description = [{
        Example:
        ```mlir
        "secret.sar_plain"(%a, %b): (!Secete.Secret<i32>, !i32) -> (!Secete.Secret<i32>)
        ```
    }];

    let arguments = (ins AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$a, AnyInteger:$b);

    let results = (outs
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output
    );

    //let hasFolder = 1;
}

def Secret_LogicalAndOp : Secret_Op<"logical_and", [Pure, Commutative]> {
    let summary = "Returns the logical AND of the secret values x and y element-wise.";

    let description = [{
        Example:
        ```mlir
        "secret.logical_and"(%a, %b): (!Secete.Secret<i32>, !Secete.Secret<i32>) -> (!Secete.Secret<i32>)
        ```
    }];

    let arguments = (ins
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$lhs,
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$rhs
    );

    let results = (outs
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output
    );
}

def Secret_LogicalOrOp : Secret_Op<"logical_or", [Pure, Commutative]> {
    let summary = "Returns the logical OR of the secret values x and y element-wise.";

    let description = [{
        Example:
        ```mlir
        "secret.logical_or"(%a, %b): (!Secete.Secret<i32>, !Secete.Secret<i32>) -> (!Secete.Secret<i32>)
        ```
    }];

    let arguments = (ins
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$lhs,
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$rhs
    );

    let results = (outs
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output
    );
}

def Secret_LogicalXorOp : Secret_Op<"logical_xor", [Pure, Commutative]> {
    let summary = "Returns the logical XOR of the secret values x and y element-wise.";

    let description = [{
        Example:
        ```mlir
        "secret.logical_xor"(%a, %b): (!Secete.Secret<i32>, !Secete.Secret<i32>) -> (!Secete.Secret<i32>)
        ```
    }];

    let arguments = (ins
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$lhs,
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$rhs
    );

    let results = (outs
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output
    );
}

def Secret_LogicalNotOp : Secret_Op<"logical_not", [Pure]> {
    let summary = "Returns the logical NOT of the secret values x element-wise.";

    let description = [{
        Example:
        ```mlir
        "secret.logical_not"(%a): (!Secete.Secret<i32>) -> (!Secete.Secret<i32>)
        ```
    }];

    let arguments = (ins AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$a);
    let results = (outs
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output
    );
}


/**********************************************************/
/***********           math operator           ************/
/**********************************************************/
def Secret_AbsOp : Secret_Op<"abs", [Pure]> {
    let summary = "Computes the absolute secret value element-wise.";

    let description = [{
        Example:
        ```mlir
        "secret.abs"(%a): (!Secete.Secret<i32>) -> (!Secete.Secret<i32>)
        ```
    }];

    let arguments = (ins AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$a);
    let results = (outs
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output
    );
}

def Secret_PowPlainOp : Secret_Op<"pow_plain", [Pure]> {
    let summary = "Computes the power of one value to another element-wise.";

    let description = [{
        Example:
        ```mlir
        "secret.shl_plain"(%a, %b): (!Secete.Secret<i32>, !i32) -> (!Secete.Secret<i32>)
        ```
    }];

    let arguments = (ins AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$a, AnyInteger:$b);

    let results = (outs
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output
    );

    //let hasFolder = 1;
}

def Secret_SqrtOp : Secret_Op<"sqrt", [Pure]> {
    let summary = "Computes element-wise square root of the input secret value.";

    let description = [{
        Example:
        ```mlir
        "secret.sqrt"(%a): (!Secete.Secret<i32>) -> (!Secete.Secret<i32>)
        ```
    }];

    let arguments = (ins AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$a);
    let results = (outs
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output
    );
}

def Secret_RsqrtOp : Secret_Op<"rsqrt", [Pure]> {
    let summary = "Computes reciprocal of square root of secret value element-wise.";

    let description = [{
        Example:
        ```mlir
        "secret.rsqrt"(%a): (!Secete.Secret<i32>) -> (!Secete.Secret<i32>)
        ```
    }];

    let arguments = (ins AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$a);
    let results = (outs
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output
    );
}

def Secret_ExpOp : Secret_Op<"exp", [Pure]> {
    let summary = "Computes base-e exponential of the specified secret value element-wise.";

    let description = [{
        Example:
        ```mlir
        "secret.exp"(%a): (!Secete.Secret<i32>) -> (!Secete.Secret<i32>)
        ```
    }];

    let arguments = (ins AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$a);
    let results = (outs
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output
    );
}

def Secret_Exp2Op : Secret_Op<"exp2", [Pure]> {
    let summary = "Computes base-2 exponential of the specified secret value element-wise.";

    let description = [{
        Example:
        ```mlir
        "secret.exp2"(%a): (!Secete.Secret<i32>) -> (!Secete.Secret<i32>)
        ```
    }];

    let arguments = (ins AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$a);
    let results = (outs
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output
    );
}

def Secret_LogOp : Secret_Op<"log", [Pure]> {
    let summary = "Computes natural logarithm of the specified secret element-wise.";

    let description = [{
        Example:
        ```mlir
        "secret.log"(%a): (!Secete.Secret<i32>) -> (!Secete.Secret<i32>)
        ```
    }];

    let arguments = (ins AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$a);
    let results = (outs
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output
    );
}

def Secret_Log2Op : Secret_Op<"log2", [Pure]> {
    let summary = "Computes base-2 logarithm of the specified secret element-wise.";

    let description = [{
        Example:
        ```mlir
        "secret.log2"(%a): (!Secete.Secret<i32>) -> (!Secete.Secret<i32>)
        ```
    }];

    let arguments = (ins AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$a);
    let results = (outs
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output
    );
}

def Secret_Log10Op : Secret_Op<"log10", [Pure]> {
    let summary = "Computes base-10 logarithm of the specified secret element-wise.";

    let description = [{
        Example:
        ```mlir
        "secret.log10"(%a): (!Secete.Secret<i32>) -> (!Secete.Secret<i32>)
        ```
    }];

    let arguments = (ins AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$a);
    let results = (outs
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output
    );
}

def Secret_Log1pOp : Secret_Op<"log1p", [Pure]> {
    let summary = "Computes natural logarithm of (1 + secret value) element-wise.";

    let description = [{
        Example:
        ```mlir
        "secret.log1p"(%a): (!Secete.Secret<i32>) -> (!Secete.Secret<i32>)
        ```
    }];

    let arguments = (ins AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$a);
    let results = (outs
        AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output
    );
}


def Secret_LoadOp : Secret_Op<"load", [MemoryEffects<[MemRead]>, Pure]> {
    let summary = "read an element from a memory.";

    let description = [{
        Example:
        ```mlir
        %0 = "secret.load" %A[%1]: (!Secete.SecretVector<i32>)
        %3 = "secret.load" %A[%1, %2] : (!Secete.SecretMatrix<i32>)
        ```
    }];

    let arguments = (ins AnyType:$memref, Variadic<AnyType>:$indices);
    let results = (outs AnyType:$result);
}

def Secret_StoreOp : Secret_Op<"store", [MemoryEffects<[MemWrite]>]> {
    let summary = "Store a value to a memory location given by indices.";

    let description = [{
        Example:
        ```mlir
        "secret.store" %0, %A[%1]: (!Secete.SecretVector<i32>) 
        "secret.store" %0, %A[%1, 5]: (!Secete.SecretMatrix<i32>) 
        ```
    }];

    let arguments = (ins AnyType:$valueToStore, AnyType:$memref, Variadic<AnyType>:$indices);
}

def Secret_AllocOp : Secret_Op<"alloc", []> {
    let summary = "The `alloc` operation allocates a region of memory.";

    let description = [{
        Example:
        ```mlir
        %0 = secret.alloc() : (!Secete.SecretVector<i32>) 
        ```
    }];

    let results = (outs AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output);
}

def Secret_DeallocOp : Secret_Op<"dealloc", []> {
    let summary = "The `dealloc` operation frees the region of memory referenced.";

    let description = [{
        Example:
        ```mlir
        secret.dealloc %0 : (!Secete.SecretVector<i32>) 
        ```
    }];

    let arguments = (ins AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$memref);
}

def Secret_AllocaOp : Secret_Op<"alloca", []> {
    let summary = "The `alloca` operation allocates memory on the stack, to be automatically released";

    let description = [{
        Example:
        ```mlir
        %0 = secret.alloca() : (!Secete.SecretVector<i32>) 
        ```
    }];

    let results = (outs AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$output);
}

def Secret_CopyOp : Secret_Op<"copy", []> {
    let summary = "Copies the data from the source to the destination.";

    let description = [{
        Example:
        ```mlir
        secret.copy %arg0, %arg1 : !Secete.SecretVector<i32> to !Secete.SecretVector<i32>
        ```
    }];

    let arguments = (ins AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$source,
                         AnyTypeOf<[Secret,SecretVector,SecretMatrix]>:$target);

    let assemblyFormat = [{
        $source `,` $target attr-dict `:` type($source) `to` type($target)
    }];
}

def Secret_CallOp: Secret_Op<"call", []> {
  let summary = "call operation";

  let description = [{
    The `secret.call` operation represents a direct call to a function that is
    within the same symbol scope as the call. The operands and result types of
    the call must match the specified function type. The callee is encoded as a
    symbol reference attribute named "callee".
    the call op to replace func::CallOp, since type conflict between function call 
    and function definition is occured in aegis other pass.
  }];
  
  let arguments = (ins
    Arg<StrAttr, "the secret function to call">:$callee,
    Arg<OptionalAttr<ArrayAttr>, "the arguments attributes">:$arg_attrs,
    Arg<OptionalAttr<ArrayAttr>, "the results attributes">:$res_attrs,
    Variadic<AnyType>:$operands
  );

  let results = (outs Variadic<AnyType>);
}


#endif  // DIALECT_SECRET_SECRETOPS_TD_