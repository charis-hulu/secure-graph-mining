#include <mpi.h>
#include <iostream>
#include <string>
#include <cstring>
#include "serial_tc.hpp"
#include "fhe.hpp"
#include <cmath> 


// #include <cereal/types/vector.hpp>
// #include <cereal/types/string.hpp>
// #include <cereal/archives/binary.hpp>
#include <sstream>
#include <string>


#include "mpi_utils.hpp"

// CEREAL_REGISTER_POLYMORPHIC_RELATION(lbcrypto::Ciphertext<lbcrypto::DCRTPoly>, lbcrypto::CiphertextImpl<lbcrypto::DCRTPoly>);


using namespace std;
using namespace lbcrypto;



class Client {
    public:
        int rank;
        int size;
        MPI_Comm comm;
        
        Rel<int> G;
        Rel<int> decGraph;

        Rel<Ciphertext<DCRTPoly>> encGraph;

        FHEHandler* handler; 

        Client(int rank_, int size_, MPI_Comm comm_) {
            rank = rank_;
            size = size_;
            comm = comm_;

            handler = new FHEHandler(
                /* multDepth = */ 17,
                /* scaleModSize = */ 50,
                /* firstModSize = */ 60,
                /* ringDim = */ 8192,
                /* CKKS security level */ HEStd_NotSet,
                /* FHEW security level */ TOY,
                /* logQ for LWE */ 25,
                /* slots */ 1
            );

        }

        void init_graph() {
            
            G.insert(0, 1);
            G.insert(0, 2);
            G.insert(1, 3);
            G.insert(2, 3);
            G.insert(3, 4);
            G.print_data();

        };

        void print_test() {
            cout << "Hello from client " << rank << endl;
        }

        void print_graph() {
            G.print_data();
        }
        void print_decrypted_graph() {
            decGraph.print_data();
        }

        void encrypt_graph() {
            cout << "Encrypting..." << endl;
            int src;
            set<int> val;
            Ciphertext<DCRTPoly> encSrc;
            Ciphertext<DCRTPoly> encDes;

            for (auto x = G.data.begin(); x != G.data.end(); x++) {
                src = x->first;
                val = x->second;
                
                vector<double> srcVector{src};
                encSrc = handler->Encrypt(srcVector);
                for(auto des = val.begin(); des != val.end(); des++) {
                    vector<double> desVector{*des};
                    encDes = handler->Encrypt(desVector);
                    encGraph.insert(encSrc, encDes);
                }
            }
        }

        void decrypt_graph() {
            cout << "Decrypting..." << endl;
            Ciphertext<DCRTPoly> src;
            set<Ciphertext<DCRTPoly>> val;

            vector<double> decSrcVector;
            vector<double> decDesVector;

            for (auto x = encGraph.data.begin(); x != encGraph.data.end(); x++) {
                src = x->first;
                val = x->second;
                decSrcVector = handler->Decrypt(src);
                for(auto des = val.begin(); des != val.end(); des++) {
                    decDesVector = handler->Decrypt(*des);
                    decGraph.insert(round(decSrcVector[0]), round(decDesVector[0]));
                }
            }
        }


        void sendEncryptedGraph() {

            vector<Ciphertext<DCRTPoly>> flatGraph = MPIUtils::flattenRelation<Ciphertext<DCRTPoly>>(encGraph);
            
            vector<string> serializedGraph = MPIUtils::serializeVectorElements<Ciphertext<DCRTPoly>>(flatGraph);

            int count = serializedGraph.size();
            vector<int> lengths(count);
            int total_len = 0;

            for (int i = 0; i < count; ++i) {
                lengths[i] = serializedGraph[i].size();
                total_len += lengths[i];
            }

            vector<char> buffer(total_len);
            int pos = 0;
            for (const auto& s : serializedGraph) {
                memcpy(&buffer[pos], s.data(), s.size());
                pos += s.size();
            }

            // Send metadata first (lengths)
            MPI_Send(&count, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
            MPI_Send(lengths.data(), count, MPI_INT, 1, 1, MPI_COMM_WORLD);
            
            // Send the actual data
            cout << "buffer size: " << total_len << endl;

            MPI_Send(buffer.data(), total_len, MPI_CHAR, 1, 2, MPI_COMM_WORLD);


            // vector<DCRTPoly> elements = encA->GetElements();


            // int count = elements.size();
            // cout << "Client: cipher size = " << count << endl;

            // string serialized = MPIUtils::serializeData<Ciphertext<DCRTPoly>>(encA);
            // int size = serialized.size();
            // cout << "Client: serialize size = " << size << endl;

            // MPI_Send(&size, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
    
            // MPI_Send(serialized.data(), size, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
            
        }


        void receiveEncryptedGraph() {
            MPI_Status status;

            // 1. Receive the size first
            int size = 0;
            MPI_Recv(&size, 1, MPI_INT, 1, 1, MPI_COMM_WORLD, &status);
            
            cout << "Size after: " << size << endl;
            // 2. Allocate buffer for serialized data
            std::vector<char> buffer(size);

            // 3. Receive the serialized data
            
            MPI_Recv(buffer.data(), size, MPI_CHAR, 1, 1, MPI_COMM_WORLD, &status);
            

            // // 4. Convert to std::string for deserialization
            // std::string serialized(buffer.begin(), buffer.end());

            // // 5. Deserialize to get Ciphertext<DCRTPoly>
            // Ciphertext<DCRTPoly> test_data = MPIUtils::deserializeData<Ciphertext<DCRTPoly>>(serialized);

            // double decrypted_test_data = handler->Decrypt(test_data)[0];

            // cout << "Decrypted test: " << decrypted_test_data << endl;

        }
        // void sendEncryptedGraph(int targetRank = 0) {
        //     // Serialize each (src, dest) pair into strings
        //     vector<string> serialized_data;
        //     for (auto& [src, dsts] : encGraph.data) {
        //         string src_ser = handler->SerializeCiphertext(src);
        //         for (auto& dst : dsts) {
        //             string dst_ser = handler->SerializeCiphertext(dst);
        //             serialized_data.push_back(src_ser);
        //             serialized_data.push_back(dst_ser);
        //         }
        //     }
        // }

};

class Server {
    public:
        int rank;
        int size;
        MPI_Comm comm;
        
        Rel<Ciphertext<DCRTPoly>> G;
        // Ciphertext<DCRTPoly> test_data;

        Server(int rank_, int size_, MPI_Comm comm_) {
            rank = rank_;
            size = size_;
            comm = comm_;
        }


        void print_test() {
            cout << "Hello from server " << rank << endl;
        }

        void receiveEncryptedGraph() {

            int count;
            MPI_Recv(&count, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            cout << "Receive count: " << count << endl;
            vector<int> lengths(count);
            MPI_Recv(lengths.data(), count, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            // cout << "Receive lengths"  << endl;
            int total_len = 0;
            for (int len : lengths) total_len += len;

            vector<char> buffer(total_len);
            MPI_Recv(buffer.data(), total_len, MPI_CHAR, 0, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            cout << "Receive buffer size: " << buffer.size() << endl;

            vector<string> serializedGraph;
            int pos = 0;
            for (int i = 0; i < count; ++i) {
                serializedGraph.emplace_back(&buffer[pos], lengths[i]);
                pos += lengths[i];
            }
            
            vector<Ciphertext<DCRTPoly>> flatGraph = MPIUtils::deserializeVectorElements<Ciphertext<DCRTPoly>>(serializedGraph);
            
            // Reconstruct relation
            for(int i = 0; i < flatGraph.size(); i+=2) {
                G.insert(flatGraph[i], flatGraph[i+1]);
            }

            // MPI_Status status;

            // // 1. Receive the size first
            // int size = 0;
            // MPI_Recv(&size, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

            // // 2. Allocate buffer for serialized data
            // std::vector<char> buffer(size);

            // // 3. Receive the serialized data
            // MPI_Recv(buffer.data(), size, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &status);
            // cout << buffer.data() << endl;

            // // 4. Convert to std::string for deserialization
            // std::string serialized(buffer.begin(), buffer.end());
            // cout << "Server: serialize size: " << serialized.size() << endl;

            // // 5. Deserialize to get Ciphertext<DCRTPoly>
            // Ciphertext<DCRTPoly> graph = MPIUtils::deserializeData<Ciphertext<DCRTPoly>>(serialized); 


            // vector<DCRTPoly> elements = graph->GetElements();
            // int count = elements.size();
            // cout << "Server: cipher size = " << count << endl;


        }


        void transitive_closure() {

        }

        // void sendEncryptedGraph() {
        //     string serialized = MPIUtils::serializeData<Ciphertext<DCRTPoly>>(test_data);
        //     int size = serialized.size();
        //     cout << "Size server after: " << size << endl;
        //     // Send the size first
        //     MPI_Send(&size, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
        //     // Then send the serialized data
        //     MPI_Send(serialized.data(), size, MPI_CHAR, 0, 1, MPI_COMM_WORLD);
        // }


        // template <typename T>
        // T deserializeData(const std::string& serialized) {
        //     T data;
        //     std::stringstream ss(serialized);
        //     {
        //         cereal::BinaryInputArchive archive(ss);
        //         archive(data);
        //     }
        //     return data;
        // }


};

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, world_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Split the world communicator
    int color = (rank == 0) ? 0 : 1;  // 0 = client, 1 = server
    MPI_Comm local_comm;
    MPI_Comm_split(MPI_COMM_WORLD, color, rank, &local_comm);

    // Rank inside new communicator
    int local_rank, local_size;
    MPI_Comm_rank(local_comm, &local_rank);
    MPI_Comm_size(local_comm, &local_size);


    if (color == 0) {
        Client client = Client(local_rank, local_size, local_comm);
        // client.print_test();
        client.init_graph();
        // client.print_graph();
        client.encrypt_graph();
        // client.decrypt_graph();
        client.print_decrypted_graph();
        client.sendEncryptedGraph();
        // client.receiveEncryptedGraph();
    } else {
        Server server = Server(local_rank, local_size, local_comm);
        if(rank == 1) {
            server.receiveEncryptedGraph();
            // server.sendEncryptedGraph();
        }
        // server.print_test();
    }


    // Free new communicator
    MPI_Comm_free(&local_comm);
    MPI_Finalize();
    return 0;
}
