#include <iostream>
#include <bits/stdc++.h>

using namespace std;


template<typename T>
void Rel<T>::insert(T key, T val) {
    data[key].insert(val);
}


template<typename T>
void Rel<T>::print_data() {
    T key;
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


template<typename T>
Rel<T> Rel<T>::select_all() {
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


template<typename T>
Rel<T> Rel<T>::join(Rel<T> target) {
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


template<typename T>
Rel<T> Rel<T>::union_op(Rel<T> op) {
    Rel<T> result = this->select_all();  // Start with a copy of current datax`
    for (auto &pair : op.data) {
        for (const auto &val : pair.second) {
            result.insert(pair.first, val);
        }
    }
    return result;
}


template<typename T>
bool Rel<T>::has(T key, T val) {
    return data.count(key) && data[key].count(val);
}


template<typename T>
bool Rel<T>::is_empty() {
    for (auto& [k, vset] : data)
        if (!vset.empty())
            return false;
    return true;
}


template<typename T>
Rel<T> Rel<T>::transitive_closure() {
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
