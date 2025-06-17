#ifndef CPU_FHE_KEY_H
#define CPU_FHE_KEY_H

#include <cstdio>
#include <stdlib.h>
#include <string>

namespace aegiscpu {

class Key {
  public:
    virtual ~Key() = default;

    /*********************************************************
     * Serialize an key object
     **********************************************************/
    virtual std::string serialize() const = 0;

    /*********************************************************
     * Deserialize data to an key object
     * @param data - deserialize data
     **********************************************************/
    virtual void deserialize(const std::string &data) = 0;
};

} // namespace aegiscpu

#endif
