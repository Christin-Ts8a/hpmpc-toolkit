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

class FluidRSSServer {
public:
    FluidRSSServer(int server_id, int committee_size, int client_size, int port_snd, int port_rcv)
        : server_id(server_id), committee_size(committee_size), client_size(client_size) {
        int threshold = committee_size % 2 == 0 ? (committee_size / 2 - 1) : (committee_size / 2);
        this->key_size = C(threshold, this->committee_size - 1);
        for(int i = 0; i < this->client_size; i++) {
            block* temp_keys = new block[this->key_size];
            this->keys.push_back(temp_keys);
        }
        receive_connection_from_client(port_rcv);
    }
    ~FluidRSSServer() {
        for(int i = 0; i < this->conns_rcv_cli.size(); i++) {
            close(this->conns_rcv_cli[i]);
        }
    }

    // 接受客户端连接
    void receive_connection_from_client(int port_rcv) {
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
            this->conns_rcv_cli.push_back(conn);
            this->streams_rcv_cli.push_back(new RecverSubChannel(conn));
        }
        close(socketfd);
        cout << "All " << client_size << " clients have been connected. The server has been initiated" << endl;
    }

    // 从客户端接受份额以及密钥
    void receive_data_from_client(block** shares, const int data_num) {
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
            cout << "start receiving the shares from clients" << endl;
            for(int i = 0; i < this->streams_rcv_cli.size(); i++) {
                shares[i] = new block[data_num];
                this->streams_rcv_cli[i]->recv_data(shares[i], data_num * sizeof(block));
                cout << "receive shares from client " << i + 1 << ", the shares num: " << data_num << endl;
            }
        }

        // 从客户端接受密钥
        for(int i = 0; i < this->streams_rcv_cli.size(); i++) {
            this->streams_rcv_cli[i]->recv_data(this->keys[i], this->key_size * sizeof(block));
            cout << "receive keys from client " << i + 1 << ", the keys num: " << this->key_size << endl;
        }
    }

private:
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
    vector<block*> keys;
    // 通信连接
    // 向下一轮committee发送数据的连接
    vector<SenderSubChannel*> streams_snd_ser;
    vector<int> conns_snd_ser;
    // 接受上一轮committee或客户端数据的连接
    vector<RecverSubChannel*> streams_rcv_cli;
    vector<int> conns_rcv_cli;
    // 向同committee发送数据的连接
    vector<SenderSubChannel*> streams_snd_comm;
    vector<int> conns_snd_comm;
    // 接受同committee发送数据的连接
    vector<RecverSubChannel*> streams_rcv_comm;
    vector<int> conns_rcv_comm;
};