#ifndef BLOCKSET
#define BLOCKSET

#include <emp-tool/emp-tool.h>
using namespace emp;

class blocknode {
public:
    blocknode() = default;
    blocknode(block node): node(node){};
    bool operator <(const blocknode &that)const {
        uint64_t this_val[2], that_val[2];
        memcpy(this_val, &this->node, sizeof(this_val));
        memcpy(that_val, &that.node, sizeof(that_val));
        if(this_val[0] < that_val[0]){
            return true;
        } else if(this_val[0] == that_val[0] && this_val[1] < that_val[1]) {
            return true;
        } else {
            return false;
        }
    }
    bool operator >(const blocknode &that)const {
        uint64_t this_val[2], that_val[2];
        memcpy(this_val, &this->node, sizeof(this_val));
        memcpy(that_val, &that.node, sizeof(that_val));
        if(this_val[0] > that_val[0]){
            return true;
        } else if(this_val[0] == that_val[0] && this_val[1] > that_val[1]) {
            return true;
        } else {
            return false;
        }
    }
    bool operator ==(const blocknode &that)const {
        uint64_t this_val[2], that_val[2];
        memcpy(this_val, &this->node, sizeof(this_val));
        memcpy(that_val, &that.node, sizeof(that_val));
        if(this_val[0] == that_val[0] && this_val[1] == that_val[1]) {
            return true;
        } else {
            return false;
        }
    }

    block node;
};
#endif