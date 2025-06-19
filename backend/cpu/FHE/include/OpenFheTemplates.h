#ifndef CPU_FHE_INCLUDE_OPENFHETEMPLATES_H_
#define CPU_FHE_INCLUDE_OPENFHETEMPLATES_H_

#include <string_view>

namespace aegiscpu {
namespace openfhe {

// clang-format off
constexpr std::string_view kIncludeStmts = R"cpp(
#include "openfhe.h"
#include <vector>
#include <iostream>
#include <cmath>
#include <functional>
)cpp";
// clang-format on

// clang-format off
constexpr std::string_view kDeserialToDoubleFuncs = R"cpp(
inline bool isLittleEndian() {
    static const uint32_t testValue = 0x01020304;
    static const bool result = (reinterpret_cast<const uint8_t*>(&testValue)[0] == 0x04);
    return result;
}
inline uint64_t toBigEndian64(uint64_t value) {
    if (isLittleEndian()) {
        return ((value & 0x00000000000000FFULL) << 56) |
               ((value & 0x000000000000FF00ULL) << 40) |
               ((value & 0x0000000000FF0000ULL) << 24) |
               ((value & 0x00000000FF000000ULL) << 8) |
               ((value & 0x000000FF00000000ULL) >> 8) |
               ((value & 0x0000FF0000000000ULL) >> 24) |
               ((value & 0x00FF000000000000ULL) >> 40) |
               ((value & 0xFF00000000000000ULL) >> 56);
    }
    return value;
}
inline uint64_t fromBigEndian64(uint64_t value) {
    return toBigEndian64(value);
}
inline uint32_t toBigEndian32(uint32_t value) {
    if (isLittleEndian()) {
        return ((value & 0x000000FF) << 24) |
               ((value & 0x0000FF00) << 8) |
               ((value & 0x00FF0000) >> 8) |
               ((value & 0xFF000000) >> 24);
    }
    return value;
}
inline uint32_t fromBigEndian32(uint32_t value) {
    return toBigEndian32(value);
}
inline double deserializeDouble(const uint8_t *&data) {
    uint64_t temp;
    std::memcpy(&temp, data, sizeof(temp));
    data += sizeof(temp);
    temp = fromBigEndian64(temp);
    double result;
    std::memcpy(&result, &temp, sizeof(result));
    return result;
}
inline uint32_t deserializeUint32(const uint8_t *&data) {
    uint32_t value;
    std::memcpy(&value, data, sizeof(value));
    data += sizeof(value);
    return fromBigEndian32(value);
}
std::vector<double> deserializeToVectorDouble(const std::vector<uint8_t>& bytes) {
    if (bytes.size() < sizeof(uint32_t)) {
        throw std::runtime_error("Insufficient bytes for vector<double> header");
    }
    const uint8_t* data = bytes.data();
    const uint8_t* end = data + bytes.size();
    uint32_t count = deserializeUint32(data);
    size_t expectedSize = sizeof(uint32_t) + count * sizeof(double);
    if (bytes.size() != expectedSize) {
        throw std::runtime_error("Invalid byte count for vector<double>");
    }
    std::vector<double> result;
    result.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        result.push_back(deserializeDouble(data));
    }
    if (data != end) {
        throw std::runtime_error("Extra bytes in vector<double> data");
    }
    return result;
}
)cpp";
// clang-format on

// clang-format off
constexpr std::string_view kUsingStmts = R"cpp(
using namespace std;
using namespace lbcrypto;
using CiphertextT = Ciphertext<DCRTPoly>;
using RLWECipher = Ciphertext<DCRTPoly>;
using LWECipher = Ciphertext<DCRTPoly>;
using PlaintextT = Plaintext;
using Plain = double;
using PlainVector = std::vector<double>;
using PlainMatrix = std::vector<PlainVector>;
using MutableCiphertextT = Ciphertext<DCRTPoly>;
using CCParamsT = CCParams<CryptoContextCKKSRNS>;
using CryptoContextT = CryptoContext<DCRTPoly>;
using EvalKeyT = EvalKey<DCRTPoly>;
using PrivateKeyT = PrivateKey<DCRTPoly>;
using PublicKeyT = PublicKey<DCRTPoly>;
)cpp";
// clang-format on

// clang-format off
constexpr std::string_view kMacroStmts = R"cpp(
#define Copy(src, dest) dest = src
#define AddPlain(c, p) AddPlainImpl((c), (p))
#define SubPlain(c, p) SubPlainImpl((c), (p))
#define Mul(a, b) clientCC->EvalMult((a), (b))
#define MulPlain(c, p) MulPlainImpl((c), (p))
#define Div(a, b) DivImpl((a), (b))
#define DivPlain(c, p)  DivPlainImpl((c), (p)) 
#define Reciprocal(c)   ReciprocalImpl((c), -5, 5, 156)  
#define Rotate(c, idx) clientCC->EvalRotate((c), (idx))
#define Bootstrap(a) clientCC->EvalBootstrap(a)
#define MakePlain(a)  double(a)
#define MakeMultPlain(...) std::vector<double>{__VA_ARGS__}
#define LoadPlainWithIndex(v, idx) v[idx]
#define LoadPlainWithTwoIndex(v, row, col) v[row][col]
#define LoadPlainWithoutIndex(v) v[0]
#define ConstantArray_to_PlainVector(ary) std::vector<double>(ary, ary + std::size(ary))
#define Cast_Plain_To_Index(clr) size_t(clr)
#define Cast_Plain_To_Cipher(clr) clientCC->Encrypt(clientPubKey, clientCC->MakeCKKSPackedPlaintext(std::vector<double>(clr)))
#define Cast_Stub(a) a
#define Vload(arg, idx) arg[idx]
#define Vstore(val, arg, idx) arg[idx] = val
)cpp";
// clang-format on

// clang-format off
constexpr std::string_view kFheUtilFuncs = R"cpp(
inline RLWECipher AddPlainImpl(RLWECipher a, Plain b) {
    return clientCC->EvalAdd(a, b);
}
inline RLWECipher AddPlainImpl(RLWECipher a, PlainVector b) {
    return clientCC->EvalAdd(a, clientCC->MakeCKKSPackedPlaintext(b));
}
inline RLWECipher SubPlainImpl(RLWECipher a, Plain b) {
    return clientCC->EvalSub(a, b);
}
inline RLWECipher SubPlainImpl(RLWECipher a, PlainVector b) {
    return clientCC->EvalSub(a, clientCC->MakeCKKSPackedPlaintext(b));
}
inline RLWECipher MulPlainImpl(RLWECipher a, Plain b) {
    return clientCC->EvalMult(a, b);
}
inline RLWECipher MulPlainImpl(RLWECipher a, PlainVector b) {
    return clientCC->EvalMult(a, clientCC->MakeCKKSPackedPlaintext(b));
}
template <typename T1, typename T2>
auto Add(T1&& a, T2&& b) -> decltype(auto) {
    return clientCC->EvalAdd(std::forward<T1>(a), std::forward<T2>(b));
}
template <typename T1, typename T2, typename... Ts>
auto Add(T1&& a, T2&& b, Ts&&... rest) {
    return Add(clientCC->EvalAdd(std::forward<T1>(a), std::forward<T2>(b)), std::forward<Ts>(rest)...);
}
template <typename T1, typename T2>
auto Sub(T1&& a, T2&& b) -> decltype(auto) {
    return clientCC->EvalSub(std::forward<T1>(a), std::forward<T2>(b));
}
template <typename T1, typename T2, typename... Ts>
auto Sub(T1&& a, T2&& b, Ts&&... rest) {
    return Sub(clientCC->EvalSub(std::forward<T1>(a), std::forward<T2>(b)), std::forward<Ts>(rest)...);
}
inline RLWECipher DivPlainImpl(RLWECipher cipherA, Plain plainB) {
    return clientCC->EvalMult(cipherA, 1.0/plainB);
}
RLWECipher DivPlainImpl(RLWECipher cipherA, PlainVector plainB) {
    PlainVector recipPlainB;
    for (auto i = 0; i < plainB.size(); i++) {
        recipPlainB.push_back(1.0/plainB[i]);
    }
    Plaintext plainRecipB  = clientCC->MakeCKKSPackedPlaintext(recipPlainB);
    return clientCC->EvalMult(cipherA, plainRecipB);
}
)cpp";
// clang-format on

// clang-format off
constexpr std::string_view kLoadCryptoResFunc = R"cpp(
static std::string ccFileName, pubKeyFileName, mulKeyFileName, rotKeyFileName;
extern "C"
bool loadCryptoResources(const std::string &ccLoc,  const std::string &pubKeyLoc, 
                         const std::string &multKeyLoc, const std::string &rotKeyLoc) {
    ccFileName = ccLoc;
    pubKeyFileName = pubKeyLoc;
    mulKeyFileName = multKeyLoc;
    rotKeyFileName = rotKeyLoc;
    return true;
}
)cpp";
// clang-format on

// clang-format off
constexpr std::string_view kInitCtxFunc = R"cpp(
static CryptoContext<DCRTPoly> clientCC;
static PublicKey<DCRTPoly> clientPubKey;
void initCryptContext() {{
    int ccSizes = CryptoContextFactory<DCRTPoly>::GetContextCount();
    if (ccSizes > 0) {{
        std::vector<uint32_t> levelBudget = {{3, 1};
        unsigned mulDepth = {0};
        if ({1}) {{
            SecretKeyDist secretKeyDist = UNIFORM_TERNARY;
            mulDepth += FHECKKSRNS::GetBootstrapDepth(levelBudget, secretKeyDist);
        }
        CCParams<CryptoContext{2}RNS> parameters;
        parameters.SetMultiplicativeDepth(mulDepth);
        parameters.SetFirstModSize({3});
        parameters.SetScalingModSize({4});
        parameters.SetScalingTechnique(FLEXIBLEAUTO);
        parameters.SetBatchSize({5});
        clientCC = GenCryptoContext(parameters);
        if (!Serial::DeserializeFromFile(pubKeyFileName, clientPubKey, SerType::BINARY)) {{
            std::cerr << "Cannot read serialized data from: " << pubKeyFileName << std::endl;
            std::exit(1);
        }
    } else {{
        clientCC->ClearEvalMultKeys();
        clientCC->ClearEvalAutomorphismKeys();
        lbcrypto::CryptoContextFactory<lbcrypto::DCRTPoly>::ReleaseAllContexts();
        if (!Serial::DeserializeFromFile(ccFileName, clientCC, SerType::BINARY)) {{
            std::cerr << "Cannot read serialized data from: " << ccFileName << std::endl;
            std::exit(1);
        }
        if (!Serial::DeserializeFromFile(pubKeyFileName, clientPubKey, SerType::BINARY)) {{
            std::cerr << "Cannot read serialized data from: " << pubKeyFileName << std::endl;
            std::exit(1);
        }
        std::ifstream multKeyIStream(mulKeyFileName, std::ios::in | std::ios::binary);
        if (!multKeyIStream.is_open()) {{
            std::cerr << "Cannot read serialization from " << mulKeyFileName << std::endl;
            std::exit(1);
        }
        if (!clientCC->DeserializeEvalMultKey(multKeyIStream, SerType::BINARY)) {{
            std::cerr << "Could not deserialize eval mult key file" << std::endl;
            std::exit(1);
        }      
        if (!rotKeyFileName.empty()) {{
            std::ifstream rotKeyIStream(rotKeyFileName, std::ios::in | std::ios::binary);
            if (!rotKeyIStream.is_open()) {{
                std::cerr << "Cannot read serialization from " << rotKeyFileName << std::endl;
                std::exit(1);
            }
            if (!clientCC->DeserializeEvalAutomorphismKey(rotKeyIStream, SerType::BINARY)) {{
                std::cerr << "Could not deserialize eval rot key file" << std::endl;
                std::exit(1);
            }
        }
    }
}
)cpp";
// clang-format on

// clang-format off
constexpr std::string_view kAegisAdaptorFunc = R"cpp(
extern "C" 
std::vector<uint8_t> {0}{1}({2}) {
    initCryptContext();
    {3}
    std::vector<RLWECipher> retV({4});
    std::vector<uint8_t> resBuf;
    for (auto r = 0; r < retV.size(); r++) {{
        std::stringstream retss;
        Serial::Serialize(retV[r], retss, SerType::BINARY);
        std::vector<uint8_t> tmpBuf((std::istreambuf_iterator<char>(retss)), std::istreambuf_iterator<char>());
        resBuf.insert(resBuf.end(), tmpBuf.begin(), tmpBuf.end());
    }
    return resBuf;
}
)cpp";
// clang-format on

// clang-format off
constexpr std::string_view kDeserisBufToSingleCipher = R"cpp(
    RLWECipher v{0};
    std::stringstream ss{0};
    ss{0}.write(reinterpret_cast<const char *>(buf{0}.data()), buf{0}.size());
    Serial::Deserialize(v{0}, ss{0}, SerType::BINARY);
)cpp";
// clang-format on

// clang-format off
constexpr std::string_view kDeserisBufToMultiCipher = R"cpp(
    std::vector<RLWECipher> v{0};
    for (auto i{0} = 0; i{0} < buf{0}.size(); i{0}++) {{
        RLWECipher tmp{0};
        std::stringstream ss{0};
        ss{0}.write(reinterpret_cast<const char *>(buf{0}[i{0}].data()), buf{0}[i{0}].size());
        Serial::Deserialize(tmp{0}, ss{0}, SerType::BINARY);
        v{0}.emplace_back(tmp{0});
    }
)cpp";
// clang-format on

// clang-format off
constexpr std::string_view kDeserisBufToSingleDouble = R"cpp(
    std::vector<double> temp{0} = deserializeToVectorDouble(buf{0});
    double v{0} = temp{0}[0];
)cpp";
// clang-format on

// clang-format off
constexpr std::string_view kDeserisBufToVectDouble = R"cpp(
    std::vector<double> v{0} = deserializeToVectorDouble(buf{0});
)cpp";
// clang-format on

// clang-format off
constexpr std::string_view kDeserisBufToMatDouble = R"cpp(
    std::vector<std::vector<double>> v{0};
    for (auto i = 0; i < buf{0}.size(); i++) {{
        std::vector<double> item = deserializeToVectorDouble(buf{0}[i]);
        v{0}.emplace_back(item);
    }
)cpp";
// clang-format on

// clang-format off
constexpr std::string_view kAllocFunc = R"cpp(
RLWECipher Alloc(size_t size) {
    std::vector<double> constVec({0}, 0.0);
    Plaintext plaintext = clientCC->MakeCKKSPackedPlaintext(constVec);
    return clientCC->Encrypt(clientPubKey, plaintext);
}
inline RLWECipher Alloc() {
    return Alloc({0});
}
std::vector<RLWECipher> AllocArray(size_t row, size_t col) {{
    return std::vector<RLWECipher>(row, Alloc(col));
}
)cpp";
// clang-format on

// clang-format off
constexpr std::string_view kCmpFuncsTemplate = R"cpp(
Ciphertext<DCRTPoly> EvalCmpWithZero(Ciphertext<DCRTPoly>& x) {{
    auto approx = [](double d) {{
        return 1.0 / (1.0 + std::exp(-15 * d));
    };
    return clientCC->EvalChebyshevFunction(approx, x, {1}, {2}, {3});
}

inline Ciphertext<DCRTPoly> Cmp_gt(const Ciphertext<DCRTPoly> &x, const Ciphertext<DCRTPoly> &y) {{
    auto z = clientCC->EvalSub(x, y);
    auto cond = EvalCmpWithZero(z);
    Plaintext plainTwo = clientCC->MakeCKKSPackedPlaintext(std::vector({0}, 2.0));
    return clientCC->EvalSub(clientCC->EvalMult(clientCC->EvalMult(cond, cond), plainTwo), cond);
}

inline Ciphertext<DCRTPoly> Cmp_eq(const Ciphertext<DCRTPoly> &x, const Ciphertext<DCRTPoly> &y) {{
    auto z = clientCC->EvalSub(x, y);
    auto cond = EvalCmpWithZero(z);
    Plaintext plainFour = clientCC->MakeCKKSPackedPlaintext(std::vector({0}, 4.0));
    auto FourCond = clientCC->EvalMult(cond, plainFour);
    return clientCC->EvalSub(FourCond, clientCC->EvalMult(FourCond, cond));
}

inline Ciphertext<DCRTPoly> Cmp_lt(const Ciphertext<DCRTPoly> &x, const Ciphertext<DCRTPoly> &y) {{
    auto z = clientCC->EvalSub(x, y);
    auto cond = EvalCmpWithZero(z);
    Plaintext plainOne   = clientCC->MakeCKKSPackedPlaintext(std::vector({0}, 1.0));
    Plaintext plainTwo   = clientCC->MakeCKKSPackedPlaintext(std::vector({0}, 2.0));
    Plaintext plainThree = clientCC->MakeCKKSPackedPlaintext(std::vector({0}, 3.0));
    auto cipherThreeCond = clientCC->EvalMult(cond, plainThree);
    auto cipherTwoCondPower = clientCC->EvalMult(clientCC->EvalMult(cond, cond), plainTwo);
    return clientCC->EvalAdd(clientCC->EvalSub(plainOne, cipherThreeCond), cipherTwoCondPower);
}

inline Ciphertext<DCRTPoly> Cmp_ge(const Ciphertext<DCRTPoly> &x, const Ciphertext<DCRTPoly> &y) {{
    auto z = clientCC->EvalSub(x, y);
    auto cond = EvalCmpWithZero(z);
    Plaintext plainTwo   = clientCC->MakeCKKSPackedPlaintext(std::vector({0}, 2.0));
    Plaintext plainThree = clientCC->MakeCKKSPackedPlaintext(std::vector({0}, 3.0));
    auto cipherTwo       = clientCC->Encrypt(clientPubKey, plainTwo);
    auto cipherThree     = clientCC->Encrypt(clientPubKey, plainThree);
    auto cipherThreeCond = clientCC->EvalMult(cipherThree, cond);
    auto cipherTwoCondPower = clientCC->EvalMult(cipherTwo, clientCC->EvalMult(cond, cond));
    return clientCC->EvalSub(cipherThreeCond, cipherTwoCondPower);
}

inline Ciphertext<DCRTPoly> Cmp_le(const Ciphertext<DCRTPoly> &x, const Ciphertext<DCRTPoly> &y) {{
    auto z = clientCC->EvalSub(x, y);
    auto cond = EvalCmpWithZero(z);
    Plaintext plainOne   = clientCC->MakeCKKSPackedPlaintext(std::vector({0}, 1.0));
    Plaintext plainTwo   = clientCC->MakeCKKSPackedPlaintext(std::vector({0}, 2.0));
    auto cipherTwoCondPower = clientCC->EvalMult(clientCC->EvalMult(cond, cond), plainTwo);
    return clientCC->EvalSub(clientCC->EvalAdd(cond, plainOne), cipherTwoCondPower);
}

inline Ciphertext<DCRTPoly> Cmp_ue(const Ciphertext<DCRTPoly> &x, const Ciphertext<DCRTPoly> &y) {{
    Plaintext plainOne   = clientCC->MakeCKKSPackedPlaintext(std::vector({0}, 1.0));
    return clientCC->EvalSub(plainOne, Cmp_eq(x, y));
}

inline Ciphertext<DCRTPoly> Select(const Ciphertext<DCRTPoly> &cond, const Ciphertext<DCRTPoly> &x, 
                                    const Ciphertext<DCRTPoly> &y) {{
    Plaintext plainOne = clientCC->MakeCKKSPackedPlaintext(std::vector({0}, 1.0));
    auto notCond       = clientCC->EvalSub(plainOne, cond);
    return clientCC->EvalAdd(clientCC->EvalMult(cond, x), clientCC->EvalMult(notCond, y));
}
)cpp";
// clang-format on

// clang-format off
constexpr std::string_view kDivFuncsTemplate = R"cpp(
RLWECipher ReciprocalImpl(RLWECipher cipher, double lower, double upper, uint32_t degree) {{
     return clientCC->EvalChebyshevFunction([](double x) -> double {{ 
                const double threshold = 0.0001;
                if (x >= threshold || x <= -threshold) {{
                    return 1.0 / x;
                } else {{
                    return 0.0;
                }
            }, cipher, lower, upper, degree);
}

RLWECipher DivImpl(RLWECipher cipherA, RLWECipher cipherB) {{
    return clientCC->EvalMult(cipherA, ReciprocalImpl(cipherB, {0}, {1}, {2}));
}
)cpp";

} // namespace openfhe
} // namespace aegiscpu


#endif // CPU_FHE_INCLUDE_OPENFHETEMPLATES_H_