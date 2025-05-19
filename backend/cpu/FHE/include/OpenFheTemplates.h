#ifndef CPU_FHE_INCLUDE_OPENFHETEMPLATES_H_
#define CPU_FHE_INCLUDE_OPENFHETEMPLATES_H_

#include <string_view>

namespace aegiscpu {
namespace openfhe {

// clang-format off
constexpr std::string_view kCmpFuncsTemplate = R"cpp(
std::vector<double> coeff1 = {1.83684403e+00, -2.70230296e-01, 1.39933082e-02, -3.08382753e-04, 3.01902138e-06, -1.08264445e-08};
std::vector<double> coeff2 = {1.36528002e+01, -1.44508325e+02, 5.88075783e+02, -1.14488119e+03, 1.23031707e+03, -7.83728468e+02, 3.03280966e+02, -6.99653527e+01, 8.84524133e+00, -4.71519103e-01};
std::vector<double> coeff3 = {5.46535806e+00, -3.13317662e+01, 1.10634375e+02, -2.21351538e+02, 2.64761671e+02, -1.96093273e+02, 9.05786026e+01, -2.53712497e+01, 3.94000641e+00, -2.60142549e-01};

template <typename T>
inline uint64_t ceilLog2(T x) {
    return static_cast<uint64_t>(std::ceil(std::log2(x)));
}

void EvalPower(std::vector<double> coefficients, std::vector<Ciphertext<lbcrypto::DCRTPoly>>& power_basis,
               Ciphertext<lbcrypto::DCRTPoly>& result) {
    if (coefficients.size() == 1) {
        if (std::fabs(std::round(coefficients[0] * pow(2, 50))) > 1.) {
            result = clientCC->EvalMult(power_basis[0], coefficients[0]);
            return;
        }
        else {
            return;
        }
    }

    std::vector<double> quotient, remainder;
    uint64_t degree = 2 * coefficients.size() - 1;
    uint64_t m      = ceilLog2(degree + 1);
    remainder.resize((1 << (m - 1)) / 2);
    quotient.resize(coefficients.size() - remainder.size());

    for (size_t i = 0; i < remainder.size(); i++) {
        remainder[i] = coefficients[i];
    }
    for (size_t i = 0; i < quotient.size(); i++) {
        quotient[i] = coefficients[i + remainder.size()];
    }

    Ciphertext<lbcrypto::DCRTPoly> cipher_quotient, cipher_remainder;
    EvalPower(quotient, power_basis, cipher_quotient);
    EvalPower(remainder, power_basis, cipher_remainder);
    result = clientCC->EvalMult(cipher_quotient, power_basis[m - 1]);
    result = clientCC->EvalAdd(result, cipher_remainder);
}

void polyEvalPower(Ciphertext<DCRTPoly>& result, Ciphertext<DCRTPoly>& x, std::vector<double>& coefficients) {
    uint64_t degree = coefficients.size() * 2 - 1;
    uint64_t m      = ceilLog2(degree + 1);

    std::vector<Ciphertext<DCRTPoly>> power_basis(m);
    power_basis[0] = x;
    for (size_t i = 1; i < m; i++) {
        power_basis[i] = clientCC->EvalMult(power_basis[i - 1], power_basis[i - 1]);
    }

    EvalPower(coefficients, power_basis, result);
}

Ciphertext<DCRTPoly> EvalCmpWithZero(Ciphertext<DCRTPoly>& x) {
    Ciphertext<DCRTPoly> result, x1, x2;
    polyEvalPower(x1, x, coeff1);
    polyEvalPower(x2, x1, coeff2);
    for (auto &x : coeff3) {
        x /= 2.0;
    }
    polyEvalPower(result, x2, coeff3);
    result = clientCC->EvalAdd(result, 0.5);
    return result;
}

inline Ciphertext<DCRTPoly> EvalCmp_gt(const Ciphertext<DCRTPoly> &x, const Ciphertext<DCRTPoly> &y) {
    auto z = clientCC->EvalSub(x, y);
    auto cond = EvalCmpWithZero(z);
    Plaintext plainTwo = clientCC->MakeCKKSPackedPlaintext(std::vector({0}, 2.0));
    auto cipherTwo     = clientCC->Encrypt(clientPubKey, plainTwo);
    return clientCC->EvalSub(clientCC->EvalMult(clientCC->EvalMult(cond, cond), cipherTwo), cond);
}

inline Ciphertext<DCRTPoly> EvalCmp_eq(const Ciphertext<DCRTPoly> &x, const Ciphertext<DCRTPoly> &y) {
    auto z = clientCC->EvalSub(x, y);
    auto cond = EvalCmpWithZero(z);
    Plaintext plainFour = clientCC->MakeCKKSPackedPlaintext(std::vector({0}, 4.0));
    auto cipherFour     = clientCC->Encrypt(clientPubKey, plainFour);
    auto FourCond = clientCC->EvalMult(cipherFour, cond);
    return clientCC->EvalSub(FourCond, clientCC->EvalMult(FourCond, cond));
}

inline Ciphertext<DCRTPoly> EvalCmp_lt(const Ciphertext<DCRTPoly> &x, const Ciphertext<DCRTPoly> &y) {
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

inline Ciphertext<DCRTPoly> EvalCmp_ge(const Ciphertext<DCRTPoly> &x, const Ciphertext<DCRTPoly> &y) { 
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

inline Ciphertext<DCRTPoly> EvalCmp_le(const Ciphertext<DCRTPoly> &x, const Ciphertext<DCRTPoly> &y) {
    auto z = clientCC->EvalSub(x, y);
    auto cond = EvalCmpWithZero(z);
    Plaintext plainOne   = clientCC->MakeCKKSPackedPlaintext(std::vector({0}, 1.0));
    Plaintext plainTwo   = clientCC->MakeCKKSPackedPlaintext(std::vector({0}, 2.0));
    auto cipherOne       = clientCC->Encrypt(clientPubKey, plainOne);
    auto cipherTwo       = clientCC->Encrypt(clientPubKey, plainTwo);
    auto cipherTwoCondPower = clientCC->EvalMult(cipherTwo, clientCC->EvalMult(cond, cond));
    return clientCC->EvalSub(clientCC->EvalAdd(cipherOne, cond), cipherTwoCondPower);
}

inline Ciphertext<DCRTPoly> EvalCmp_ue(const Ciphertext<DCRTPoly> &x, const Ciphertext<DCRTPoly> &y) {
    Plaintext plainOne   = clientCC->MakeCKKSPackedPlaintext(std::vector({0}, 1.0));
    auto cipherOne       = clientCC->Encrypt(clientPubKey, plainOne);
    return clientCC->EvalSub(cipherOne, EvalCmp_eq(x, y));
}

inline Ciphertext<DCRTPoly> EvalSel(const Ciphertext<DCRTPoly> &cond, const Ciphertext<DCRTPoly> &x, 
                                    const Ciphertext<DCRTPoly> &y) {
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