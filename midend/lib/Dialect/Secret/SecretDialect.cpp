#include "Dialect/Secret/SecretDialect.h"
#include "Dialect/Secret/SecretDialect.cpp.inc"
#include "Dialect/Secret/SecretOps.h"
#include "Dialect/Secret/SecretTypes.h"
#include "llvm/ADT/TypeSwitch.h"            
#include "mlir/IR/Builders.h"               
#include "mlir/IR/DialectImplementation.h"  

#define GET_TYPEDEF_CLASSES
#include "Dialect/Secret/SecretTypes.cpp.inc"
#define GET_OP_CLASSES
#include "Dialect/Secret/SecretOps.cpp.inc"


namespace mlir {
namespace aegis {
namespace secret {


/*****************************************************************************/
// Secret dialect.
// Dialect construction: there is one instance per context and it registers its
// operations, types, and interfaces here.
/*****************************************************************************/

void SecretDialect::initialize() {
    addOperations<
#define GET_OP_LIST
#include "Dialect/Secret/SecretOps.cpp.inc"
      >();

    addTypes<
#define GET_TYPEDEF_LIST
#include "Dialect/Secret/SecretTypes.cpp.inc"
      >();
}

}  // namespace secret
}  // namespace aegis
}  // namespace mlir