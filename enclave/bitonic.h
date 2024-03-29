#ifndef BITONIC_H
#define BITONIC_H

#include "shared.h"
#include <fstream>

class Bitonic {
  public:
    Bitonic(EnclaveServer &eServer);
    Bitonic(EnclaveServer &eServer, int inputId, int64_t start, int64_t initSize);
    Bitonic(EnclaveServer &eServer, EncOneBlock *a, int64_t start, int64_t size);
    ~Bitonic();
    void smallBitonicMerge(EncOneBlock *a, int64_t start, int64_t size, bool flipped, bool isDirect = false);
    void smallBitonicSort(EncOneBlock *a, int64_t start, int64_t size, bool flipped, bool isDirect = false);
    void bitonicMerge(int64_t start, int64_t size, int flipped);
    void bitonicSort(int64_t start, int64_t size, int flipped);

  private:
    int64_t M;
    int B;
    int inputId;
    int64_t start, initSize;
    EncOneBlock *row1;
    EncOneBlock *row2;
    EnclaveServer &eServer;
    EncOneBlock *a;
    EncOneBlock num1, num2;
    bool swap, nswap;

    unsigned char key[32];
    mbedtls_aes_context aes;
    mbedtls_gcm_context gcm;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;
    size_t iv_offset, iv_offset1;
    unsigned char iv[16];
};

#endif // !BITONIC_H
