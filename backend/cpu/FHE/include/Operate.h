#ifndef CPU_FHE_OPERATE_H
#define CPU_FHE_OPERATE_H

#include <inttypes.h>
#include <vector>

namespace aegiscpu {

std::vector<uint8_t> encrypt(std::vector<double>& data);
std::vector<double> decrypt(std::vector<uint8_t>& data);

} //namespace aegiscpu

#endif