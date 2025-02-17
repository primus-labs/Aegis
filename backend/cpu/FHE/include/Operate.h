#ifndef CPU_FHE_OPERATE_H
#define CPU_FHE_OPERATE_H


namespace aegiscpu {

std::vector<uint8_t> encrypt(std::vector<double>& data);
std::vector<double> decrypte(std::vector<uint8_t>& data);

} //namespace aegiscpu

#endif