#ifndef DIALECT_SECRET_SECRETOPS_TD_
#define DIALECT_SECRET_SECRETOPS_TD_

include "SecretDialect.td"
include "SecretTypes.td"
include "mlir/IR/BuiltinAttributeInterfaces.td"
include "mlir/Interfaces/ControlFlowInterfaces.td"
include "mlir/Interfaces/InferTypeOpInterface.td"
include "mlir/Interfaces/SideEffectInterfaces.td"


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

def Secret_RevealOp : Secret_Op<"reveal", [Pure]> {
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

def Secret_AddOp : Secret_Op<"add", [Pure, Commutative]> {
    let summary = "Adds two secret values";

    let description = [{
        The "add" operation performs element-wise addition between two secret tensors.
        The shapes of the tensor operands are expected to match.

        Example:
        ```mlir
        "secret.add"(%a, %b): (!Secete.Secret<i32>, !Secete.Secret<i32>) -> (!ecete.Secret<i32>)
        ```
    }];

    let arguments = (ins
        AnyTypeOf<[Secret,BatchedSecret]>:$lhs,
        AnyTypeOf<[Secret,BatchedSecret]>:$rhs
    );

    let results = (outs
        AnyTypeOf<[Secret,BatchedSecret]>:$output
    );
}

def Secret_AddPlainOp : Secret_Op<"add_plain", [Pure]> {
    let summary = "Adds an secret value and a clear value";

    let description = [{
        The "add_plain" operation performs element-wise addition between secret value and clear value.
        The shapes of the two value operands are expected to match.

        Example:
        ```mlir
        "secret.add_plain"(%a, %b): (!Secete.Secret<i32>, !i32) -> (!ecete.Secret<i32>)
        ```
    }];

    let arguments = (ins AnyTypeOf<[Secret,BatchedSecret]>:$a, AnyInteger:$b);

    let results = (outs
        AnyTypeOf<[Secret,BatchedSecret]>:$output
    );

    let hasFolder = 1;
}

def Secret_SubOp : Secret_Op<"sub", [Pure]> {
    let summary = "subtraction two secret values";

    let description = [{
        The "sub" operation performs element-wise subtraction between two secret tensors.
        The shapes of the tensor operands are expected to match.

        Example:
        ```mlir
        "secret.sub"(%a, %b): (!Secete.Secret<i32>, !Secete.Secret<i32>) -> (!ecete.Secret<i32>)
        ```
    }];

    let arguments = (ins
        AnyTypeOf<[Secret,BatchedSecret]>:$lhs,
        AnyTypeOf<[Secret,BatchedSecret]>:$rhs
    );

    let results = (outs
        AnyTypeOf<[Secret,BatchedSecret]>:$output
    );
}

def Secret_SubPlainOp : Secret_Op<"sub_plain", [Pure]> {
    let summary = "subtraction an secret value and a clear value";

    let description = [{
        The "sub_plain" operation performs element-wise subtraction between secret value and clear value.
        The shapes of the two value operands are expected to match.

        Example:
        ```mlir
        "secret.sub_plain"(%a, %b): (!Secete.Secret<i32>, !i32) -> (!ecete.Secret<i32>)
        ```
    }];

    let arguments = (ins AnyTypeOf<[Secret,BatchedSecret]>:$a, AnyInteger:$b);

    let results = (outs
        AnyTypeOf<[Secret,BatchedSecret]>:$output
    );

    let hasFolder = 1;
}

def Secret_NegOp : Secret_Op<"neg", [Pure]> {
    let summary = "Negates an secret value";

    let description = [{

        Example:
        ```mlir
        "secret.neg"(%a): (!Secete.Secret<i32>) -> (!Secete.Secret<i32>)
        ```
    }];

    let arguments = (ins AnyTypeOf<[Secret,BatchedSecret]>:$a);
    let results = (outs
        AnyTypeOf<[Secret,BatchedSecret]>:$output
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
        AnyTypeOf<[Secret,BatchedSecret]>:$lhs,
        AnyTypeOf<[Secret,BatchedSecret]>:$rhs
    );

    let results = (outs
        AnyTypeOf<[Secret,BatchedSecret]>:$output
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

    let arguments = (ins AnyTypeOf<[Secret,BatchedSecret]>:$a, AnyInteger:$b);

    let results = (outs
        AnyTypeOf<[Secret,BatchedSecret]>:$output
    );

    let hasFolder = 1;
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
        AnyTypeOf<[Secret,BatchedSecret]>:$lhs,
        AnyTypeOf<[Secret,BatchedSecret]>:$rhs
    );

    let results = (outs
        AnyTypeOf<[Secret,BatchedSecret]>:$output
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

    let arguments = (ins AnyTypeOf<[Secret,BatchedSecret]>:$a, AnyInteger:$b);

    let results = (outs
        AnyTypeOf<[Secret,BatchedSecret]>:$output
    );

    let hasFolder = 1;
}

def Secret_EqOp : Secret_Op<"eq", [Pure, Commutative]> {
    let summary = "Equality two secret values";

    let description = [{
        The "eq" operation performs element-wise equality between two secret tensors.
        The shapes of the tensor operands are expected to match.

        Example:
        ```mlir
        "secret.eq"(%a, %b): (!Secete.Secret<i32>, !Secete.Secret<i32>) -> (!ecete.Secret<i32>)
        ```
    }];

    let arguments = (ins
        AnyTypeOf<[Secret,BatchedSecret]>:$lhs,
        AnyTypeOf<[Secret,BatchedSecret]>:$rhs
    );

    let results = (outs
        AnyTypeOf<[Secret,BatchedSecret]>:$output
    );
}

def Secret_EqPlainOp : Secret_Op<"eq_plain", [Pure]> {
    let summary = "Equality an secret value and a clear value";

    let description = [{
        The "eq_plain" operation performs element-wise equality between secret value and clear value.
        The shapes of the two value operands are expected to match.

        Example:
        ```mlir
        "secret.eq_plain"(%a, %b): (!Secete.Secret<i32>, !i32) -> (!ecete.Secret<i32>)
        ```
    }];

    let arguments = (ins AnyTypeOf<[Secret,BatchedSecret]>:$a, AnyInteger:$b);

    let results = (outs
        AnyTypeOf<[Secret,BatchedSecret]>:$output
    );

    let hasFolder = 1;
}

def Secret_NotEqOp : Secret_Op<"not_eq", [Pure, Commutative]> {
    let summary = "Not-Equality two secret values";

    let description = [{
        The "not_eq" operation performs element-wise not-equality between two secret tensors.
        The shapes of the tensor operands are expected to match.

        Example:
        ```mlir
        "secret.not_eq"(%a, %b): (!Secete.Secret<i32>, !Secete.Secret<i32>) -> (!ecete.Secret<i32>)
        ```
    }];

    let arguments = (ins
        AnyTypeOf<[Secret,BatchedSecret]>:$lhs,
        AnyTypeOf<[Secret,BatchedSecret]>:$rhs
    );

    let results = (outs
        AnyTypeOf<[Secret,BatchedSecret]>:$output
    );
}

def Secret_NotEqPlainOp : Secret_Op<"not_eq_plain", [Pure]> {
    let summary = "Not-equality an secret value and a clear value";

    let description = [{
        The "not_eq_plain" operation performs element-wise not-equality between secret value and clear value.
        The shapes of the two value operands are expected to match.

        Example:
        ```mlir
        "secret.not_eq_plain"(%a, %b): (!Secete.Secret<i32>, !i32) -> (!ecete.Secret<i32>)
        ```
    }];

    let arguments = (ins AnyTypeOf<[Secret,BatchedSecret]>:$a, AnyInteger:$b);

    let results = (outs
        AnyTypeOf<[Secret,BatchedSecret]>:$output
    );

    let hasFolder = 1;
}

#endif  // DIALECT_SECRET_SECRETOPS_TD_