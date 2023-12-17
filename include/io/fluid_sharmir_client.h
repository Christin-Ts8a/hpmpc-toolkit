#include <iostream>
#include <emp-tool/emp-tool.h>
#include <jsoncpp/json/json.h>
#include "utils/math_utils.h"

using namespace std;
using namespace emp;
using namespace Json;

class FluidSharmirClient {
public:
    FluidSharmirClient();
    FluidSharmirClient(int client_id, int server_size)
        : client_id(client_id), server_size(server_size) {
        // 计算默认阈值，保证诚实大多数
        this->threshold = server_size % 2 == 0 ? (server_size / 2 - 1) : (server_size / 2);
        // 多项式系数申请对应空间并生成系数
        this->polynomial_coefficient = new block[this->threshold];
        PRG prg;
        prg.random_block(this->polynomial_coefficient, this->threshold);
        // 给随机数种子申请对应空间
        get_random_seed(this->keys);
        cout << "The client has been initiated" << endl;
    }
    ~FluidSharmirClient() {
        delete[] this->polynomial_coefficient;
    }

    // 获取随机数作为为随机数生成器的种子
    void get_random_seed(block& seed){
        uint32_t *seed_ptr = (uint32_t *)&seed;
        random_device rand_div("/dev/urandom");
        for(size_t i = 0; i < sizeof(block) / sizeof(uint32_t); ++i)
            seed_ptr[i] = rand_div();
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
    void get_dataset(string filepath, int* &data, int data_num){
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

    // 生成数据对应的份额
    void get_shares_from_dataset(block** &shares, const int* data, const int data_num){
        shares = new block*[this->server_size];
        for(int i = 0; i < this->server_size; ++i){
            shares[i] = new block[data_num];
            for(int j = 0; j < data_num; ++j) {
                get_sharmir_shares(shares[i][j], data[j], i+1);
            }
        }
    }

    void get_sharmir_shares(block& share, const int data, int base) {
        block temp;
        memset(&temp, 0, sizeof(block));
        for(int i = 0; i < this->threshold; i++) {
            int var = pow(base, i+1);
            temp += this->polynomial_coefficient[i] * var;
        }
        share = data + temp;
    }

    void get_sharmir_shares(block& share, block data, int base) {
        block temp;
        memset(&temp, 0, sizeof(block));
        for(int i = 0; i < this->threshold; i++) {
            int var = pow(base, i+1);
            temp += this->polynomial_coefficient[i] * var;
        }
        share = data + temp;
    }

    void get_correlated_randomness(block **&randoms, const int data_num) {
        block* secret = new block[data_num + 2];
        PRG().random_block(secret, data_num + 2);
        randoms = new block*[this->server_size];
        for(int i = 0; i < this->server_size; ++i){
            randoms[i] = new block[data_num + 2];
            for(int j = 0; j < data_num + 2; ++j) {
                get_sharmir_shares(randoms[i][j], secret[j], i+1);
            }
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
    void send_data_to_server(block** &shares, const int data_num) {
        cout << "start sending shares to clients" << endl;
        for(int i = 0; i < this->streams.size(); i++) {
            this->streams[i]->send_data(shares[i], data_num * sizeof(block));
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
    // 密钥，用于生成随机数份额
    block keys;
    // 多项式系数
    block* polynomial_coefficient;
    // 通信连接
    vector<SenderSubChannel*> streams;
};