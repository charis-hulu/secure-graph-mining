#include "openfhe.h"

using namespace lbcrypto;

int main() {


    uint32_t multDepth      = 17;
    uint32_t scaleModSize = 50;
    uint32_t firstModSize = 60;
    uint32_t ringDim      = 8192;
    SecurityLevel sl      = HEStd_NotSet;
    BINFHE_PARAMSET slBin = TOY;
    uint32_t logQ_ccLWE   = 25;
    uint32_t slots        = 1;  // sparsely-packed
    uint32_t batchSize    = slots;

    CCParams<CryptoContextCKKSRNS> parameters;
    parameters.SetMultiplicativeDepth(multDepth);
    parameters.SetScalingModSize(scaleModSize);
    parameters.SetFirstModSize(firstModSize);
    // parameters.SetScalingTechnique(scTech);
    parameters.SetSecurityLevel(sl);
    parameters.SetRingDim(ringDim);
    parameters.SetBatchSize(batchSize);
    parameters.SetSecretKeyDist(UNIFORM_TERNARY);
    parameters.SetKeySwitchTechnique(HYBRID);
    parameters.SetNumLargeDigits(3);

    CryptoContext<DCRTPoly> cc = GenCryptoContext(parameters);

    // Enable the features that you wish to use
    cc->Enable(PKE);
    cc->Enable(KEYSWITCH);
    cc->Enable(LEVELEDSHE);
    cc->Enable(ADVANCEDSHE);
    cc->Enable(SCHEMESWITCH);

    std::cout << "CKKS scheme is using ring dimension " << cc->GetRingDimension();
    std::cout << ", number of slots " << slots << ", and supports a multiplicative depth of " << multDepth << std::endl
              << std::endl;

    // Generate encryption keys
    auto keys = cc->KeyGen();

    // Step 2: Prepare the FHEW cryptocontext and keys for FHEW and scheme switching
    SchSwchParams params;
    params.SetSecurityLevelCKKS(sl);
    params.SetSecurityLevelFHEW(slBin);
    params.SetCtxtModSizeFHEWLargePrec(logQ_ccLWE);
    params.SetNumSlotsCKKS(slots);
    params.SetNumValues(slots);
    auto privateKeyFHEW = cc->EvalSchemeSwitchingSetup(params);
    auto ccLWE          = cc->GetBinCCForSchemeSwitch();

    ccLWE->BTKeyGen(privateKeyFHEW);
    cc->EvalSchemeSwitchingKeyGen(keys, privateKeyFHEW);

    // std::cout << "FHEW scheme is using lattice parameter " << ccLWE->GetParams()->GetLWEParams()->Getn();
    // std::cout << ", logQ " << logQ_ccLWE;
    // std::cout << ", and modulus q " << ccLWE->GetParams()->GetLWEParams()->Getq() << std::endl << std::endl;
    // Set the scaling factor to be able to decrypt; the LWE mod switch is performed on the ciphertext at the last level
    
    auto pLWE1           = ccLWE->GetMaxPlaintextSpace().ConvertToInt();  // Small precision
    auto modulus_LWE     = 1 << logQ_ccLWE;
    auto beta            = ccLWE->GetBeta().ConvertToInt();
    auto pLWE2           = modulus_LWE / (2 * beta);  // Large precision
    double scaleSignFHEW = 1.0;
    cc->EvalCompareSwitchPrecompute(pLWE2, scaleSignFHEW);

    std::cout << "Finish preparation" << std::endl;

    // Step 3: Encoding and encryption of inputs
    std::vector<double> x1(slots, 3.2);
    std::vector<double> x2(slots, 5.25);

    // Encoding as plaintexts
    Plaintext ptxt1 = cc->MakeCKKSPackedPlaintext(x1, 1, 0, nullptr, slots);
    Plaintext ptxt2 = cc->MakeCKKSPackedPlaintext(x2, 1, 0, nullptr, slots);
    std::cout << "Finish encoding" << std::endl;
    // Encrypt the encoded vectors
    auto c1 = cc->Encrypt(keys.publicKey, ptxt1);
    auto c2 = cc->Encrypt(keys.publicKey, ptxt2);
    std::cout << "Finish encryption" << std::endl;

    auto cResult = cc->EvalCompareSchemeSwitching(c1, c2, slots, slots);
    std::cout << "Finish Comparison" << std::endl;
    
    Plaintext plaintextDec3;
    cc->Decrypt(keys.secretKey, cResult, &plaintextDec3);
    
    std::cout << "Finish Decryption" << std::endl;

    plaintextDec3->SetLength(slots);
    std::cout << "Decrypted switched result: " << plaintextDec3 << std::endl;
    return 0;
}