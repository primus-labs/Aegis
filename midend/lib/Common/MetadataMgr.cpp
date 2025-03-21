#include "Common/MetadataMgr.h"

namespace mlir {
namespace aegis {

// Get the singleton instance
MetadataMgr &MetadataMgr::getInstance() {
  static MetadataMgr instance;
  return instance;
}

// Add metadata
void MetadataMgr::addMetadata(llvm::StringRef key, llvm::StringRef value) {
  std::lock_guard<std::mutex> lock(mutex);
  metadataMap[key] = value;
}

// Get metadata
llvm::StringRef MetadataMgr::getMetadata(llvm::StringRef key) const {
  std::lock_guard<std::mutex> lock(mutex);
  auto it = metadataMap.find(key);
  if (it != metadataMap.end()) {
    return it->second;
  }
  return "";
}

// Remove metadata
void MetadataMgr::removeMetadata(llvm::StringRef key) {
  std::lock_guard<std::mutex> lock(mutex);
  metadataMap.erase(key);
}

// Clear all metadata
void MetadataMgr::clearMetadata() {
  std::lock_guard<std::mutex> lock(mutex);
  metadataMap.clear();
}

} // namespace aegis
} // namespace mlir