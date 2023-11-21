#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "jsoncpp/json/json.h"
#include <emp-tool/emp-tool.h>
// #include <emp-tool/io/highspeed_net_io_channel.h>
#include "../utils/math_utils.h"

using namespace std;
using namespace emp;
using namespace Json;

class FluidRSSClient {
    public:
    FluidRSSClient();
    FluidRSSClient(int client_id, int server_size)
        : client_id(client_id), server_size(server_size) {
        // 计算默认阈值，保证诚实大多数
        this->threshold = server_size % 2 == 0 ? (server_size / 2 - 1) : (server_size / 2);
        this->key_size = C(this->threshold, server_size);
        // 给随机数种子申请对应空间
        this->keys = new block[this->key_size];
        // 生成随机数种子，数量与 server_size 对应
        get_random_seeds(this->keys);
    }
    ~FluidRSSClient() {
        delete[] this->keys;
    }

    // 获取随机数作为为随机数生成器的种子
    void get_random_seed(block* seed){
        uint32_t *seed_ptr = (uint32_t *)&seed;
        random_device rand_div("/dev/urandom");
        for(size_t i = 0; i < sizeof(block) / sizeof(uint32_t); ++i)
            seed_ptr[i] = rand_div();
    }

    // 获取多个随机数作为为随机数生成器的种子
    void get_random_seeds(block* seeds){
        uint32_t *seed_ptr = (uint32_t *)seeds;
        random_device rand_div("/dev/urandom");
        for(size_t i = 0; i < (sizeof(block) * this->key_size) / sizeof(uint32_t); ++i){
            seed_ptr[i] = rand_div();
        }
    }

    // 生成int型随机数
    void generate_random_int(int num) {
        #ifdef SOURCE_DIR
            string path(SOURCE_DIR);
            path += "/resources/data.json";
            ofstream file(path);
            Value value;
            for(int i = 0; i < num; i++) {
                value[i] = rand();
            }
            file << value;
        #else
            cerr << "there is no file to write the random number, please provide the base(project) dir by cmake macro \"SOURCE_DIR\"" << endl;
        #endif
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
        if(data_num > dataset.size()) {
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
    void get_shares_from_dataset(block* shares, const int* data, const int data_num){
        shares = new block[data_num];
        for(int i = 0; i < data_num; i++) {
            block temp_sum;
            for(int j = 0; j < this->key_size; j++) {
                block temp;
                PRG prg(&(this->keys[j]));
                prg.random_block(&temp);
                temp_sum -= temp;
            }
            shares[i] = data[i] - temp_sum;
        }
    }

    // 生成数据对应的份额
    void get_shares_from_dataset(block* shares, const long* data, const int data_num){
        shares = new block[data_num];
        for(int i = 0; i < data_num; i++) {
            block temp_sum;
            for(int j = 0; j < this->key_size; j++) {
                block temp;
                PRG prg(&(this->keys[j]));
                prg.random_block(&temp);
                temp_sum -= temp;
            }
            shares[i] = data[i] - temp_sum;
        }
    }

    // 获取与服务器的连接
    void get_connection_to_servers(vector<string> ips, vector<int> ports){
        int socketfd = socket(AF_INET, SOCK_STREAM, 0);
        if(socketfd == -1) {
            perror("socket");
            return;
        }

        sockaddr_in ser;
        // base on IPV4
        ser.sin_family = AF_INET;
        for(int i = 0; i < ips.size(); i++) {
            // character order transfer
            ser.sin_port = htons(ports[0]);
            ser.sin_addr.s_addr = inet_addr(ips[0].c_str());

            int conn = connect(socketfd, (struct sockaddr*)&ser, sizeof(ser));
            if(conn == -1) {
                perror("accept");
                return;
            }
            SenderSubChannel* stream = new SenderSubChannel(conn);
            this->streams.push_back(stream);
        }
    }

    // 向服务器发送数据
    void send_data_to_server(const void* shares, const int data_num){
        string path(SOURCE_DIR);
        path += "/resources/mappings.json";
        ifstream file(path);
        Value mappings;
        file >> mappings;
        string index = to_string(this->server_size) + "-party";
        Value mapping = mappings[index];
        for(int i = 0; i < mapping.size(); i++) {
            for(int j = 0; j < mapping[i].size(); j++) {
                int party = mapping[i][j].asInt();
                this->streams[party]->send_data(&(this->keys[i]), 1);
                if(i == 0) {
                    this->streams[party]->send_data(shares, data_num);
                }
            }
        }
    }

    private:
    // 客户端序号
    int client_id;
    // 服务器committee大小
    int server_size;
    // RSS阈值
    int threshold;
    // （t，n）-RSS下的密钥数量
    int key_size;
    // 密钥，用于生成随机数份额
    block* keys;
    // 通信连接
    vector<SenderSubChannel*> streams;
};