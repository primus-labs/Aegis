#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#include "mlir/IR/Value.h"
#include "mlir/include/mlir/IR/BuiltinOps.h"
#include "mlir/include/mlir/Dialect/Affine/Analysis/AffineAnalysis.h"
#include "mlir/include/mlir/Dialect/Affine/Utils.h"
#include "mlir/include/mlir/IR/Types.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/include/llvm/ADT/ArrayRef.h"
#include "llvm/include/llvm/ADT/SmallVector.h"
#include "llvm/Support/FormatVariadic.h"
#include "Common/FheDefines.h"
#include <cstdint>
#include <optional>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>


namespace mlir {
namespace aegis {

// Extract statically constant input indices from a MemRefAccess to a vector of
// indexes per dimension. Returns a std::nullopt if the indices are not
// constants (e.g. derived from inputs).
std::optional<std::vector<uint64_t>>
extractStaticIndices(const affine::MemRefAccess &access);

// Extract and flatten the index of a MemRefAccess to the corresponding index in
// a 1-dimensional flattened array. Returns a std::nullopt if the indices are
// not constants (e.g. derived from inputs).
std::optional<uint64_t>
getFlattenedAccessIndex(const affine::MemRefAccess &access,
                        mlir::Type memRefType);

// calc index for a memref with strided and offset data.
int64_t calcFlattenIndex(llvm::ArrayRef<int64_t> indices,
                         llvm::ArrayRef<int64_t> strides, int64_t offset);

// calc a unflatten ndex for a memref with strided and offset data.
llvm::SmallVector<int64_t> calcUnflattenIndex(int64_t index,
                                              llvm::ArrayRef<int64_t> strides,
                                              int64_t offset);

// Helper function to check if a value is encrypted
bool isEncrypted(Value value, llvm::DenseMap<Value, bool> &cache);

// Helper function to check if a value(func params) is encrypted
bool isArgEncrypted(Value value);

// Appends an integer element to the GaloisIndex metadata attribute
void addGaloisIndex(mlir::ModuleOp module, int32_t newVal);

//  Retrieves all elements from GaloisIndex metadata attribute
llvm::SmallVector<int32_t> getAllGaloisIndexs(mlir::ModuleOp module);

// Adjusts batch size based on the batchSize parameter, adjust batch size to power of 2
int64_t adjustAndGetBatchSize(int64_t batchSize);

// Searches for a specified tool in system paths and environment variables.
std::string findAegisTool(const std::string &toolFileName, const std::string &envVarName);

// Execute external program and capture output.
// Exit code of the tool, -1 indicates execution failure
int executeAegisTool(const std::string aegisTool, const std::vector<std::string> args,
                     std::string &output, std::string &errorMsg);

} // namespace aegis
} // namespace mlir

#endif // COMMON_UTILS_H