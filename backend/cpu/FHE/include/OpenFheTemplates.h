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
#define Rotate(c, idx) clientCC->EvalRotate((c), (idx))
#define Bootstrap(a) clientCC->EvalBootstrap(a)
#define MakePlain(a)  double(a)
#define MakeMultPlain(...) std::vector<double>{__VA_ARGS__}
#define LoadPlainWithIndex(v, idx) v[idx]
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
    double v{0};
    memcpy(&v{0}, buf{0}.data(), buf{0}.size());
)cpp";
// clang-format on

// clang-format off
constexpr std::string_view kDeserisBufToVectDouble = R"cpp(
    std::vector<double> v{0};
    size_t count{0} = buf{0}.size() / sizeof(double);
    v{0}.resize(count{0});
    memcpy(v{0}.data(), buf{0}.data(), buf{0}.size());
)cpp";
// clang-format on

// clang-format off
constexpr std::string_view kDeserisBufToMatDouble = R"cpp(
    std::vector<double> v{0};
    size_t count{0} = buf{0}.size() / sizeof(double);
    v{0}.resize(count{0});
    memcpy(v{0}.data(), buf{0}.data(), buf{0}.size());

    std::vector<std::vector<double>> v{0};
    for (auto i = 0; i < buf{0}.size(); i++) {{
        std::vector<double> item;
        size_t count = buf{0}[i].size() / sizeof(double);
        item.resize(count);
        memcpy(item.data(), buf{0}[i].data(), buf{0}[i].size());
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
std::vector<double> coeff1 = {{1.83684403e+00, -2.70230296e-01, 1.39933082e-02, -3.08382753e-04, 3.01902138e-06, -1.08264445e-08};
std::vector<double> coeff2 = {{1.36528002e+01, -1.44508325e+02, 5.88075783e+02, -1.14488119e+03, 1.23031707e+03, -7.83728468e+02, 3.03280966e+02, -6.99653527e+01, 8.84524133e+00, -4.71519103e-01};
std::vector<double> coeff3 = {{5.46535806e+00, -3.13317662e+01, 1.10634375e+02, -2.21351538e+02, 2.64761671e+02, -1.96093273e+02, 9.05786026e+01, -2.53712497e+01, 3.94000641e+00, -2.60142549e-01};

template <typename T>
inline uint64_t ceilLog2(T x) {{
    return static_cast<uint64_t>(std::ceil(std::log2(x)));
}

void EvalPower(std::vector<double> coefficients, std::vector<Ciphertext<lbcrypto::DCRTPoly>>& power_basis,
               Ciphertext<lbcrypto::DCRTPoly>& result) {{
    if (coefficients.size() == 1) {{
        if (std::fabs(std::round(coefficients[0] * pow(2, 50))) > 1.) {{
            result = clientCC->EvalMult(power_basis[0], coefficients[0]);
            return;
        }
        else {{
            return;
        }
    }

    std::vector<double> quotient, remainder;
    uint64_t degree = 2 * coefficients.size() - 1;
    uint64_t m      = ceilLog2(degree + 1);
    remainder.resize((1 << (m - 1)) / 2);
    quotient.resize(coefficients.size() - remainder.size());

    for (size_t i = 0; i < remainder.size(); i++) {{
        remainder[i] = coefficients[i];
    }
    for (size_t i = 0; i < quotient.size(); i++) {{
        quotient[i] = coefficients[i + remainder.size()];
    }

    Ciphertext<lbcrypto::DCRTPoly> cipher_quotient, cipher_remainder;
    EvalPower(quotient, power_basis, cipher_quotient);
    EvalPower(remainder, power_basis, cipher_remainder);
    result = clientCC->EvalMult(cipher_quotient, power_basis[m - 1]);
    result = clientCC->EvalAdd(result, cipher_remainder);
}

void polyEvalPower(Ciphertext<DCRTPoly>& result, Ciphertext<DCRTPoly>& x, std::vector<double>& coefficients) {{
    uint64_t degree = coefficients.size() * 2 - 1;
    uint64_t m      = ceilLog2(degree + 1);

    std::vector<Ciphertext<DCRTPoly>> power_basis(m);
    power_basis[0] = x;
    for (size_t i = 1; i < m; i++) {{
        power_basis[i] = clientCC->EvalMult(power_basis[i - 1], power_basis[i - 1]);
    }

    EvalPower(coefficients, power_basis, result);
}

Ciphertext<DCRTPoly> EvalCmpWithZero(Ciphertext<DCRTPoly>& x) {{
    Ciphertext<DCRTPoly> result, x1, x2;
    polyEvalPower(x1, x, coeff1);
    polyEvalPower(x2, x1, coeff2);
    for (auto &x : coeff3) {{
        x /= 2.0;
    }
    polyEvalPower(result, x2, coeff3);
    result = clientCC->EvalAdd(result, 0.5);
    return result;
}

inline Ciphertext<DCRTPoly> Cmp_gt(const Ciphertext<DCRTPoly> &x, const Ciphertext<DCRTPoly> &y) {{
    auto z = clientCC->EvalSub(x, y);
    auto cond = EvalCmpWithZero(z);
    Plaintext plainTwo = clientCC->MakeCKKSPackedPlaintext(std::vector({0}, 2.0));
    auto cipherTwo     = clientCC->Encrypt(clientPubKey, plainTwo);
    return clientCC->EvalSub(clientCC->EvalMult(clientCC->EvalMult(cond, cond), cipherTwo), cond);
}

inline Ciphertext<DCRTPoly> Cmp_eq(const Ciphertext<DCRTPoly> &x, const Ciphertext<DCRTPoly> &y) {{
    auto z = clientCC->EvalSub(x, y);
    auto cond = EvalCmpWithZero(z);
    Plaintext plainFour = clientCC->MakeCKKSPackedPlaintext(std::vector({0}, 4.0));
    auto cipherFour     = clientCC->Encrypt(clientPubKey, plainFour);
    auto FourCond = clientCC->EvalMult(cipherFour, cond);
    return clientCC->EvalSub(FourCond, clientCC->EvalMult(FourCond, cond));
}

inline Ciphertext<DCRTPoly> Cmp_lt(const Ciphertext<DCRTPoly> &x, const Ciphertext<DCRTPoly> &y) {{
    auto z = clientCC->EvalSub(x, y);
    auto cond = EvalCmpWithZero(z);
    Plaintext plainOne   = clientCC->MakeCKKSPackedPlaintext(std::vector({0}, 1.0));
    Plaintext plainTwo   = clientCC->MakeCKKSPackedPlaintext(std::vector({0}, 2.0));
    Plaintext plainThree = clientCC->MakeCKKSPackedPlaintext(std::vector({0}, 3.0));
    auto cipherOne       = clientCC->Encrypt(clientPubKey, plainOne);
    auto cipherTwo       = clientCC->Encrypt(clientPubKey, plainTwo);
    auto cipherThree     = clientCC->Encrypt(clientPubKey, plainThree);
    auto cipherThreeCond = clientCC->EvalMult(cipherThree, cond);
    auto cipherTwoCondPower = clientCC->EvalMult(cipherTwo, clientCC->EvalMult(cond, cond));
    return clientCC->EvalAdd(clientCC->EvalSub(cipherOne, cipherThreeCond), cipherTwoCondPower);
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
    auto cipherOne       = clientCC->Encrypt(clientPubKey, plainOne);
    auto cipherTwo       = clientCC->Encrypt(clientPubKey, plainTwo);
    auto cipherTwoCondPower = clientCC->EvalMult(cipherTwo, clientCC->EvalMult(cond, cond));
    return clientCC->EvalSub(clientCC->EvalAdd(cipherOne, cond), cipherTwoCondPower);
}

inline Ciphertext<DCRTPoly> Cmp_ue(const Ciphertext<DCRTPoly> &x, const Ciphertext<DCRTPoly> &y) {{
    Plaintext plainOne   = clientCC->MakeCKKSPackedPlaintext(std::vector({0}, 1.0));
    auto cipherOne       = clientCC->Encrypt(clientPubKey, plainOne);
    return clientCC->EvalSub(cipherOne, Cmp_eq(x, y));
}

inline Ciphertext<DCRTPoly> Select(const Ciphertext<DCRTPoly> &cond, const Ciphertext<DCRTPoly> &x, 
                                    const Ciphertext<DCRTPoly> &y) {{
    Plaintext plainOne = clientCC->MakeCKKSPackedPlaintext(std::vector({0}, 1.0));
    auto cipherOne     = clientCC->Encrypt(clientPubKey, plainOne);
    auto notCond       = clientCC->EvalSub(cipherOne, cond);
    return clientCC->EvalAdd(clientCC->EvalMult(cond, x), clientCC->EvalMult(notCond, y));
}
)cpp";
// clang-format on

} // namespace openfhe
} // namespace aegiscpu


#endif // CPU_FHE_INCLUDE_OPENFHETEMPLATES_H_