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


/// simplifies away castop(castop(x)) to x if the types work
::mlir::OpFoldResult secret::CastOp::fold(secret::CastOp::FoldAdaptor adaptor)
{
    if (auto m_op = getInput().getDefiningOp<secret::CastOp>()) {
        if (m_op.getInput().getType() == getResult().getType())
            return m_op.getInput();
        
        if (auto mm_op = m_op.getInput().getDefiningOp<secret::CastOp>()) {
            if (mm_op.getInput().getType() == getResult().getType())
                return mm_op.getInput();
        }
    }
    else if (getInput().getType() == getResult().getType()) {
        return getInput();
    }

    return {};
}


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