#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include "Common/MetadataMgr.h"
#include "Common/Utils.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/include/mlir/Dialect/Affine/Analysis/AffineAnalysis.h"
#include "mlir/include/mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/include/mlir/Dialect/Affine/IR/AffineValueMap.h"
#include "mlir/include/mlir/IR/AffineExpr.h"
#include "mlir/include/mlir/IR/BuiltinAttributeInterfaces.h"
#include "mlir/include/mlir/IR/Types.h"
#include "mlir/include/mlir/Support/LLVM.h"
#include "llvm/include/llvm/ADT/ArrayRef.h"
#include "llvm/include/llvm/ADT/SmallVector.h"

namespace mlir {
namespace aegis {

std::optional<std::vector<uint64_t>>
extractStaticIndices(const affine::MemRefAccess &access) {
  affine::AffineValueMap thisMap;
  access.getAccessMap(&thisMap);
  std::vector<uint64_t> accessIndices;

  for (size_t i = 0; i < access.getRank(); ++i) {
    // The access indices of the global memref must be constant,
    // meaning that they cannot be a variable access (for example, a
    // loop index) or symbolic, for example, an input symbol.
    auto affineValue = thisMap.getResult(i);
    if (affineValue.getKind() != AffineExprKind::Constant) {
      return std::nullopt;
    }

    accessIndices.push_back(
        (dyn_cast<mlir::AffineConstantExpr>(thisMap.getResult(i))).getValue());
  }

  return std::move(accessIndices);
}

std::optional<uint64_t>
getFlattenedAccessIndex(const affine::MemRefAccess &access,
                        mlir::Type memRefType) {
  auto accessIndices = extractStaticIndices(access);
  if (!accessIndices.has_value()) {
    return std::nullopt;
  }

  return mlir::ElementsAttr::getFlattenedIndex(
      memRefType, llvm::ArrayRef<uint64_t>(accessIndices.value()));
}

int64_t calcFlattenIndex(llvm::ArrayRef<int64_t> indices,
                         llvm::ArrayRef<int64_t> strides, int64_t offset) {
  int64_t index = offset;
  for (size_t i = 0; i < strides.size(); ++i) {
    index += indices[i] * strides[i];
  }

  return index;
}

llvm::SmallVector<int64_t> calcUnflattenIndex(int64_t index,
                                              llvm::ArrayRef<int64_t> strides,
                                              int64_t offset) {
  llvm::SmallVector<int64_t> indices;
  int64_t idx = index - offset;
  for (int64_t stride : strides) {
    indices.push_back(idx / stride);
    idx = idx % stride;
  }

  return indices;
}

bool isArgEncrypted(Value value) {
  // If the value is a BlockArgument, check its attribute.
  // If the BlockArgument value no metadata, then default param is encrypted.
  if (auto arg = mlir::dyn_cast<BlockArgument>(value)) {
    auto funcOp = mlir::dyn_cast<func::FuncOp>(arg.getOwner()->getParentOp());
    assert(funcOp);

    const MetadataMgr &metaMgr = MetadataMgr::getInstance();
    unsigned idx = arg.getArgNumber();
    if (auto nameAttr = funcOp.getArgAttr(idx, PARAM_ATTR_NAME)) {
      if (auto strAttr = mlir::dyn_cast<StringAttr>(nameAttr)) {
        bool bClear = (metaMgr.getMetadata(strAttr.getValue()) == CLEAR);
        return !bClear;
      }
    } else {
      // default set type as encrypted
      return true;
    }
  }

  // Return true in others.
  return true;
}

bool isEncrypted(Value value, llvm::DenseMap<Value, bool> &cache) {
  // Check if the value is already in the cache
  auto it = cache.find(value);
  if (it != cache.end()) {
    return it->second;
  }

  // If the value is a BlockArgument, check its attribute.
  // If the BlockArgument value no metadata, then default param is encrypted.
  if (auto arg = mlir::dyn_cast<BlockArgument>(value)) {
    auto funcOp = mlir::dyn_cast<func::FuncOp>(arg.getOwner()->getParentOp());
    assert(funcOp);

    const MetadataMgr &metaMgr = MetadataMgr::getInstance();
    unsigned idx = arg.getArgNumber();
    if (auto nameAttr = funcOp.getArgAttr(idx, PARAM_ATTR_NAME)) {
      if (auto strAttr = mlir::dyn_cast<StringAttr>(nameAttr)) {
        bool bClear = (metaMgr.getMetadata(strAttr.getValue()) == CLEAR);
        cache[value] = !bClear;
        return !bClear;
      }
    } else {
      // can't find argument attrbute, default set argument type as encrypted.
      cache[value] = true;
      return true;
    }
  }

  // If the value is an operation result, check its defining operation
  if (auto opResult = mlir::dyn_cast<OpResult>(value)) {
    Operation *op = opResult.getDefiningOp();

    // If the value is an constop, then return false.
    if (auto const_op = mlir::dyn_cast<arith::ConstantOp>(op)) {
      cache[value] = false;
      return false;
    }

    // If the value is an get_global op, then return false.
    if (auto getGlobal = mlir::dyn_cast<memref::GetGlobalOp>(op)) {
      cache[value] = false;
      return false;
    }

    // // If the operation is a secret operation and the secret op is not
    // castop, the result is encrypted if
    // (isa<secret::SecretDialect>(op->getDialect()) &&
    // !isa<secret::CastOp>(op)) {
    //     cache[value] = true;
    //     return true;
    // }

    // For any operation, check if any of its operands is encrypted
    for (Value operand : op->getOperands()) {
      if (isEncrypted(operand, cache)) {
        cache[value] = true;
        return true;
      } else {
        continue;
      }
    }

    // Once none of the operands of this op are of the encrypted type, return
    // false.
    cache[value] = false;
    return false;
  }

  // Default to true if the value is neither a BlockArgument nor an OpResult
  cache[value] = true;
  return true;
}

} // namespace aegis
} // namespace mlir
