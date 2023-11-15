#include <iostream>
#include <fstream>
#include <jsoncpp/json/json.h>
#include <emp-tool/utils/prg.h>
#include "../utils/math_utils.h"

using namespace std;
using namespace emp;
using namespace Json;

class FluidRSSClient {
    public:
    FluidRSSClient();
    FluidRSSClient(int client_size, int client_id, int server_size)
        : client_size(client_size), client_id(client_id), server_size(server_size) {
        // 计算默认阈值，保证诚实大多数
        this->threshold = server_size % 2 == 0 ? (server_size / 2 - 1) : (server_size / 2);
        this->key_size = C(this->threshold, server_size);
        // 给随机数种子申请对应空间
        this->keys = new block[this->key_size];
        // 生成随机数种子，数量与 server_size 对应
        get_random_seeds(this->keys, this->key_size);
    }
    ~FluidRSSClient() {
        delete this->keys;
    }

    // 获取随机数作为为随机数生成器的种子
    void get_random_seed(block* seed){
        uint32_t *seed_ptr = (uint32_t *)&seed;
        random_device rand_div("/dev/urandom");
        for(size_t i = 0; i < sizeof(block) / sizeof(uint32_t); ++i)
            seed_ptr[i] = rand_div();
    }

    // 获取多个随机数作为为随机数生成器的种子
    void get_random_seeds(block* seeds, int num = 1){
        int seed_num = C(this->threshold, this->server_size);
        uint32_t *seed_ptr = (uint32_t *)&seeds;
        random_device rand_div("/dev/urandom");
        for(size_t i = 0; i < (sizeof(block) * seed_num) / sizeof(uint32_t); ++i)
            seed_ptr[i] = rand_div();
    }

    // 生成int型随机数
    void generate_random_int(int num) {
        ofstream file("./data.txt");
        Value value;
        for(int i = 0; i < num; i++) {
            value[i] = rand();
        }
        file << value;
    }

    // 获取int型数据
    void get_dataset(string filepath, int* data, int data_num){
        ifstream file(filepath);
        if(!file.is_open()) {
            cout << "File can not be opend" << endl;
            return;
        }

        Value dataset;
        file >> dataset;
        if(num > dataset.size()) {
            cout << "There is not enough data" << endl;
        }

        data = new int[data_num];
        for(int i = 0; i < data_num; i++) {
            *(data+i) = dataset[i].asInt();
        }
    }

    // // 获取float型数据，返回获取的数据个数
    // long get_dataset(string filepath, float* data);

    // // 获取double型数据，返回获取的数据个数
    // long get_dataset(string filepath, double* data);

    // 生成数据对应的份额
    void get_shares_from_dataset(block* shares, int* data, int data_num){
        shares = new block[data_num];
        for(int i = 0; i < data_num; i++) {
            block temp_sum;
            for(int j = 0; j < this->key_size; j++) {
                block temp;
                PRG prg(this->keys[j]);
                prg.random_block(temp);
                temp_sum -= temp;
            }
            shares[i] = data[i] - temp_sum;
        }
    }

    // 生成数据对应的份额
    void get_shares_from_dataset(block* shares, long* data);

    // 获取与服务器的连接
    void get_connection_to_servers();

    // 向服务器发送数据
    void send_data_to_server();

    private:
    int client_size;
    int client_id;
    int server_size;
    int threshold;
    int key_size;
    block* keys;
};