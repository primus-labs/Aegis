#ifndef COMMON_FHEDEFINES_H
#define COMMON_FHEDEFINES_H

#define EMITC_ADD_NAME      "Add"
#define EMITC_ADDPLAIN_NAME "AddPlain"
#define EMITC_SUB_NAME      "Sub"
#define EMITC_SUBPLAIN_NAME "SubPlain"
#define EMITC_NEG_NAME      "Neg"
#define EMITC_MUL_NAME      "Mul"
#define EMITC_MULPLAIN_NAME "MulPlain"
#define EMITC_DIV_NAME      "Div"
#define EMITC_DIVPLAIN_NAME "DivPlain"
#define EMITC_RECIPROCAL_NAME   "Reciprocal"
#define EMITC_ROTATE_NAME   "Rotate"
#define EMITC_BOOT_NAME     "Bootstrap"
#define EMITC_SELECT_NAME   "Select"
#define EMITC_CMP_PREFIX_NAME "Cmp_"

#define RLWECIPHER_TYPE_NAME    "RLWECipher"
#define VECT_RLWECIPHER_TYPE_NAME   "std::vector<RLWECipher>"
#define LWECIPHER_TYPE_NAME     "LWECipher"
#define VECT_LWECIPHER_TYPE_NAME    "std::vector<LWECipher>"
#define MAT_LWECIPHER_TYPE_NAME "std::vector<std::vector<LWECipher>>"
#define PLAIN_TYPE_NAME "Plain"
#define VECT_PLAIN_TYPE_NAME    "PlainVector"
#define MAT_PLAIN_TYPE_NAME "PlainMatrix"

#define ARG_OR_RET_ATTR_NAME "onnx.name"
#define ARG_OR_RET_ATTR_TYPE "onnx.type"
#define DIMS_ATTR_NAME  "onnx.dims"
#define ENCRYPTED "encrypted"
#define CLEAR "clear"
#define GALOIS_KEY_INDEX    "fhe.GaloisKeyIndex"
#define EXPORT_FUNCNAME_PRIFIX  "aegis_mlir_"

#define CMP_OP_MULT_DEPTH 14
#define SELECT_OP_MULT_DEPTH 2
#define RECIP_OP_MULT_DEPTH 9
#define DIV_OP_MULT_DEPTH   (RECIP_OP_MULT_DEPTH + 1)

#define LOWER_BOUND     -10
#define UPPER_BOUND     10
#define POLY_DEGREE     156

#define FHE_DEFAULT_BATCH_SIZE       256
#define FHE_MAX_MUL_DEPTH_WITH_CMP   18
#define FHE_MAX_MUL_DEPTH_NO_CMP     8
#define FHE_FIRST_MOD_SIZE  60
#define FHE_SCALE_MOD_SIZE  59
inline bool enableFheBoostrapFlag = false;
inline bool isBatchSizeAdjusted = false;
inline int64_t globalFheBatchSize = FHE_DEFAULT_BATCH_SIZE;

// In OpenFHE and other FHE libraries, the implementation of ​​homomorphic rotation​​ differs, 
// particularly in terms of ​​rotation direction​​ and ​​parameter definitions​​, which require special attention. 
// In OpenFHE, ​​positive numbers​​ represent a ​​left cyclic shift​​, while ​​negative numbers​​ correspond to a ​​right cyclic shift​​.
inline constexpr bool kNegativeShiftRight = true;    //default for OpenFHE


#endif //COMMON_FHEDEFINES_H