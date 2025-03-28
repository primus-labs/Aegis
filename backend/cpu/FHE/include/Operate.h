#ifndef CPU_FHE_OPERATE_H
#define CPU_FHE_OPERATE_H

#include <inttypes.h>
#include <vector>
#include <cstddef> // size_t

namespace aegiscpu {

std::vector<uint8_t> encrypt(const std::vector<double> &data);
std::vector<double> decrypt(const std::vector<uint8_t> &data, size_t plaintextSize);

} // namespace aegiscpu

#endif