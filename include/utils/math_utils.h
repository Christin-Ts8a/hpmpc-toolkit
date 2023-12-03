#ifndef MATH_UTILS
#define MATH_UTILS
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

void shares_permutation(block *&init_shares, int n) {
    vector<blocknode> sort_shares;
    vector<int> sort_index;
    for(int i = 0; i < n; i++) {
        blocknode node(init_shares[i]);
        sort_shares.push_back(node);
    }
    sort(sort_shares.begin(), sort_shares.end());
    for(int i = 0; i < n; i++) {
        auto it = find(sort_shares.begin(), sort_shares.end(), blocknode(init_shares[i]));
        sort_index.push_back(distance(sort_shares.begin(), it));
    }
    for(int i = 0; i < n - 1; i++) {
        for(int j = 0; j < n - 1 - i; j++){
            if(sort_index[i] > j) {
                int temp_index = sort_index[i];
                sort_index[i] = sort_index[j];
                sort_index[j] = temp_index;

                block temp_share = init_shares[i];
                init_shares[i] = init_shares[j];
                init_shares[j] = temp_share;
            }
        }
    }
}

#endif