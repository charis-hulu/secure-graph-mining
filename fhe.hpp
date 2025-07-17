#ifndef FHE_HPP
#define FHE_HPP

#include "openfhe.h"

#include <vector>
#include <iostream>


#include "lattice/lat-hal.h"
#include "math/distrgen.h"
#include "math/matrix.h"
#include "math/nbtheory.h"
#include "testdefs.h"
#include "utils/serial.h"
#include "utils/utilities.h"

using namespace lbcrypto;

class FHEHandler {
public:
    FHEHandler(uint32_t multDepth, uint32_t scaleModSize, uint32_t firstModSize, 
                          uint32_t ringDim, SecurityLevel sl, BINFHE_PARAMSET slBin, uint32_t logQ_ccLWE, uint32_t slots)
        : multDepth(multDepth), scaleModSize(scaleModSize), firstModSize(firstModSize),
          ringDim(ringDim), sl(sl), slBin(slBin), logQ_ccLWE(logQ_ccLWE), slots(slots) {
        Init();
    }

    Ciphertext<DCRTPoly> IsEqual(Ciphertext<DCRTPoly> c1, Ciphertext<DCRTPoly> c2) {
        Ciphertext<DCRTPoly> comp_1 = cc->EvalCompareSchemeSwitching(c1, c2, slots, slots);
        Ciphertext<DCRTPoly> comp_2 = cc->EvalCompareSchemeSwitching(c2, c1, slots, slots);

        std::vector<double> minus{-1};

        Ciphertext<DCRTPoly> cipher_minus = Encrypt(minus);  

        Ciphertext<DCRTPoly> result = Add(comp_1, comp_2);
        result = Add(result, cipher_minus);
        result = Mult(result, cipher_minus);
        
        return result;
    }

    Ciphertext<DCRTPoly> Encrypt(const std::vector<double>& data) {

        if (data.size() != slots) {
            throw std::runtime_error("Input vector size must match number of slots.");
        }
        
        Plaintext ptxt = cc->MakeCKKSPackedPlaintext(data);
        Ciphertext<DCRTPoly> cptxt = cc->Encrypt(keys.publicKey, ptxt);
        return cptxt;
    }


    std::vector<double> Decrypt(Ciphertext<DCRTPoly> ciphertext) {
        Plaintext result;
        cc->Decrypt(keys.secretKey, ciphertext, &result);
        result->SetLength(slots);
        return result->GetRealPackedValue();
    }



    // Ciphertext<DCRTPoly> sort(
    //     Ciphertext<DCRTPoly> c,
    //     const size_t vectorLength,
    //     double leftBoundC,
    //     double rightBoundC,
    //     uint32_t degreeC,
    //     uint32_t degreeI
    // )
    // {
    //     Ciphertext<DCRTPoly> VR = replicateRow(c, vectorLength);
    //     Ciphertext<DCRTPoly> VC = replicateColumn(transposeRow(c, vectorLength, true), vectorLength);
    //     Ciphertext<DCRTPoly> C = compare(
    //         VR, VC,
    //         leftBoundC, rightBoundC,
    //         degreeC
    //     );
    //     Ciphertext<DCRTPoly> R = sumRows(C, vectorLength);

    //     std::vector<double> subMask(vectorLength * vectorLength);
    //     for (size_t i = 0; i < vectorLength; i++)
    //         for (size_t j = 0; j < vectorLength; j++)
    //             subMask[i * vectorLength + j] = -1.0 * i - 0.5;
    //     Ciphertext<DCRTPoly> M = indicator(
    //         R + subMask,
    //         -0.5, 0.5,
    //         -1.0 * vectorLength, 1.0 * vectorLength,
    //         degreeI
    //     );

    //     Ciphertext<DCRTPoly> S = sumColumns(M * VR, vectorLength);

    //     return S;
    // }

    Ciphertext<DCRTPoly> Add(Ciphertext<DCRTPoly> c1, Ciphertext<DCRTPoly> c2) {
        return cc->EvalAdd(c1, c2);
    }

    Ciphertext<DCRTPoly> Mult(Ciphertext<DCRTPoly> c1, Ciphertext<DCRTPoly> c2) {
        return cc->EvalMult(c1, c2);
    }

    void PrintConfig() {
        std::cout << "CKKS scheme: ringDim = " << cc->GetRingDimension()
                  << ", slots = " << slots
                  << ", multDepth = " << multDepth << std::endl;
    }

private:
    CryptoContext<DCRTPoly> cc;
    KeyPair<DCRTPoly> keys;
    std::shared_ptr<BinFHEContext> ccLWE;

    uint32_t multDepth, scaleModSize, firstModSize, ringDim, logQ_ccLWE, slots;
    SecurityLevel sl;
    BINFHE_PARAMSET slBin;

    void Init() {
        CCParams<CryptoContextCKKSRNS> params;
        params.SetMultiplicativeDepth(multDepth);
        params.SetScalingModSize(scaleModSize);
        params.SetFirstModSize(firstModSize);
        params.SetSecurityLevel(sl);
        params.SetRingDim(ringDim);
        params.SetBatchSize(slots);
        params.SetSecretKeyDist(UNIFORM_TERNARY);
        params.SetKeySwitchTechnique(HYBRID);
        params.SetNumLargeDigits(3);

        cc = GenCryptoContext(params);

        cc->Enable(PKE);
        cc->Enable(KEYSWITCH);
        cc->Enable(LEVELEDSHE);
        cc->Enable(ADVANCEDSHE);
        cc->Enable(SCHEMESWITCH);

        keys = cc->KeyGen();

        SchSwchParams switchParams;
        switchParams.SetSecurityLevelCKKS(sl);
        switchParams.SetSecurityLevelFHEW(slBin);
        switchParams.SetCtxtModSizeFHEWLargePrec(logQ_ccLWE);
        switchParams.SetNumSlotsCKKS(slots);
        switchParams.SetNumValues(slots);

        auto privateKeyFHEW = cc->EvalSchemeSwitchingSetup(switchParams);
        ccLWE = cc->GetBinCCForSchemeSwitch();
        ccLWE->BTKeyGen(privateKeyFHEW);
        cc->EvalSchemeSwitchingKeyGen(keys, privateKeyFHEW);

        auto beta = ccLWE->GetBeta().ConvertToInt();
        auto modulus_LWE = 1 << logQ_ccLWE;
        auto pLWE2 = modulus_LWE / (2 * beta);
        double scaleSignFHEW = 1.0;
        cc->EvalCompareSwitchPrecompute(pLWE2, scaleSignFHEW);

        std::cout << "Initialization complete.\n";
    }
};

#endif

// int main() {
//     FHEHandler handler(
//         /* multDepth = */ 17,
//         /* scaleModSize = */ 50,
//         /* firstModSize = */ 60,
//         /* ringDim = */ 8192,
//         /* CKKS security level */ HEStd_NotSet,
//         /* FHEW security level */ TOY,
//         /* logQ for LWE */ 25,
//         /* slots */ 1
//     );

//     handler.PrintConfig();

//     std::vector<double> a{100};
//     std::vector<double> b{100};



//     auto c1 = handler.Encrypt(a);
//     auto c2 = handler.Encrypt(b);

//     auto result = handler.IsEqual(c1, c2);
//     auto decrypted = handler.Decrypt(result);

//     std::cout << "Comparison result: ";
//     for (const auto& val : decrypted) {
//         std::cout << val << " ";
//     }
//     std::cout << std::endl;


//     // auto sumResult = handler.Add(c1, c2);
//     // auto decryptedSum = handler.Decrypt(sumResult);

//     // std::cout << "Addition result: ";
//     // for (const auto& val : decryptedSum) {
//     //     std::cout << val << " ";
//     // }
//     // std::cout << std::endl;

//     return 0;
// }
