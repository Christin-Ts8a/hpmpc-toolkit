#ifndef __MATH_UTILS_H
#define __MATH_UTILS_H
#include <set>
#include <typeinfo>
#include <emp-tool/emp-tool.h>
#include "container/blocknode.h"

using namespace emp;
using namespace std;
long factorial(int n) {
    long N = 1;
    for(int i = 1; i <= n; i++) {
        N *= i;
    }
    return N;
}

long C(int t, int n) {
    return factorial(n) / (factorial(t) * factorial(n - t));
}

int power(int a, int b) {
    int res = 1;
    for(int i = 0; i < b; i++) {
        res *= a;
    }
    return res;
}

void shares_permutation(block** &a, block** &b, block* &c, block *&init_index, int share_size, int triples_num) {
    vector<blocknode> unsort_index;
    vector<int> sort_index;
    cout << "shares_permutation: prepare unsorted index vector" << endl;
    for(int i = 0; i < triples_num; i++) {
        blocknode node(init_index[i]);
        unsort_index.push_back(node);
    }
    cout << "shares_permutation: sort the unsorted index vector" << endl;
    sort(unsort_index.begin(), unsort_index.end());
    cout << "shares_permutation: resort the vector accroding to the distance" << endl;
    for(int i = 0; i < triples_num; i++) {
        auto it = find(unsort_index.begin(), unsort_index.end(), blocknode(init_index[i]));
        sort_index.push_back(distance(unsort_index.begin(), it));
    }
    cout << "shares_permutation: resort the triples" << endl;
    for(int i = 0; i < triples_num - 1; i++) {
        for(int j = 0; j < triples_num - 1 - i; j++){
            if(sort_index[j] > sort_index[j + 1]) {
                block tempc = c[j];
                c[j] = c[j + 1];
                c[j + 1] = tempc;

                for(int k = 0; k < share_size; k++) {
                    block temp = a[k][j];
                    a[k][j] = a[k][j + 1];
                    a[k][j + 1] = temp;

                    temp = b[k][j];
                    b[k][j] = b[k][j + 1];
                    b[k][j + 1] = temp;
                }
            }
        }
    }
    cout << "shares_permtation: permutation has done" << endl;
}

bool block_equal(block &a, block &b) {
    uint64_t a_val[2], b_val[2];
    memcpy(a_val, &a, sizeof(a_val));
    memcpy(b_val, &b, sizeof(b_val));
    if(a_val[0] != b_val[0]) {
        return false;
    } else if(a_val[1] != b_val[1]) {
        return false;
    } else {
        return true;
    }
}

#endif