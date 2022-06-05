#include <openenclave/corelibc/stdlib.h>
#include<iostream>
#include<math.h>
#include <vector>
#include <random>
#include <cassert>
#include<stdio.h>

#include "./include/definitions.h"
#include "../enclave/include/common.h"
#include "osort_t.h"

// Function Declaration
int greatestPowerOfTwoLessThan(int n);
int smallestPowerOfTwoLargerThan(int n);
void smallBitonicMerge(int *a, int start, int size, int flipped);
void smallBitonicSort(int *a, int start, int size, int flipped);
void opOneLinearScanBlock(int index, int* block, size_t blockSize, int structureId, int write);
void bitonicMerge(int structureId, int start, int size, int flipped, int* row1, int* row2);
void bitonicSort(int structureId, int start, int size, int flipped, int* row1, int* row2);
int callSort(int sortId, int structureId);

void padWithDummy(int structureId, int start, int realNum);
bool isTargetBitOne(int randomKey, int iter);
void mergeSplitHelper(Bucket_x *inputBuffer, int inputBufferLen, std::vector<int> &numRows2, int outputId0, int outputId1, int iter, std::vector<int> bucketAddr, int outputStructureId);
void mergeSplit(int inputStructureId, int outputStructureId, int inputId0, int inputId1, int outputId0, int outputId1, std::vector<int> bucketAddr1, std::vector<int> bucketAddr2, std::vector<int> &numRows1, std::vector<int> &numRows2, int iter);
void kWayMergeSort(int inputStructureId, int outputStructureId, std::vector<int> &numRows1, std::vector<int> &numRows2, std::vector<int> bucketAddr);
void swapRow(Bucket_x *a, Bucket_x *b);
bool cmpHelper(Bucket_x *a, Bucket_x *b);
int partition(Bucket_x *arr, int low, int high);
void quickSort(Bucket_x *arr, int low, int high);
void bucketSort(int inputStructureId, int bucketId, int size, int dataStart);
int bucketOSort(int structureId, int size);

struct HeapNode {
  Bucket_x *data;
  int bucketIdx;
  int elemIdx;
};

class Heap {
  HeapNode *harr;
  int heapSize;
  int batchSize;
  
public:
  Heap(HeapNode *a, int size, int bsize) {
    heapSize = size;
    harr = a;
    int i = (heapSize - 1) / 2;
    batchSize = bsize;
    while (i >= 0) {
      Heapify(i);
      i --;
    }
  }
  
  void Heapify(int i) {
    int l = left(i);
    int r = right(i);
    int target = i;
    
    if (l < heapSize && cmpHelper(harr[i].data + harr[i].elemIdx % batchSize, harr[l].data + harr[l].elemIdx % batchSize)) {
      target = l;
    }
    if (r < heapSize && cmpHelper(harr[target].data + harr[target].elemIdx % batchSize, harr[r].data + harr[r].elemIdx % batchSize)) {
      target = r;
    }
    if (target != i) {
      swapHeapNode(&harr[i], &harr[target]);
      Heapify(target);
    }
  }
  
  int left(int i) {
    return (2 * i + 1);
  }
  
  int right (int i) {
    return (2 * i + 2);
  }
  
  void swapHeapNode(HeapNode *a, HeapNode *b) {
    HeapNode temp = *a;
    *a = *b;
    *b = temp;
  }
  
  HeapNode *getRoot() {
    return &harr[0];
  }
  
  int getHeapSize() {
    return heapSize;
  }
  
  bool reduceSizeByOne() {
    oe_free(harr[0].data);
    heapSize --;
    if (heapSize > 0) {
      harr[0] = harr[heapSize];
      Heapify(0);
      return true;
    } else {
      return false;
    }
  }
  
  void replaceRoot(HeapNode x) {
    harr[0] = x;
    Heapify(0);
  }
};

// Print Message
#define DBGprint(...) { \
  fprintf(stderr, "%s: Line %d:\t", __FILE__, __LINE__); \
  fprintf(stderr, __VA_ARGS__); \
  fprintf(stderr, "\n"); \
}

int paddedSize;


// Functions x crossing the enclave boundary
void opOneLinearScanBlock(int index, int* block, size_t blockSize, int structureId, int write) {
  if (!write) {
    OcallReadBlock(index, block, blockSize * structureSize[structureId], structureId);
  } else {
    OcallWriteBlock(index, block, blockSize * structureSize[structureId], structureId);
  }
  return;
}

int greatestPowerOfTwoLessThan(int n) {
	int k = 1;
	while (k > 0 && k < n) {
		k = k << 1;
	}
	return k >> 1;
}

int smallestPowerOfTwoLargerThan(int n) {
  int k = 1;
  while (k > 0 && k < n) {
    k = k << 1;
  }
  return k;
}

void smallBitonicMerge(int *a, int start, int size, int flipped) {
  if (size == 1) {
    return;
  } else {
    int swap = 0;
    int mid = greatestPowerOfTwoLessThan(size);
    for (int i = 0; i < size - mid; ++i) {
      int num1 = a[start + i];
      int num2 = a[start + mid + i];
      swap = num1 > num2;
      swap = swap ^ flipped;
      a[start + i] = (!swap * num1) + (swap * num2);
      a[start + i + mid] = (swap * num1) + (!swap * num2);
    }
    smallBitonicMerge(a, start, mid, flipped);
    smallBitonicMerge(a, start + mid, size - mid, flipped);
  }
  return;
}

//Correct, after testing
void smallBitonicSort(int *a, int start, int size, int flipped) {
  if (size <= 1) {
    return ;
  } else {
    int mid = greatestPowerOfTwoLessThan(size);
    smallBitonicSort(a, start, mid, 1);
    smallBitonicSort(a, start + mid, size - mid, 0);
    smallBitonicMerge(a, start, size, flipped);
  }
  return;
}

// start, size -> BLOCK start, #BLOCK
// call bitonic merge need to initialize row1 & row2 with size BLOCK_DATA_SIZE
void bitonicMerge(int structureId, int start, int size, int flipped, int* row1, int* row2) {
  if (size < 1) {
    return ;
  } else if (size < MEM_IN_ENCLAVE) {
    int *trustedMemory = (int*)oe_malloc(size * BLOCK_DATA_SIZE * sizeof(int));
    for (int i = 0; i < size; ++i) {
      opOneLinearScanBlock((start + i) * BLOCK_DATA_SIZE, &trustedMemory[i * BLOCK_DATA_SIZE], BLOCK_DATA_SIZE, structureId, 0);
    }
    smallBitonicMerge(trustedMemory, 0, size * BLOCK_DATA_SIZE, flipped);
    for (int i = 0; i < size; ++i) {
      opOneLinearScanBlock((start + i) * BLOCK_DATA_SIZE, &trustedMemory[i * BLOCK_DATA_SIZE], BLOCK_DATA_SIZE, structureId, 1);
    }
    oe_free(trustedMemory);
  } else {
    int swap = 0;
    int mid = greatestPowerOfTwoLessThan(size);
    for (int i = 0; i < size - mid; ++i) {
      opOneLinearScanBlock((start + i) * BLOCK_DATA_SIZE, row1, BLOCK_DATA_SIZE, structureId, 0);
      opOneLinearScanBlock((start + mid + i) * BLOCK_DATA_SIZE, row2, BLOCK_DATA_SIZE, structureId, 0);
      int num1 = row1[0], num2 = row2[0];
      swap = num1 > num2;
      swap = swap ^ flipped;
      for (int j = 0; j < BLOCK_DATA_SIZE; ++j) {
        int v1 = row1[j];
        int v2 = row2[j];
        row1[j] = (!swap * v1) + (swap * v2);
        row2[j] = (swap * v1) + (!swap * v2);
      }
      opOneLinearScanBlock((start + i) * BLOCK_DATA_SIZE, row1, BLOCK_DATA_SIZE, structureId, 1);
      opOneLinearScanBlock((start + mid + i) * BLOCK_DATA_SIZE, row2, BLOCK_DATA_SIZE, structureId, 1);
    }
    bitonicMerge(structureId, start, mid, flipped, row1, row2);
    bitonicMerge(structureId, start + mid, size - mid, flipped, row1, row2);
  }
  return;
}

void bitonicSort(int structureId, int start, int size, int flipped, int* row1, int* row2) {
  if (size < 1) {
    return;
  } else if (size < MEM_IN_ENCLAVE) {
    int *trustedMemory = (int*)oe_malloc(size * BLOCK_DATA_SIZE * sizeof(int));
    for (int i = 0; i < size; ++i) {
      opOneLinearScanBlock((start + i) * BLOCK_DATA_SIZE, &trustedMemory[i * BLOCK_DATA_SIZE], BLOCK_DATA_SIZE, structureId, 0);
    }
    smallBitonicSort(trustedMemory, 0, size * BLOCK_DATA_SIZE, flipped);
    // write back
    for (int i = 0; i < size; ++i) {
      opOneLinearScanBlock((start + i) * BLOCK_DATA_SIZE, &trustedMemory[i * BLOCK_DATA_SIZE], BLOCK_DATA_SIZE, structureId, 1);
    }
    oe_free(trustedMemory);
  } else {
    int mid = greatestPowerOfTwoLessThan(size);
    bitonicSort(structureId, start, mid, 1, row1, row2);
    bitonicSort(structureId, start + mid, size - mid, 0, row1, row2);
    bitonicMerge(structureId, start, size, flipped, row1, row2);
  }
  return;
}

void padWithDummy(int structureId, int start, int realNum) {
  //int blockSize = structureSize[structureId];
  int len = BUCKET_SIZE - realNum;
  if (len <= 0) {
    return ;
  }
  Bucket_x *junk = (Bucket_x*)oe_malloc(len * sizeof(Bucket_x));
  // memset(junk, 0xff, blockSize * len);
  for (int i = 0; i < len; ++i) {
    junk[i].x = -1;
    junk[i].key = -1;
  }
  
  opOneLinearScanBlock(2 * (start + realNum), (int*)junk, len, structureId, 1);
  oe_free(junk);
}

bool isTargetBitOne(int randomKey, int iter) {
  assert(iter >= 1);
  return (randomKey & (0x01 << (iter - 1))) == 0 ? false : true;
}

void mergeSplitHelper(Bucket_x *inputBuffer, int inputBufferLen, std::vector<int> &numRows2, int outputId0, int outputId1, int iter, std::vector<int> bucketAddr, int outputStructureId) {
  // write back standard
  int batchSize = 1; // 8192
  Bucket_x *buf0 = (Bucket_x*)oe_malloc(batchSize * sizeof(Bucket_x));
  Bucket_x *buf1 = (Bucket_x*)oe_malloc(batchSize * sizeof(Bucket_x));
  int counter0 = 0, counter1 = 0;
  int randomKey = 0;
  
  for (int i = 0; i < inputBufferLen; ++i) {
    if ((inputBuffer[i].key != DUMMY) && (inputBuffer[i].x != DUMMY)) {
      randomKey = inputBuffer[i].key;
      
      if (isTargetBitOne(randomKey, iter + 1)) {
        buf1[counter1 % batchSize] = inputBuffer[i];
        counter1 ++;
        if (counter1 % batchSize == 0) {
          // int start = (counter1 / batchSize - 1) * batchSize;
          opOneLinearScanBlock(2 * (bucketAddr[outputId1] +  numRows2[outputId1]), (int*)buf1, (size_t)batchSize, outputStructureId, 1);
          numRows2[outputId1] += batchSize;
          memset(buf1, NULLCHAR, batchSize * sizeof(Bucket_x));
        }
      } else {
        buf0[counter0 % batchSize] = inputBuffer[i];
        counter0 ++;
        if (counter0 % batchSize == 0) {
          // int start = (counter0 / batchSize - 1) * batchSize;
          opOneLinearScanBlock(2 * (bucketAddr[outputId0] + numRows2[outputId0]), (int*)buf0, (size_t)batchSize, outputStructureId, 1);
          numRows2[outputId0] += batchSize;
          memset(buf0, NULLCHAR, batchSize * sizeof(Bucket_x));
        }
      }
    }
  }
  
  // int start = (counter1 / batchSize - 1) * batchSize;
  opOneLinearScanBlock(2 * (bucketAddr[outputId1] + numRows2[outputId1]), (int*)buf1, (size_t)(counter1 % batchSize), outputStructureId, 1);
  numRows2[outputId1] += counter1 % batchSize;
  // start = (counter0 / batchSize - 1) * batchSize;
  opOneLinearScanBlock(2 * (bucketAddr[outputId0] + numRows2[outputId0]), (int*)buf0, (size_t)(counter0 % batchSize), outputStructureId, 1);
  numRows2[outputId0] += counter0 % batchSize;
  
  oe_free(buf0);
  oe_free(buf1);
}

// inputId: start index for inputStructureId
// numRow1: input bucket length, numRow2: output bucket length
void mergeSplit(int inputStructureId, int outputStructureId, int inputId0, int inputId1, int outputId0, int outputId1, std::vector<int> bucketAddr1, std::vector<int> bucketAddr2, std::vector<int> &numRows1, std::vector<int> &numRows2, int iter) {
  Bucket_x *inputBuffer = (Bucket_x*)oe_malloc(sizeof(Bucket_x) * BUCKET_SIZE);
  // BLOCK#0
  opOneLinearScanBlock(2 * bucketAddr1[inputId0], (int*)inputBuffer, BUCKET_SIZE, inputStructureId, 0);
  mergeSplitHelper(inputBuffer, numRows1[inputId0], numRows2, outputId0, outputId1, iter, bucketAddr2, outputStructureId);
  if (numRows2[outputId0] > BUCKET_SIZE || numRows2[outputId1] > BUCKET_SIZE) {
    DBGprint("overflow error during merge split!\n");
  }
  
  // BLOCK#1
  opOneLinearScanBlock(2 * bucketAddr1[inputId1], (int*)inputBuffer, BUCKET_SIZE, inputStructureId, 0);
  mergeSplitHelper(inputBuffer, numRows1[inputId1], numRows2, outputId0, outputId1, iter, bucketAddr2, outputStructureId);
  
  if (numRows2[outputId0] > BUCKET_SIZE || numRows2[outputId1] > BUCKET_SIZE) {
    DBGprint("overflow error during merge split!\n");
  }
  
  padWithDummy(outputStructureId, bucketAddr2[outputId1], numRows2[outputId1]);
  padWithDummy(outputStructureId, bucketAddr2[outputId0], numRows2[outputId0]);
  
  oe_free(inputBuffer);
}

void kWayMergeSort(int inputStructureId, int outputStructureId, std::vector<int> &numRows1, std::vector<int> &numRows2, std::vector<int> bucketAddr) {
  int mergeSortBatchSize = 16; // 256
  int writeBufferSize = 32; // 8192
  int numWays = (int)numRows1.size();
  HeapNode inputHeapNodeArr[numWays];
  int totalCounter = 0;
  std::vector<int> readBucketAddr(bucketAddr);
  int writeBucketAddr = 0;
  
  for (int i = 0; i < numWays; ++i) {
    HeapNode node;
    node.data = (Bucket_x*)oe_malloc(mergeSortBatchSize * sizeof(Bucket_x));
    node.bucketIdx = i;
    node.elemIdx = 0;
    opOneLinearScanBlock(2 * readBucketAddr[i], (int*)node.data, (size_t)std::min(mergeSortBatchSize, numRows1[i]), inputStructureId, 0);
    inputHeapNodeArr[i] = node;
    // Update data count
    readBucketAddr[i] += std::min(mergeSortBatchSize, numRows1[i]);
  }
  
  Heap heap(inputHeapNodeArr, numWays, mergeSortBatchSize);
  // DBGprint("init heap success");
  Bucket_x *writeBuffer = (Bucket_x*)oe_malloc(writeBufferSize * sizeof(Bucket_x));
  int writeBufferCounter = 0;

  while (1) {
    HeapNode *temp = heap.getRoot();
    memcpy(writeBuffer + writeBufferCounter, temp->data + temp->elemIdx % mergeSortBatchSize, sizeof(Bucket_x));
    writeBufferCounter ++;
    totalCounter ++;
    temp->elemIdx ++;
    
    if (writeBufferCounter == writeBufferSize) {
      opOneLinearScanBlock(2 * writeBucketAddr, (int*)writeBuffer, (size_t)writeBufferSize, outputStructureId, 1);
      writeBucketAddr += writeBufferSize;
      numRows2[temp->bucketIdx] += writeBufferSize;
      writeBufferCounter = 0;
    }
    // re-get bucketIdx mergeSortBatchSize data, juct compare certain index data
    if (temp->elemIdx < numRows1[temp->bucketIdx] && (temp->elemIdx % mergeSortBatchSize) == 0) {
      opOneLinearScanBlock(2 * readBucketAddr[temp->bucketIdx], (int*)(temp->data), (size_t)std::min(mergeSortBatchSize, numRows1[temp->bucketIdx]-temp->elemIdx), inputStructureId, 0);
      
      readBucketAddr[temp->bucketIdx] += std::min(mergeSortBatchSize, numRows1[temp->bucketIdx]-temp->elemIdx);
      heap.Heapify(0);
      // one bucket data is empty
    } else if (temp->elemIdx >= numRows1[temp->bucketIdx]) {
      bool res = heap.reduceSizeByOne();
      if (!res) {
        break;
      }
    } else {
      heap.Heapify(0);
    }
  }
  opOneLinearScanBlock(2 * writeBucketAddr, (int*)writeBuffer, (size_t)writeBufferCounter, outputStructureId, 1);
  numRows2[-1] += writeBufferCounter;
  oe_free(writeBuffer);
}

void swapRow(Bucket_x *a, Bucket_x *b) {
  Bucket_x *temp = (Bucket_x*)oe_malloc(sizeof(Bucket_x));
  memmove(temp, a, sizeof(Bucket_x));
  memmove(a, b, sizeof(Bucket_x));
  memmove(b, temp, sizeof(Bucket_x));
  oe_free(temp);
}

bool cmpHelper(Bucket_x *a, Bucket_x *b) {
  return (a->x > b->x) ? true : false;
}

int partition(Bucket_x *arr, int low, int high) {
  Bucket_x *pivot = arr + high;
  int i = low - 1;
  for (int j = low; j <= high - 1; ++j) {
    if (cmpHelper(pivot, arr + j)) {
      i++;
      if (i != j) {
        swapRow(arr + i, arr + j);
      }
    }
  }
  if (i + 1 != high) {
    swapRow(arr + i + 1, arr + high);
  }
  return (i + 1);
}

void quickSort(Bucket_x *arr, int low, int high) {
  if (high > low) {
    int mid = partition(arr, low, high);
    quickSort(arr, low, mid - 1);
    quickSort(arr, mid + 1, high);
  }
}

void bucketSort(int inputStructureId, int bucketId, int size, int dataStart) {
  Bucket_x *arr = (Bucket_x*)oe_malloc(BUCKET_SIZE * sizeof(Bucket_x));
  opOneLinearScanBlock(2 * dataStart, (int*)arr, (size_t)size, inputStructureId, 0);
  quickSort(arr, 0, size - 1);
  opOneLinearScanBlock(2 * dataStart, (int*)arr, (size_t)size, inputStructureId, 1);
  oe_free(arr);
}

// size = #inputs real size
int bucketOSort(int structureId, int size) {
  int bucketNum = smallestPowerOfTwoLargerThan(ceil(2.0 * size / BUCKET_SIZE));
  // paddedSize = bucketNum * BUCKET_SIZE;
  std::vector<int> bucketAddr1(bucketNum, -1);
  std::vector<int> bucketAddr2(bucketNum, -1);
  std::vector<int> numRows1(bucketNum, 0);
  std::vector<int> numRows2(bucketNum, 0);
  
  int ranBinAssignIters = log2(bucketNum) - 1;
  
  for (int i = 0; i < bucketNum; ++i) {
    bucketAddr1[i] = i * BUCKET_SIZE;
    bucketAddr2[i] = i * BUCKET_SIZE;
  }
  
  // TODO: Assign each element in X a uniformly random key (Finished)
  srand((unsigned)time(NULL));
  Bucket_x *trustedMemory = (Bucket_x*)oe_malloc(BLOCK_DATA_SIZE * sizeof(Bucket_x));
  int *inputTrustMemory = (int*)oe_malloc(BLOCK_DATA_SIZE * sizeof(int));
  
  int total = 0;
  
  // read input & generate randomKey, write to designed bucket
  for (int i = 0; i < size; i += BLOCK_DATA_SIZE) {
    // TODO: 973 later x become all 0
    opOneLinearScanBlock(i, inputTrustMemory, std::min(BLOCK_DATA_SIZE, size - i), structureId - 1, 0);
    int randomKey;
    for (int j = 0; j < std::min(BLOCK_DATA_SIZE, size - i); ++j) {
      oe_random(&randomKey, 4);
      trustedMemory[j].x = inputTrustMemory[j];
      trustedMemory[j].key = randomKey;
      // TODO: improve
      int offset = bucketAddr1[(i + j) % bucketNum] + numRows1[(i + j) % bucketNum];
      opOneLinearScanBlock(offset * 2, (int*)(&trustedMemory[j]), (size_t)1, structureId, 1);
      /*
      std::cout<<"=====WriteB0=====\n";
      std::cout<<(size_t)1 * sizeof(Bucket_x)<<std::endl;
      std::cout<<trustedMemory[j].x <<" "<<trustedMemory[j].key<<std::endl;
      std::cout<<"=====WriteB0=====\n";*/
      // paddedSize = bucketNum * BUCKET_SIZE;
      numRows1[(i + j) % bucketNum] ++;
    }
    total += std::min(BLOCK_DATA_SIZE, size - i);
  }
  oe_free(trustedMemory);
  oe_free(inputTrustMemory);

  std::cout << "=======Initial0=======\n";
  paddedSize = bucketNum * BUCKET_SIZE;
  print(structureId, paddedSize);
  std::cout << "=======Initial0=======\n";

  for (int i = 0; i < bucketNum; ++i) {
    DBGprint("currently bucket %d has %d records/%d", i, numRows1[i], BUCKET_SIZE);
    padWithDummy(structureId, bucketAddr1[i], numRows1[i]);
    
  }
  
  std::cout << "=======Initial=======\n";
  paddedSize = bucketNum * BUCKET_SIZE;
  print(structureId, paddedSize);
  std::cout << "=======Initial=======\n";
  
  for (int i = 0; i < ranBinAssignIters; ++i) {
    // data in Array1, update numRows2
    if (i % 2 == 0) {
      for (int j = 0; j < bucketNum / 2; ++j) {
        int jj = (j / (int)pow(2, i)) * (int)pow(2, i);
        mergeSplit(structureId, structureId + 1, j + jj, j + jj + (int)pow(2, i), 2 * j, 2 * j + 1, bucketAddr1, bucketAddr2, numRows1, numRows2, i);
      }
      int count = 0;
      for (int k = 0; k < bucketNum; ++k) {
        numRows1[k] = 0;
        count += numRows2[k];
      }
      DBGprint("after %dth merge split, we have %d tuples\n", i, count);
    } else {
      for (int j = 0; j < bucketNum / 2; ++j) {
        int jj = (j / (int)pow(2, i)) * (int)pow(2, i);
        // TODO: scan
        mergeSplit(structureId + 1, structureId, j + jj, j + jj + (int)pow(2, i), 2 * j, 2 * j + 1, bucketAddr2, bucketAddr1, numRows2, numRows1, i);
      }
      int count = 0;
      for (int k = 0; k < bucketNum; ++k) {
        numRows2[k] = 0;
        count += numRows1[k];
      }
      DBGprint("after %dth merge split, we have %d tuples\n", i, count);
    }
    DBGprint("\n\n Finish random bin assignment iter%dth out of %d\n\n", i, ranBinAssignIters);
  }
  
  int resultId = 0;
  if (ranBinAssignIters % 2 == 0) {
    std::cout << "=======MergeSplit0=======\n";
    paddedSize = bucketNum * BUCKET_SIZE;
    print(structureId, paddedSize);
    std::cout << "=======MergeSplit0=======\n";
    for (int i = 0; i < bucketNum; ++i) {
      bucketSort(structureId, i, numRows1[i], bucketAddr1[i]);
    }
    std::cout << "=======bucketSort0=======\n";
    paddedSize = bucketNum * BUCKET_SIZE;
    print(structureId, paddedSize);
    std::cout << "=======bucketSort0=======\n";
    kWayMergeSort(structureId, structureId + 1, numRows1, numRows2, bucketAddr1);
    std::cout << "=======mergeSort0=======\n";
    paddedSize = bucketNum * BUCKET_SIZE;
    print(structureId + 1, paddedSize);
    std::cout << "=======mergeSort0=======\n"; 
    resultId = structureId + 1;
  } else {
    std::cout << "=======MergeSplit1=======\n";
    paddedSize = bucketNum * BUCKET_SIZE;
    print(structureId + 1, paddedSize);
    std::cout << "=======MergeSplit1=======\n"; 
    for (int i = 0; i < bucketNum; ++i) {
      bucketSort(structureId + 1, i, numRows2[i], bucketAddr2[i]);
    }
    std::cout << "=======bucketSort1=======\n";
    paddedSize = bucketNum * BUCKET_SIZE;
    print(structureId + 1, paddedSize);
    std::cout << "=======bucketSort1=======\n";
    kWayMergeSort(structureId + 1, structureId, numRows2, numRows1, bucketAddr2);
    std::cout << "=======mergeSort1=======\n";
    paddedSize = bucketNum * BUCKET_SIZE;
    print(structureId, paddedSize);
    std::cout << "=======mergeSort1=======\n"; 
    resultId = structureId;
  }
  paddedSize = N;
  test(resultId, paddedSize);
  return resultId;
}

// trusted function
int callSort(int sortId, int structureId, int paddedSize) {
  // bitonic sort
  int size = paddedSize / BLOCK_DATA_SIZE;
  printf("size: %d %d\n", paddedSize, size);
  if (sortId == 1) {
    return bucketOSort(structureId, N);
  }
  if (sortId == 3) {
    int *row1 = (int*)oe_malloc(BLOCK_DATA_SIZE * sizeof(int));
    int *row2 = (int*)oe_malloc(BLOCK_DATA_SIZE * sizeof(int));
    bitonicSort(structureId, 0, size, 0, row1, row2);
    return -1;
  }
  return -1;
}

// trusted function
void callSmallSort(int *a, size_t dataSize) {
  int size = (int)dataSize;
  smallBitonicSort(a, 0, size, 0);
  return;
}