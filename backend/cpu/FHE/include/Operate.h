#ifndef CPU_FHE_OPERATE_H
#define CPU_FHE_OPERATE_H

#include <inttypes.h>
#include <vector>
#include <cstddef> // size_t
#include <sstream>
#include "openfhe.h"
using namespace lbcrypto;

namespace aegiscpu {

std::vector<uint8_t> encrypt(const std::vector<double> &data);
std::vector<double> decrypt(const std::vector<uint8_t> &data, size_t plaintextSize);
void serializeCiphertext(const Ciphertext<DCRTPoly>& ciphertext, std::vector<uint8_t>& bytes);
Ciphertext<DCRTPoly> deserializeCiphertext(const std::vector<uint8_t>& bytes);

} // namespace aegiscpu

#endif