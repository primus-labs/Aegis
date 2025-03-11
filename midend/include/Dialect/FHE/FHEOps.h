#ifndef DIALECT_FHE_FHEOPS_H
#define DIALECT_FHE_FHEOPS_H

#include <mlir/IR/Builders.h>
#include <mlir/IR/BuiltinOps.h>
#include <mlir/IR/BuiltinTypes.h>
#include <mlir/Interfaces/ControlFlowInterfaces.h>
#include <mlir/Interfaces/SideEffectInterfaces.h>
#include "mlir/Dialect/Arith/IR/Arith.h"

#include "Dialect/FHE/FHEDialect.h"
#include "Dialect/FHE/FHETypes.h"

#define GET_OP_CLASSES
#include "Dialect/FHE/FHEOps.h.inc"


namespace mlir {
namespace aegis {
namespace fhe {



}
}
}

#endif