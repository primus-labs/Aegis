#include <cstddef>
#include <cstdint>
#include <vector>
#include <optional>
#include <utility>

#include "llvm/include/llvm/ADT/ArrayRef.h"     
#include "llvm/include/llvm/ADT/SmallVector.h"
#include "mlir/include/mlir/Support/LLVM.h" 
#include "mlir/include/mlir/IR/AffineExpr.h"
#include "mlir/include/mlir/IR/BuiltinAttributeInterfaces.h"
#include "mlir/include/mlir/IR/Types.h" 
#include "mlir/include/mlir/Dialect/Affine/Analysis/AffineAnalysis.h"
#include "mlir/include/mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/include/mlir/Dialect/Affine/IR/AffineValueMap.h"


namespace mlir {
namespace aegis {


std::optional<std::vector<uint64_t>> extractStaticIndices(const affine::MemRefAccess& access) {
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


std::optional<uint64_t> getFlattenedAccessIndex(const affine::MemRefAccess& access, mlir::Type memRefType) 
{
    auto accessIndices = extractStaticIndices(access);
    if (!accessIndices.has_value()) {
        return std::nullopt;
    }

    return mlir::ElementsAttr::getFlattenedIndex(memRefType, llvm::ArrayRef<uint64_t>(accessIndices.value()));
}


int64_t calcFlattenIndex(llvm::ArrayRef<int64_t> indices, llvm::ArrayRef<int64_t> strides, int64_t offset) {
    int64_t index = offset;
    for (size_t i = 0; i < strides.size(); ++i) {
        index += indices[i] * strides[i];
    }
    
    return index;
}

llvm::SmallVector<int64_t> calcUnflattenIndex(int64_t index, llvm::ArrayRef<int64_t> strides, int64_t offset) {
    llvm::SmallVector<int64_t> indices;
    int64_t idx = index - offset;
    for (int64_t stride : strides) {
        indices.push_back(idx / stride);
        idx = idx % stride;
    }

    return indices;
}

} // namespace aegis
} // namespace mlir

