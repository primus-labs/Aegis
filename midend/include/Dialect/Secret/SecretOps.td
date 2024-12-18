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
    let summary = "Adds two encrypted integers";

    let description = [{
        The "add" operation performs element-wise addition between two tensors.
        The shapes of the tensor operands are expected to match.

        Example:
        ```mlir
        "secret.add"(%a, %b): (!Secete.Secret<2>, !Secete.Secret<2>) -> (!ecete.Secret<2>)
        ```
    }];

    let arguments = (ins
        Variadic<AnyType>:$x
    );

    let results = (outs
        AnyTypeOf<[Secret,BatchedSecret]>:$output
    );
}

def Secret_AddPlainOp : Secret_Op<"add", [Pure]> {
    let summary = "Adds an encrypted integer and a clear integer";

    let description = [{
        The "add" operation performs element-wise addition between two tensors.
        The shapes of the tensor operands are expected to match.

        Example:
        ```mlir
        "secret.add"(%a, %b): (!Secete.Secret<2>, !Secete.Secret<2>) -> (!ecete.Secret<2>)
        ```
    }];

    let arguments = (ins AnyTypeOf<[Secret,BatchedSecret]>:$a, AnyInteger:$b);

    let results = (outs
        AnyTypeOf<[Secret,BatchedSecret]>:$output
    );

    let hasFolder = 1;
}

#endif  // DIALECT_SECRET_SECRETOPS_TD_