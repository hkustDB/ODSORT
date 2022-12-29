#ifndef IOTOOLS_H
#define IOTOOLS_H

#include <cstdint>
#include <iostream>
#include <vector>
#include <fstream>
#include <random>
#include <chrono>
#include <unordered_map>
#include <omp.h>

#include <boost/sort/sort.hpp>
#include <boost/math/tools/roots.hpp>
#include <boost/math/special_functions/pow.hpp>

#include <common.h>

class DataStore {
  public:
    DataStore(EncOneBlock **arrayAddr, int64_t N, int64_t M, int B);
    ~DataStore();
    void init(int structureId, int64_t size);
    void print(int structureId, int64_t size, OutputType outputtype, const char* filepath);
    void test(int structureId, int64_t size, SortType sorttype);
    int64_t RandRange(int64_t start, int64_t end);
  private:
    EncOneBlock **arrayAddr;
    vector<int> delArray;
    int64_t N, M;
    int B;
    char *filepath = "/home/chenbingnan/mysamples/OQSORT/out.txt";
    OutputType type = FILEOUT;
    std::random_device dev;
    std::mt19937 rng(dev()); 
};

#endif // !IOTOOLS_H
