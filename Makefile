try_compare: try_compare.cpp
	g++ try_compare.cpp \
    -I /usr/local/include/openfhe \
    -I /usr/local/include/openfhe/core \
    -I /usr/local/include/openfhe/pke \
    -I /usr/local/include/openfhe/binfhe \
    /usr/local/lib/libOPENFHEcore.so \
    /usr/local/lib/libOPENFHEpke.so \
    /usr/local/lib/libOPENFHEbinfhe.so \
    -o try_compare


compare: fhe.cpp
	g++ fhe.cpp \
    -I /usr/local/include/openfhe \
    -I /usr/local/include/openfhe/core \
    -I /usr/local/include/openfhe/pke \
    -I /usr/local/include/openfhe/binfhe \
    /usr/local/lib/libOPENFHEcore.so \
    /usr/local/lib/libOPENFHEpke.so \
    /usr/local/lib/libOPENFHEbinfhe.so \
    -o fhe

simulate: simulate.cpp fhe.hpp mpi_utils.hpp serial_tc.hpp serial_tc.tpp
	mpic++ simulate.cpp \
    -I /usr/local/include/openfhe \
    -I /usr/local/include/openfhe/core \
    -I /usr/local/include/openfhe/pke \
    -I /usr/local/include/openfhe/binfhe \
    /usr/local/lib/libOPENFHEcore.so \
    /usr/local/lib/libOPENFHEpke.so \
    /usr/local/lib/libOPENFHEbinfhe.so \
    -o simulate

clean:
	rm compare