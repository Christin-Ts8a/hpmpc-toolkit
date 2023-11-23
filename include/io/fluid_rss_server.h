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
        
        receive_connection_from_client(port_rcv);
    }
    ~FluidRSSServer() {
        for(int i = 0; i < this->conns_rcv_cli.size(); i++) {
            close(this->conns_rcv_cli[i]);
        }
    }
    void receive_connection_from_client(int port_rcv) {
        int socketfd = socket(AF_INET, SOCK_STREAM, 0);
        if(socketfd == -1) {
            cerr << "Prepare socket error" << endl;
            close(socketfd);
            return;
        }
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
    block* keys;
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