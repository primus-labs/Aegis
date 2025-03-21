#ifndef CPU_FHE_CRYPTOCONTEXTMGR_H
#define CPU_FHE_CRYPTOCONTEXTMGR_H

#include "openfhe.h"
using namespace lbcrypto;

namespace aegiscpu {

class CryptoContextMgr {
private:
  CryptoContext<DCRTPoly> cryptoContext;
  CryptoContextMgr() = default;

public:
  // Delete copy constructor and assignment operator to prevent copying.
  CryptoContextMgr(const CryptoContextMgr &) = delete;
  CryptoContextMgr &operator=(const CryptoContextMgr &) = delete;

  static CryptoContextMgr &getInstance() {
    static CryptoContextMgr instance;
    return instance;
  }

  CryptoContext<DCRTPoly>
  getCryptoContext(const CCParams<CryptoContextCKKSRNS> &parameters) {
    static std::once_flag initFlag;
    std::call_once(initFlag,
                   [&]() { cryptoContext = GenCryptoContext(parameters); });

    return cryptoContext;
  }

  CryptoContext<DCRTPoly> getCryptoContext() { return cryptoContext; }
  void setCryptoContext(CryptoContext<DCRTPoly> cryptoContext) {
    this->cryptoContext = cryptoContext;
  }
};

} // namespace aegiscpu

#endif