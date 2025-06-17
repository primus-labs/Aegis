#ifndef CPU_FHE_OPERATE_H
#define CPU_FHE_OPERATE_H

#include <inttypes.h>
#include <vector>
#include <cstddef> // size_t
#include <sstream>
#include "openfhe.h"
using namespace lbcrypto;

namespace aegiscpu {
namespace openfhe {

std::vector<uint8_t> encrypt(const std::vector<double> &data);
std::vector<double> decrypt(const std::vector<uint8_t> &data, size_t plaintextSize);
void serializeCiphertext(const Ciphertext<DCRTPoly> &ciphertext, std::vector<uint8_t> &bytes);
Ciphertext<DCRTPoly> deserializeCiphertext(const std::vector<uint8_t> &bytes);

// ecrypt multiple ciphertexts in a contiguous buffer.
// The buffer data is divided into n equal parts. The size of data must be divisible by n.
// Each part is then decrypted to a plaintext vector of size `plaintextSize`.
std::vector<std::vector<double>> decryptBatch(const std::vector<uint8_t> &data, size_t n, size_t plaintextSize);

} // namespace openfhe
} // namespace aegiscpu

#endif