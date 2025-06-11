#include <iostream>
#include <unordered_map>
#include <bits/stdc++.h>

using namespace std;


template<typename T>

class Rel {
public:
    unordered_map<int, set<T>> data;
    void insert(int key, T val) {
        data[key].insert(val);
    }

    void print_data() {
        int key;
        set<T> val;
        for (auto x = data.begin(); x != data.end(); x++) {
            key = x->first;
            val = x->second;
            cout << key << ": ";
            for(auto y = val.begin(); y != val.end(); y++) {
                cout << *y << " ";
            }
        cout << endl;
        }
    }

    Rel<T> select_all() {
        Rel<T> result;
        
        int key;
        set<T> val;
        for (auto x = data.begin(); x != data.end(); x++) {
            key = x->first;
            val = x->second;
            for(auto y = val.begin(); y != val.end(); y++) {
                result.insert(key, *y);
            }
        }
        return result;
    }

    Rel<T> join(Rel<T> target) {
        Rel<T> result;
        
        // unordered_map<int, set<T>> target_data = targe

        int key;
        set<T> val;

        int target_key;
        set<T> target_val;
        for (auto x = data.begin(); x != data.end(); x++) {
            key = x->first;
            val = x->second;
            for(auto y = val.begin(); y != val.end(); y++) {
                for (auto target_x = target.data.begin(); target_x != target.data.end(); target_x++) {
                    target_key = target_x->first;
                    target_val = target_x->second;
                    if(*y == target_key) {
                        for(auto target_y = target_val.begin(); target_y != target_val.end(); target_y++) {
                            result.insert(key, *target_y);
                        }
                    }
                }
            }
        }
        return result;

    }

    Rel<T> union_op(Rel<T> op) {
        Rel<T> result = this->select_all();  // Start with a copy of current datax`
        for (auto &pair : op.data) {
            for (const auto &val : pair.second) {
                result.insert(pair.first, val);
            }
        }
        return result;
    }

    bool has(int key, T val) {
        return data.count(key) && data[key].count(val);
    }

    bool is_empty() {
        for (auto& [k, vset] : data)
            if (!vset.empty())
                return false;
        return true;
    }

    Rel<T> transitive_closure() {
        Rel<T> delta_T = this->select_all();  // new edges found in the current round
        Rel<T> total_T = this->select_all();  // accumulated transitive closure

        while (!delta_T.data.empty()) {
            Rel<T> new_T;

            for (auto &pair : delta_T.data) {
                int x = pair.first;
                for (const auto &y : pair.second) {
                    if (data.count(y)) {
                        for (const auto &z : data[y]) {
                            if (!total_T.has(x, z) && !delta_T.has(x, z)) {
                                new_T.insert(x, z);
                            }
                        }
                    }
                }
            }
            total_T = total_T.union_op(new_T);
            delta_T = new_T;
        }
        return total_T;
    }
};

int main(int argc, char**argv) {
    
    Rel<int> G;
    G.insert(0, 1);
    G.insert(0, 2);
    G.insert(1, 3);
    G.insert(2, 3);
    G.insert(3, 4);

    G.print_data();

    Rel<int> G1 = G.transitive_closure();


    cout << "G1" << endl;
    G1.print_data();
    // Rel<int> G1;
    // G1 = G.select_all();
    // G1.print_data();

    // cout << "G2" << endl;
    // Rel<int> G2;
    // G2 = G.join(G1);
    // G2.print_data();
    

    // Rel<int> G3;

    return 0;
}