#include <iostream>
#include <unordered_map>
#include <set>
#include <vector>
#include <mpi.h>

using namespace std;

template<typename T>
class Rel {
public:
    unordered_map<int, set<T>> data;

    void insert(int key, T val) {
        data[key].insert(val);
    }

    void print_data() {
        for (auto& [key, valset] : data) {
            cout << key << ": ";
            for (auto& val : valset)
                cout << val << " ";
            cout << endl;
        }
    }

    Rel<T> select_all() {
        Rel<T> result;
        for (auto& [key, valset] : data)
            for (auto& val : valset)
                result.insert(key, val);
        return result;
    }

    Rel<T> union_op(Rel<T> op) {
        Rel<T> result = this->select_all();
        for (auto& [key, valset] : op.data)
            for (auto& val : valset)
                result.insert(key, val);
        return result;
    }

    bool has(int key, T val) {
        return data.count(key) && data[key].count(val);
    }

    Rel<T> transitive_closure() {
        Rel<T> delta_T = this->select_all();
        Rel<T> total_T = this->select_all();

        while (!delta_T.data.empty()) {
            Rel<T> new_T;

            for (auto& [x, ys] : delta_T.data) {
                for (auto& y : ys) {
                    if (data.count(y)) {
                        for (auto& z : data[y]) {
                            if (!total_T.has(x, z))
                                new_T.insert(x, z);
                        }
                    }
                }
            }
            total_T = total_T.union_op(new_T);
            delta_T = new_T;
        }
        return total_T;
    }

    void distribute_graph(int world_rank, int world_size) {
        if (world_rank == 0) {
            // Split data by process
            vector<vector<int>> send_buf(world_size);  // Flattened [key, val, key, val, ...]
            for (auto& [key, valset] : data) {
                int target = hash<int>{}(key) % world_size;
                for (auto& val : valset) {
                    send_buf[target].push_back(key);
                    send_buf[target].push_back(val);
                }
            }

            // Send to other processes
            for (int i = 1; i < world_size; ++i) {
                int size = send_buf[i].size();
                MPI_Send(&size, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                if (size > 0)
                    MPI_Send(send_buf[i].data(), size, MPI_INT, i, 1, MPI_COMM_WORLD);
            }

            // Keep own part
            data.clear();
            for (size_t i = 0; i < send_buf[0].size(); i += 2)
                insert(send_buf[0][i], send_buf[0][i + 1]);

        } else {
            // Receive size
            int size;
            MPI_Recv(&size, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (size > 0) {
                vector<int> recv_buf(size);
                MPI_Recv(recv_buf.data(), size, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                for (int i = 0; i < size; i += 2)
                    insert(recv_buf[i], recv_buf[i + 1]);
            }
        }
    }

    void exchange_data(Rel<T>& delta_T, int world_rank, int world_size, int iteration) {
        vector<vector<int>> send_buf(world_size);
        for (auto& [key, valset] : delta_T.data) {
            for (auto& val : valset) {
                int target = hash<int>{}(val) % world_size;
                send_buf[target].push_back(key);
                send_buf[target].push_back(val);
            }
        }

        vector<int> sendcounts(world_size);
        for(int i=0; i< world_size; i++) {
            sendcounts[i] = send_buf[i].size();
        }
        
        // Initialize send displacements
        int total_send = 0;
        vector<int> sdispls(world_size);
        for(int i=0; i< world_size; i++) {
            sdispls[i] = total_send;
            total_send += sendcounts[i];
        }
        
        // Flatten the send_data to into send_flat
        vector<int> send_flat(total_send);
        for (int i = 0; i < world_size; ++i) {
            copy(send_buf[i].begin(), send_buf[i].end(),
                send_flat.begin() + sdispls[i]);
        }

        // Exchange size of data will be sent
        vector<int> recvcounts(world_size);
        MPI_Alltoall(sendcounts.data(), 1, MPI_INT, recvcounts.data(), 1, MPI_INT, MPI_COMM_WORLD);

        // Initial receive displacements
        vector<int> rdispls(world_size);
        int total_recv = 0;
        for (int i = 0; i < world_size; ++i) {
            rdispls[i] = total_recv;
            total_recv += recvcounts[i];
        }



        // Exchange actual data
        vector<int> recv_flat(total_recv);
        MPI_Alltoallv(send_flat.data(), sendcounts.data(), sdispls.data(), MPI_INT,
                    recv_flat.data(), recvcounts.data(), rdispls.data(), MPI_INT,
                    MPI_COMM_WORLD);
        
        // Replace delta_T data with the received data
        delta_T.data.clear();
        for (int i = 0; i < total_recv; i += 2) {
            int key = recv_flat[i];
            int val = recv_flat[i + 1];
            delta_T.insert(key, val);
        }

    }

    Rel<T> parallel_transitive_closure(int world_rank, int world_size) {
        Rel<T> total_T = this->select_all();
        Rel<T> delta_T = this->select_all();
        exchange_data(delta_T, world_rank, world_size, -1);
        

        int i=0;
        while (true) {
            Rel<T> new_T;

            for (auto& [x, ys] : delta_T.data) {
                for (auto& y : ys) {
                    if (data.count(y)) {
                        for (auto& z : data[y]) {
                            if (!total_T.has(x, z))
                                new_T.insert(x, z);
                        }
                    }
                }
            }
            total_T = total_T.union_op(new_T);
            // cout << "Rank " << world_rank << " iteration " << i << " new_T:\n";
            // total_T.print_data();
            delta_T = new_T;
            
            exchange_data(delta_T, world_rank, world_size, i);

            // Synchronization
            int local_empty = delta_T.data.empty() ? 1 : 0;
            int global_empty;
            MPI_Allreduce(&local_empty, &global_empty, 1, MPI_INT, MPI_LAND, MPI_COMM_WORLD);

            if (global_empty)
                break;
           
            i += 1;
        }

        return total_T;
    }

    void gather_data(int dest, int rank, int size) {

        vector<int> send_buf;
        int sendcount = 0;
        for (auto& [key, valset] : data) {
            for (auto& val : valset) {
                send_buf.push_back(key);
                send_buf.push_back(val);
                sendcount += 2;
            }
        }
        

        vector<int> recvcounts(size);
        MPI_Gather(&sendcount, 1, MPI_INT, recvcounts.data(), 1, MPI_INT, dest, MPI_COMM_WORLD);


        vector<int> rdispls(size);
        int total_recv = 0;
        if(rank == 0) {
            for (int i = 0; i < size; ++i) {
                rdispls[i] = total_recv;
                total_recv += recvcounts[i];
            }
        }

        vector<int> recv_buf(total_recv);
        MPI_Gatherv(send_buf.data(), sendcount, MPI_INT, recv_buf.data(), recvcounts.data(), rdispls.data(), MPI_INT, dest, MPI_COMM_WORLD);

        
        if (rank == dest) {
            data.clear();
            for (size_t i = 0; i < total_recv; i += 2) {
                int key = recv_buf[i];
                int val = recv_buf[i + 1];
                insert(key, val);
            }
        } 
    }

};

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    Rel<int> G;

    if (world_rank == 0) {
        G.insert(0, 1);
        G.insert(0, 2);
        G.insert(1, 3);
        G.insert(2, 3);
        G.insert(3, 4);
    }

    // Distribute graph from rank 0 to all
    G.distribute_graph(world_rank, world_size);

    MPI_Barrier(MPI_COMM_WORLD);
    cout << "Rank " << world_rank << " has:\n";
    G.print_data();


    // G.parallel_transitive_closure(world_rank, world_size);
    Rel<int> G1 = G.parallel_transitive_closure(world_rank, world_size);
    
    G1.gather_data(0, world_rank, world_size);

    MPI_Barrier(MPI_COMM_WORLD);
    cout << "Rank " << world_rank << " transitive closure:\n";
    G1.print_data();

    MPI_Finalize();
    return 0;
}
