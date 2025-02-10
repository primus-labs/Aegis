#include "Dialect/FHE/FHEDialect.h"
#include "Dialect/FHE/FHEDialect.cpp.inc"
#include "Dialect/FHE/FHEOps.h"
#include "Dialect/FHE/FHETypes.h"
#include "llvm/ADT/TypeSwitch.h"            
#include "mlir/IR/Builders.h"               
#include "mlir/IR/DialectImplementation.h"  

#define GET_TYPEDEF_CLASSES
#include "Dialect/FHE/FHETypes.cpp.inc"
#define GET_OP_CLASSES
#include "Dialect/FHE/FHEOps.cpp.inc"


namespace mlir {
namespace aegis {
namespace fhe {


/*****************************************************************************/
// FHE dialect.
// Dialect construction: there is one instance per context and it registers its
// operations, types, and interfaces here.
/*****************************************************************************/

void FHEDialect::initialize() {
    addOperations<
#define GET_OP_LIST
#include "Dialect/FHE/FHEOps.cpp.inc"
      >();

    addTypes<
#define GET_TYPEDEF_LIST
#include "Dialect/FHE/FHETypes.cpp.inc"
      >();
}

}  // namespace fhe
}  // namespace aegis
}  // namespace mlir