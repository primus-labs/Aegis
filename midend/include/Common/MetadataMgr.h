#ifndef COMMON_METADATA_MGR_H
#define COMMON_METADATA_MGR_H

#include "mlir/IR/Attributes.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringRef.h"
#include <memory>
#include <mutex>

namespace mlir {
namespace aegis {

class MetadataMgr {
public:
  // Get the singleton instance
  static MetadataMgr &getInstance();

  // Add metadata
  void addMetadata(llvm::StringRef key, llvm::StringRef value);

  // Get metadata
  llvm::StringRef getMetadata(llvm::StringRef key) const;

  // Remove metadata
  void removeMetadata(llvm::StringRef key);

  // Clear all metadata
  void clearMetadata();

  // DisDeable copying and assignment
  MetadataMgr(const MetadataMgr &) = delete;
  MetadataMgr &operator=(const MetadataMgr &) = delete;

private:
  // Private constructor to enforce singleton pattern
  MetadataMgr() = default;

  // Map to store metadata
  llvm::DenseMap<llvm::StringRef, llvm::StringRef> metadataMap;

  // Mutex to ensure thread safety
  mutable std::mutex mutex;
};

} // namespace aegis
} // namespace mlir

#endif // COMMON_METADATA_MGR_H