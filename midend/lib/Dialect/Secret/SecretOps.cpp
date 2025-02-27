#include <cassert>
#include <optional>

#include "Dialect/Secret/SecretOps.h"
#include "Dialect/Secret/SecretTypes.h"

#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/TypeSwitch.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/EmitC/IR/EmitC.h"
#include "mlir/IR/DialectImplementation.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/IR/TypeSupport.h"
#include "mlir/IR/Types.h"
#include "mlir/Support/LLVM.h"


#define DEBUG_TYPE "secret-ops"


namespace mlir {
namespace aegis {
namespace secret {

// SecretType SecretVectorType::getCorrespondingSecretType() const
// {
//     return SecretType::get(getContext(), getPlaintextType());
// }

// SecretVectorType SecretVectorType::get(::mlir::MLIRContext *context, ::mlir::Type plaintextType)
// {
//     assert(false && "This really should not be used!");
//     return get(context, plaintextType, -1);
// }


}  // namespace secret
}  // namespace aegis
}  // namespace mlir
