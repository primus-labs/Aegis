#include "CryptoContextMgr.h"

namespace aegiscpu {
namespace openfhe {

CryptoContextMgr &CryptoContextMgr::getInstance() {
    static CryptoContextMgr instance;
    return instance;
}

} // namespace openfhe
} // namespace aegiscpu
