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


/// simplifies away castop(castop(x)) to x if the types work
::mlir::OpFoldResult fhe::CastOp::fold(fhe::CastOp::FoldAdaptor adaptor)
{
    if (auto m_op = getInput().getDefiningOp<fhe::CastOp>()) {
        if (m_op.getInput().getType() == getResult().getType())
            return m_op.getInput();
        
        if (auto mm_op = m_op.getInput().getDefiningOp<fhe::CastOp>()) {
            if (mm_op.getInput().getType() == getResult().getType())
                return mm_op.getInput();
        }
    }
    else if (getInput().getType() == getResult().getType()) {
        return getInput();
    }

    return {};
}


/// simplify rotate(cipher, 0) to cipher
::mlir::OpFoldResult fhe::RotateOp::fold(fhe::RotateOp::FoldAdaptor adaptor)
{
    if (getI() == 0)
        return getCipher();

    return {};
}



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