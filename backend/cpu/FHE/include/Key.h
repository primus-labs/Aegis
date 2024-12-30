#ifndef CPU_FHE_KEY_H
#define CPU_FHE_KEY_H


#include <cstdio>
#include <stdlib.h>
#include <string>


/*******************************
 **********  key type *********
 *****************************/
enum KEY_TYPE {
    // fhe key type
    FHE_PRIVATE_KEY,
    FHE_PUB_KEY,
    FHE_RELIN_KEY,
    FHE_ROT_KEY,
    FHE_BS_KEY,

    // zkp key type
    ZKP_P_KEY,
    ZKP_V_KEY,

    UNKOWN_KEY,
};


class Key {
public:
    virtual ~Key() = default;

    /*********************************************************
    * Serialize an key object
    * @param filename - object to serialize
    **********************************************************/
    virtual void serialize(const std::string& filename) const = 0;

    /*********************************************************
    * Deserialize data to an key object
    * @param data - deserialize data
    **********************************************************/
    virtual void deserialize(const std::string& data) = 0;

    /*********************************************************
    * Get key type
    **********************************************************/
    virtual KEY_TYPE getType() const = 0;
};


#endif
