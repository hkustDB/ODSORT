# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.
include ../../config.mk

OE_CRYPTO_LIB := mbedtls

CFLAGS=$(shell pkg-config oeenclave-$(C_COMPILER) --cflags)
CXXFLAGS=$(shell pkg-config oeenclave-$(CXX_COMPILER) --cflags)
LDFLAGS=$(shell pkg-config oeenclave-$(CXX_COMPILER) --libs)
INCDIR=$(shell pkg-config oeenclave-$(C_COMPILER) --variable=includedir)
CRYPTO_LDFLAGS=$(shell pkg-config oeenclave-$(COMPILER) --variable=${OE_CRYPTO_LIB}libs)

all:
	$(MAKE) build
	$(MAKE) keys
	$(MAKE) sign

build:
	@ echo "Compilers used: $(CC), $(CXX)"
	oeedger8r ../oqsort.edl --trusted \
		--search-path $(INCDIR) \
		--search-path $(INCDIR)/openenclave/edl/sgx
	@ echo "FLAGS: $(LDFLAGS) $(CRYPTO_LDFLAGS)"
	$(CXX) -g -c $(CXXFLAGS) -DOE_API_VERSION=2 bitonic.cpp
	$(CXX) -g -c $(CXXFLAGS) -DOE_API_VERSION=2 bucket.cpp
	$(CXX) -g -c $(CXXFLAGS) -DOE_API_VERSION=2 quick.cpp
	$(CXX) -g -c $(CXXFLAGS) -DOE_API_VERSION=2 oq.cpp
	$(CXX) -g -c $(CXXFLAGS) -DOE_API_VERSION=2 shared.cpp
	$(CXX) -g -c $(CXXFLAGS) -DOE_API_VERSION=2 ../include/common.cpp
	$(CXX) -g -c $(CXXFLAGS) -DOE_API_VERSION=2 enc.cpp
	$(CC) -g -c $(CFLAGS) -DOE_API_VERSION=2 oqsort_t.c
	$(CXX) -o oqsortenc enc.o bitonic.o bucket.o quick.o oq.o shared.o common.o oqsort_t.o $(LDFLAGS) $(CRYPTO_LDFLAGS)

sign:
	oesign sign -e oqsortenc -c oqsort.conf -k private.pem

clean:
	rm -f *.o oqsortenc oqsortenc.signed private.pem public.pem oqsort_t.* oqsort_args.h

keys:
	openssl genrsa -out private.pem -3 3072
	openssl rsa -in private.pem -pubout -out public.pem