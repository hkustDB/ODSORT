#ifndef BITONIC_H
#define BITONIC_H

#include "shared.h"

class Bitonic {
  public:
    Bitonic(EnclaveServer &eServer, EncOneBlock *a);
    Bitonic(EnclaveServer &eServer, int inputId, int64_t start, int64_t initSize);
    ~Bitonic();
    void smallBitonicMerge(EncOneBlock *a, int64_t start, int64_t size, int flipped);
    void smallBitonicMerge(int64_t start, int64_t size, int flipped);
    void smallBitonicSort(EncOneBlock *a, int64_t start, int64_t size, int flipped);
    void smallBitonicSort(int64_t start, int64_t size, int flipped);
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
};

#endif // !BITONIC_H