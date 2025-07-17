// mpi_utils.hpp
#pragma once

#include <mpi.h>
#include <string>
#include <vector>
#include <sstream>
#include "cereal/archives/binary.hpp"
#include "key/key-ser.h"
#include "scheme/ckksrns/ckksrns-ser.h"
// #include "openfhe.h"
// #include "utils/serial.h"

// CEREAL_REGISTER_TYPE(lbcrypto::CryptoParametersCKKSRNS);
// CEREAL_REGISTER_TYPE(lbcrypto::SWITCHCKKSRNS);
// CEREAL_REGISTER_TYPE(lbcrypto::SchemeCKKSRNS);
// CEREAL_REGISTER_TYPE(lbcrypto::EvalKeyRelinImpl<lbcrypto::DCRTPolyImpl<bigintdyn::mubintvec<bigintdyn::ubint<unsigned long>>>>)
// CEREAL_REGISTER_TYPE(lbcrypto::CiphertextImpl<lbcrypto::DCRTPoly>);

namespace MPIUtils {
    
    template <typename T>
    string serializeData(T &data) {
        // cout << "SERIALIZING DATA..." << endl;
        std::stringstream ss;
        { //Needed for RAII in Cereal
            cereal::BinaryOutputArchive archive( ss );
            archive( data );
            // archive(data);
        }
        string serialized = ss.str();
        return serialized;
    }

    template <typename T>
    T deserializeData(const std::string& serialized) {
        T data;
        std::stringstream ss(serialized);
        {
            cereal::BinaryInputArchive archive(ss);
            archive(data);
        }
        return data;
    }

    template <typename T>
    vector<T> flattenRelation(Rel<T> rel) {

        vector<T> flat_relation;
        for (auto& [key, valset] : rel.data) {
            for (auto& val : valset) {
                flat_relation.push_back(key);
                flat_relation.push_back(val);
            }
        }
        return flat_relation;
    }


    template <typename T>
    vector<string> serializeVectorElements(vector<T> vec) {
        
        vector<string> string_vector;
        
        int size = vec.size();
        for(int i=0; i<size; i++) {
            string ss = serializeData<T>(vec[i]);
            string_vector.push_back(ss); 
        }
        

        return string_vector;
    }

    template <typename T>
    vector<T> deserializeVectorElements(vector<string> string_vec) {
        
        vector<T> vec;
        
        int size = string_vec.size();
        for(int i = 0; i < size; i++) {
            T element = deserializeData<T>(string_vec[i]);
            vec.push_back(element); 
        }

        return vec;
    }

}