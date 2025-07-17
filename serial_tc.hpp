#ifndef SERIAL_TC_HPP
#define SERIAL_TC_HPP

#include <unordered_map>
#include <bits/stdc++.h>

template<typename T>
class Rel {
public:
    std::unordered_map<T, std::set<T>> data;
        
    void insert(T key, T val);

    void print_data();

    Rel<T> select_all();

    Rel<T> join(Rel<T> target);

    Rel<T> union_op(Rel<T> op);

    bool has(T key, T val);

    bool is_empty();

    Rel<T> transitive_closure();
};


#include "serial_tc.tpp"

#endif