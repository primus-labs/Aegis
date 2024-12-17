#ifndef DIALECT_SECRET_SECRETOPS_H
#define DIALECT_SECRET_SECRETOPS_H

#include <mlir/IR/Builders.h>
#include <mlir/IR/BuiltinOps.h>
#include <mlir/IR/BuiltinTypes.h>
#include <mlir/Interfaces/ControlFlowInterfaces.h>
#include <mlir/Interfaces/SideEffectInterfaces.h>

#include "Dialect/Secret/SecretDialect.h"
#include "Dialect/Secret/SecretTypes.h"

#define GET_OP_CLASSES
#include "Dialect/Secret/SecretOps.h.inc"


namespace mlir {
namespace aegis {
namespace secret {



}
}
}

#endif