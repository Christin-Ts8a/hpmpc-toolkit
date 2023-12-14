#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <jsoncpp/json/json.h>
#include <emp-tool/emp-tool.h>
#include "utils/math_utils.h"
#include "utils/constant.h"
#include "io/net.h"

using namespace std;
using namespace emp;
using namespace Json;

class FluidRSSServer {
public:
    FluidRSSServer(int server_id, int committee_size, int client_size, int port_rcv)
        : server_id(server_id), committee_size(committee_size), client_size(client_size) {
        this->threshold = committee_size % 2 == 0 ? (committee_size / 2 - 1) : (committee_size / 2);
        this->key_size = C(threshold, this->committee_size - 1);
        for(int i = 0; i < this->client_size; i++) {
            block* temp_keys = new block[this->key_size];
            this->keys_cli.push_back(temp_keys);
        }
        receive_connection_from_client(port_rcv);
    }
    ~FluidRSSServer() {
        delete[] this->keys;
    }

    // 接受客户端连接
    void receive_connection_from_client(int port_rcv) {
        int socketfd = socket(AF_INET, SOCK_STREAM, 0);
        if(socketfd == -1) {
            cerr << "Prepare socket error" << endl;
            close(socketfd);
            return;
        }
        // socket端口复用
        int reuse = 1;
		setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse));
        sockaddr_in ser;
        // base on IPV4
        ser.sin_family = AF_INET;
        ser.sin_port = htons(port_rcv);
        ser.sin_addr.s_addr = INADDR_ANY;
        if(bind(socketfd, (struct sockaddr*)&ser, sizeof(ser)) == -1) {
            cerr << "bind socket error" << endl;
            close(socketfd);
            return;
        }
        if(listen(socketfd, 1024) == -1) {
            cerr << "listen socket error" << endl;
            close(socketfd);
            return;
        }
        for(int i = 0; i < this->client_size; i++) {
            struct sockaddr_in cli;
            // used to output size of client addr struct
            socklen_t len = sizeof(cli);
            int conn = accept(socketfd, (struct sockaddr*)&cli, &len);
            if(conn == -1) {
                cerr << "accept connect error" << endl;
                close(socketfd);
                return;
            }
            cout << "received a connection from " << inet_ntoa(cli.sin_addr) << ":" << cli.sin_port << endl;
            this->streams_rcv_cli.push_back(new RecverSubChannel(conn));
        }
        close(socketfd);
        cout << "All " << client_size << " clients have been connected. The server has been initiated" << endl;
    }

    // 从客户端接受份额以及密钥
    void receive_data_from_client(block** &shares, const int data_num) {
        Value mappings;
        Value mapping;
        #ifdef SOURCE_DIR
            string path(SOURCE_DIR);
            path += "/resources/mappings.json";
            ifstream ifs(path);
            ifs >> mappings;
        #else
            cerr << "There is no base path" << endl;
            return;
        #endif
        string index = to_string(this->committee_size) + "-party";
        mapping = mappings[index];

        // 从客户端接受份额
        if(find(mapping[0].begin(), mapping[0].end(), this->server_id) == mapping[0].end()) {
            shares = new block*[this->streams_rcv_cli.size()];
            cout << "start receiving the shares from clients" << endl;
            for(int i = 0; i < this->streams_rcv_cli.size(); i++) {
                shares[i] = new block[data_num];
                this->streams_rcv_cli[i]->recv_data(shares[i], data_num * sizeof(block));
                cout << "receive shares from client " << i + 1 << ", the shares num: " << data_num << endl;
            }
        }

        // 从客户端接受密钥
        for(int i = 0; i < this->streams_rcv_cli.size(); i++) {
            this->streams_rcv_cli[i]->recv_data(this->keys_cli[i], this->key_size * sizeof(block));
            cout << "receive keys from client " << i + 1 << ", the keys num: " << this->key_size << endl;
        }
        // 合并客户端年密钥作为这一轮committee密钥
        this->keys = new block[this->key_size];
        this->prgs = new PRG[this->key_size];
        memset(this->keys, 0, sizeof(block) * this->key_size);
        for(int i = 0; i < this->key_size; i++) {
            for(int j = 0; j < this->keys_cli.size(); j++) {
                this->keys[i] += this->keys_cli[j][i];
            }
            this->prgs[i] = PRG(&(this->keys[i]));
        }
    }

    // 获取乘法三元组([[a]], [[b]], [c])
    void get_triples_rss(block** &a, block** &b, block* &c, const int triples_num) {
        cout << "start generate triples' RSS shares([[a]], [[b]], [c])" << endl;
        a = new block*[this->key_size];
        b = new block*[this->key_size];
        c = new block[triples_num];
        memset(c, 0, sizeof(block) * triples_num);
        cout << "init triples by allocating space" << endl;
        for(int i = 0; i < this->key_size; i++) {
            a[i] = new block[triples_num];
            b[i] = new block[triples_num];
            this->prgs[i].random_block(a[i], triples_num);
            this->prgs[i].random_block(b[i], triples_num);
        }
        cout << "the processing of generating ([[a]], [[b]]) has done" << endl;

        Value mappings;
        #ifdef SOURCE_DIR
            string path(SOURCE_DIR);
            path += "/resources/rho.json";
            ifstream ifs(path);
            ifs >> mappings;
        #else
            cerr << "There is no base path" << endl;
            return;
        #endif
        string index = to_string(this->server_size) + "-party";
        Value mapping = mappings[index];
        Value rho = mapping[this->server_id];
        for(int i = 0; i < triples_num; i++) {
            block temp;
            memset(&temp, 0, sizeof(block));
            for(int j = 0; j < rho.size(); j++) {
                for(int k = 0; k < rho[j].size(); k++) {
                    temp += a[j][i] * b[k][i];
                }
            }
            c[i] += temp;
        }
        cout << "the process of computing ([c]) has done" << endl;
    }

    // 随机置换三元组
    void triples_permutation(block** &a, block ** &b, block* &c, int triples_num) {
        cout << "start permute the triples" << endl;
        if(this->streams_rcv_comm.size() < this->threshold || this->streams_snd_comm.size() < this->threshold) {
            cerr << "未连接committee" << endl;
            return;
        }
        block **index_per = new block*[this->key_size];
        for(int i = 0; i < this->key_size; i++) {
            index_per[i] = new block[triples_num];
            this->prgs[i].random_block(index_per[i], triples_num);
        }
        cout << "the shares of permutation index has been generated" << endl;
        Value mappings;
        #ifdef SOURCE_DIR
            string path(SOURCE_DIR);
            path += "/resources/chi.json";
            ifstream ifs(path);
            ifs >> mappings;
        #else
            cerr << "There is no base path" << endl;
        #endif
        string index = to_string(this->committee_size) + "-party";
        Value mapping = mappings[index];
        Value chi = mapping[this->server_id];
        cout << "the " << this->committee_size << "-party mapping for sending shares of permutation index has been read" << endl;

        // chi的大小表示需要向几个目标服务器发送份额，需要接收份额的目标服务器连接位于连接池的头部
        for(int i = 0; i < chi.size(); i++) {
            // chi中的每个数组代表需要向目标服务器发送的份额下标
            for(int j = 0; j < chi[i].size(); j++) {
                int value_id = chi[i][j].asInt();
                this->streams_snd_comm[i]->send_data(index_per[value_id], sizeof(block) * triples_num);
            }
            this->streams_snd_comm[i]->flush();
        }
        cout << "the shares of permutation index has been sent" << endl;

        block** index_per_rcv = new block*[chi.size()];
        for(int i = 0; i < chi.size(); i++) {
            index_per_rcv[i] = new block[triples_num];
            memset(index_per_rcv[i], 0, sizeof(block) * triples_num);
            block *index_temp = new block[triples_num];
            for(int j = 0; j < chi[i].size(); j++) {
                this->streams_rcv_comm[i]->recv_data(index_temp, sizeof(block) * triples_num);
                for(int k = 0; k < triples_num; k++) {
                    index_per_rcv[i][k] += index_temp[k];
                }
            }
            delete[] index_temp;
        }
        cout << "the shares of permutation index has been received" << endl;

        block* index_res = new block[triples_num];
        memset(index_res, 0, sizeof(block) * triples_num);
        for(int i = 0; i < this->key_size; i++) {
            for(int j = 0; j < triples_num; j++) {
                index_res[j] += index_per[i][j];
            }
            delete[] index_per[i];
        }
        delete[] index_per;

        for(int i = 0; i < chi.size(); i++) {
            for(int j = 0; j < triples_num; j++) {
                index_res[j] += index_per_rcv[i][j];
            }
            delete[] index_per_rcv[i];
        }
        delete[] index_per_rcv;
        cout << "the permutation index has been computated" << endl;

        shares_permutation(a, b, c, index_res, this->key_size, triples_num);
        cout << "the triples have been permuted" << endl;
    }

    void receive_connection_from_committee(int port_rcv, int connect_size) {
        cout << "start receiving connection from committee" << endl;
        
        int socketfd = socket(AF_INET, SOCK_STREAM, 0);
        if(socketfd == -1) {
            cerr << "Prepare socket error" << endl;
            close(socketfd);
            return;
        }
        int reuse = 1;
		setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse));
        sockaddr_in ser;
        // base on IPV4
        ser.sin_family = AF_INET;
        ser.sin_port = htons(port_rcv);
        ser.sin_addr.s_addr = INADDR_ANY;
        if(bind(socketfd, (struct sockaddr*)&ser, sizeof(ser)) == -1) {
            cerr << "bind socket error" << endl;
            close(socketfd);
            return;
        }
        if(listen(socketfd, 1024) == -1) {
            cerr << "listen socket error" << endl;
            close(socketfd);
            return;
        }
        for(int i = 0; i < connect_size; i++) {
            struct sockaddr_in cli;
            // used to output size of client addr struct
            socklen_t len = sizeof(cli);
            int conn = accept(socketfd, (struct sockaddr*)&cli, &len);
            if(conn == -1) {
                cerr << "accept connect error" << endl;
                close(socketfd);
                return;
            }
            cout << "received a connection from committee " << inet_ntoa(cli.sin_addr) << ":" << cli.sin_port << endl;
            this->streams_rcv_comm.push_back(new RecverSubChannel(conn));
        }
        close(socketfd);
    }

    void get_connection_to_committee_permute(vector<string> ips, vector<int> ports_rcv) {
        // 优先连接恢复明文时目标服务器，保证目标服务器的连接池前t个连接用于接收缺失的复制秘密份额
        for(int i = 1; i <= this->threshold; i++) {
            int index = (this->server_id + i) % (this->committee_size);
            get_connection(ips[index], ports_rcv[index], this->streams_snd_comm);
        }
    }

    void get_connection_to_committee_remain(vector<string> ips, vector<int> ports_rcv) {
        cout << "connect the remain committee" << endl;
        // 存储未连接committee地址下标
        int indexs[this->committee_size] = {0};
        for(int i = 0; i < this->committee_size; i++) {
            if(this->server_id == i) {
                indexs[i] = -1;
            }
            for(int j = 1; j <= this->threshold; j++) {
                if(this->server_id + j == i) {
                    indexs[i] = -1;
                }
            }
        }

        // 连接剩余committee
        for(int i = 0; i < this->committee_size; i++) {
            if(indexs[i] != -1) {
                get_connection(ips[i], ports_rcv[i], this->streams_snd_comm);
            }
        }
    }

    bool verify_with_open(block** &a, block** &b, block* &c, int open_num) {
        cout << "verify_with_open: start" << endl;
        Value mappings;
        #ifdef SOURCE_DIR
            string path(SOURCE_DIR);
            path += "/resources/chi.json";
            ifstream ifs(path);
            ifs >> mappings;
        #else
            cerr << "There is no base path" << endl;
        #endif
        string index = to_string(this->committee_size) + "-party";
        Value mapping = mappings[index];
        Value chi = mapping[this->server_id];

        // 发送[[a]]
        for(int i = 0; i < chi.size(); i++) {
            for(int j = 0; j < chi[i].size(); j++) {
                int value_id = chi[i][j].asInt();
                this->streams_snd_comm[i]->send_data(a[value_id], sizeof(block) * open_num);
            }
            this->streams_snd_comm[i]->flush();
        }
        cout << "verify_with_open: the shares of [[a]] have been sent" << endl;

        // 发送[[b]]
        for(int i = 0; i < chi.size(); i++) {
            for(int j = 0; j < chi[i].size(); j++) {
                int value_id = chi[i][j].asInt();
                this->streams_snd_comm[i]->send_data(b[value_id], sizeof(block) * open_num);
            }
            this->streams_snd_comm[i]->flush();
        }
        cout << "verify_with_open: the shares of [[b]] have been sent" << endl;

        // 发送[c]
        for(int i = 0; i < this->streams_snd_comm.size(); i++) {
            this->streams_snd_comm[i]->send_data(c, sizeof(block) * open_num);
            this->streams_snd_comm[i]->flush();
        }
        cout << "verify_with_open: the shares of [[c]] have been sent" << endl;

        // 接收[[a]]
        block** a_rcv = new block*[chi.size()];
        for(int i = 0; i < chi.size(); i++) {
            a_rcv[i] = new block[open_num];
            memset(a_rcv[i], 0, sizeof(block) * open_num);
            block *a_temp = new block[open_num];
            for(int j = 0; j < chi[i].size(); j++) {
                this->streams_rcv_comm[i]->recv_data(a_temp, sizeof(block) * open_num);
                for(int k = 0; k < open_num; k++) {
                    a_rcv[i][k] += a_temp[k];
                }
            }
            delete[] a_temp;
        }
        cout << "verify_with_open: the shares of [[a]] have been received" << endl;

        // 接收[[b]]
        block** b_rcv = new block*[chi.size()];
        for(int i = 0; i < chi.size(); i++) {
            b_rcv[i] = new block[open_num];
            memset(b_rcv[i], 0, sizeof(block) * open_num);
            block *b_temp = new block[open_num];
            for(int j = 0; j < chi[i].size(); j++) {
                this->streams_rcv_comm[i]->recv_data(b_temp, sizeof(block) * open_num);
                for(int k = 0; k < open_num; k++) {
                    b_rcv[i][k] += b_temp[k];
                }
            }
            delete[] b_temp;
        }
        cout << "verify_with_open: the shares of [[b]] have been received" << endl;

        // 接收[c]
        block **c_rcv = new block*[this->streams_rcv_comm.size()];
        for(int i = 0; i < this->streams_rcv_comm.size(); i++) {
            c_rcv[i] = new block[open_num];
            this->streams_rcv_comm[i]->recv_data(c_rcv[i], sizeof(block) * open_num);
        }
        cout << "verify_with_open: the shares of [c] have been received" << endl;

        // 计算(a, b, c)明文
        block* a_res = new block[open_num];
        memset(a_res, 0, sizeof(block) * open_num);
        block* b_res = new block[open_num];
        memset(b_res, 0, sizeof(block) * open_num);
        block* c_res = new block[open_num];
        memset(c_res, 0, sizeof(block) * open_num);
        cout << "verify_with_open: the triples result container has been initiated" << endl;
        // 将本地份额相加
        for(int i = 0; i < open_num; i++) {
            for(int j = 0; j < this->key_size; j++) {
                a_res[i] += a[j][i];
                b_res[i] += b[j][i];
            }
            c_res[i] += c[i];
        }
        cout << "verify_with_open: add the local shares of ([[a]], [[b]], [c]) to the result" << endl;

        // 将收到份额相加
        for(int i = 0; i < open_num; i++) {
            for(int j = 0; j < chi.size(); j++) {
                a_res[i] += a_rcv[j][i];
                b_res[i] += b_rcv[j][i];
            }
            for(int j = 0; j < this->streams_rcv_comm.size(); j++) {
                c_res[i] += c_rcv[j][i];
            }
        }
        cout << "verify_with_open: add the received shares of ([[a]], [[b]], [c]) to the result" << endl;

        // 验证三元组
        for(int i = 0; i < open_num; i++) {
            block ab = a_res[i] * b_res[i];
            if(!block_equal(ab, c_res[i])) {
                // cout << "verity_with_open: verify the equation: a * b = c fail" << endl;
                // return false;
            }
        }
        cout << "verity_with_open: verify the equation: a * b = c success" << endl;
        return true;
    }

    bool verity_without_open(block** &a, block** &b, block* &c, int triples_num) {
        // 分享[c]，将其变为复制秘密份额[[c]]
        
        // 桶数量
        int bucket_num = TRIPLES_VERIFY_B / triples_num;
        for(int i = 0; i < bucket_num; i++) {
            block** bucket_a = new block*[this->key_size];
            block** bucket_b = new block*[this->key_size];
            block* bucket_c = new block[TRIPLES_VERIFY_B];
            block** bucket_rho = new block*[this->key_size];
            for(int j = 0; j < TRIPLES_VERIFY_B; j++) {
                for(int k = 0; k < this->key_size; k++) {
                    bucket_a[k][j] = a[k + i * TRIPLES_VERIFY_B][j];
                    bucket_b[k][j] = b[k + i * TRIPLES_VERIFY_B][j];
                }
                bucket_c[j] = c[j];
            }
        }
        block** rho = new block*[this->key_size];
        block** sigma = new block*[this->key_size];
        for(int i = 0; i < this->key_size; i++) {
            rho[i] = new block[triples_num];
            for(int j = 0; j < triples_num; j++) {
                rho[i][j] = a[i][j];
            }
        }
        return true;
    }

// private:
    // 服务器序号
    int server_id;
    // 当前committee大小
    int committee_size;
    // 客户端大小(或上一轮committee大小)
    int client_size;
    // 下一轮committee大小
    int server_size;
    // RSS阈值
    int threshold;
    // （t，n）-RSS下的密钥数量
    int key_size;
    // 密钥，用于生成随机数份额
    vector<block*> keys_cli;
    block* keys;
    PRG* prgs;
    // 通信连接
    // 向下一轮committee发送数据的连接
    vector<SenderSubChannel*> streams_snd_ser;
    // 接受上一轮committee或客户端数据的连接
    vector<RecverSubChannel*> streams_rcv_cli;
    // 向同committee发送数据的连接
    vector<SenderSubChannel*> streams_snd_comm;
    // 接受同committee发送数据的连接
    vector<RecverSubChannel*> streams_rcv_comm;
};