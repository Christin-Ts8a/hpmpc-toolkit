#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "jsoncpp/json/json.h"
#include <emp-tool/emp-tool.h>
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
        cout << "The client has been initiated" << endl;
    }
    ~FluidRSSClient() {
        delete[] this->keys;
    }

    // 获取随机数作为为随机数生成器的种子
    void get_random_seed(block& seed){
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
        Value value;
        #ifdef SOURCE_DIR
            string path(SOURCE_DIR);
            path += "/resources/data.json";
            ofstream file(path, ios::out | ios:: binary);
            for(int i = 0; i < num; i++) {
                value[i] = rand();
            }
            file << value;
        #else
            cerr << "there is no file to write the random number, please provide the base(project) dir by cmake macro \"SOURCE_DIR\"" << endl;
            return;
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
        for(int i = 0; i < data_num; i++) {
            block temp_sum;
            memset(&temp_sum, 0, sizeof(block));
            for(int j = 0; j < this->key_size; j++) {
                block temp;
                PRG prg(&(this->keys[j]));
                prg.random_block(&temp);
                temp_sum += temp;
            }
            shares[i] = data[i] - temp_sum;
        }
    }

    // 生成数据对应的份额
    void get_shares_from_dataset(block* shares, const long* data, const int data_num){
        for(int i = 0; i < data_num; i++) {
            block temp_sum;
            memset(&temp_sum, 0, sizeof(block));
            for(int j = 1; j < this->key_size; j++) {
                block temp;
                PRG prg(&(this->keys[j]));
                prg.random_block(&temp);
                temp_sum += temp;
            }
            shares[i] = data[i] - temp_sum;
        }
    }

    void get_shares_from_dataset_traditional(block** shares, const int* data, const int data_num){
        for(int i = 0; i < data_num; i++) {
            block temp_sum;
            memset(&temp_sum, 0, sizeof(block));
            for(int j = 0; j < this->key_size; j++) {
                block temp;
                PRG prg(&(this->keys[j]));
                prg.random_block(&temp);
                temp_sum += temp;
                *shares[j] = temp;
            }
            *shares[0] = data[i] - temp_sum;
        }
    }

    // 获取与服务器的连接
    void get_connection_to_servers(vector<string> ips, vector<int> ports, int server_num){

        struct sockaddr_in ser;
        // base on IPV4
        ser.sin_family = AF_INET;
        for(int i = 0; i < server_num; i++) {
            int socketfd = socket(AF_INET, SOCK_STREAM, 0);
            if(socketfd == -1) {
                cerr << "Prepare socket error" << endl;
                return;
            }
            // character order transfer
            ser.sin_port = htons(ports[i]);
            ser.sin_addr.s_addr = inet_addr(ips[i].c_str());
            cout << "start connecting server: " << ips[i] << ":" << ports[i] << endl;
            while(1) {
                int conn = connect(socketfd, (struct sockaddr*)&ser, sizeof(ser));
                if(conn == 0) {
                    this->streams.push_back(new SenderSubChannel(socketfd));
                    cout << "create a connection to the server " << inet_ntoa(ser.sin_addr) << ":" << ports[i] << endl;
                    break;
                }
                close(socketfd);
            }
            cout << "The " << i + 1 << "th server has been connected" << endl;
        }
    }

    // 向服务器发送数据
    void send_data_to_server(const void* shares, const int data_num){
        Value mappings;
        #ifdef SOURCE_DIR
            string path(SOURCE_DIR);
            path += "/resources/mappings.json";
            ifstream file(path);
            file >> mappings;
        #else
            cerr << "There is no base path" << endl;
            return;
        #endif
        string index = to_string(this->server_size) + "-party";
        Value mapping = mappings[index];

        cout << "start sending shares to clients" << endl;
        // 发送特殊份额
        for(int i = 0; i < this->streams.size(); i++) {
            if(find(mapping[0].begin(), mapping[0].end(), i) == mapping[0].end()) {
                this->streams[i]->send_data(shares, data_num * sizeof(block));
                this->streams[i]->flush();
                cout << "the sending of the shares to the " << i + 1 << "th server has complished, the number of the sending shares: " << data_num << endl;
            }
        }
        cout << "shares sending finished" << endl;

        // 发送密钥
        cout << "start sending keys to clients" << endl;
        for(int i = 0; i < this->streams.size(); i++) {
            int key_snd_num = C(this->threshold, this->server_size - 1);
            block key_snd[key_snd_num];
            int key_index = 0;
            for(int j = 0; j < this->key_size; j++) {
                if(find(mapping[j].begin(), mapping[j].end(), i) == mapping[j].end()) {
                    key_snd[key_index++] = this->keys[j];
                }
            }
            this->streams[i]->send_data(key_snd, key_snd_num * sizeof(block));
            this->streams[i]->flush();
            cout << "the sending of the keys to the "  << i + 1 << "th server has complished, the number of the sending keys: " << key_snd_num << endl;
        }
        cout << "keys sending finished" << endl;
    }

    void send_data_to_server_traditional(block** shares, const int data_num) {
        Value mappings;
        #ifdef SOURCE_DIR
            string path(SOURCE_DIR);
            path += "/resources/mappings.json";
            ifstream file(path);
            file >> mappings;
        #else
            cerr << "There is no base path" << endl;
            return;
        #endif
        string index = to_string(this->server_size) + "-party";
        Value mapping = mappings[index];

        cout << "start sending shares to clients" << endl;
        for(int i = 0; i < this->streams.size(); i++) {
            for(int j = 0; j < mapping.size(); j++) {
                if(find(mapping[j].begin(), mapping[j].end(), i) == mapping[j].end()) {
                    this->streams[i]->send_data(shares[j], data_num * sizeof(block));
                }
            }
            this->streams[i]->flush();
            cout << "the sending of the shares to the " << i + 1 << "th server has complished, the number of the sending shares: " << data_num << endl;
        }
        cout << "shares sending finished" << endl;
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