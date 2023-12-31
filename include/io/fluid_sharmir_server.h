#include <iostream>
#include <emp-tool/emp-tool.h>
#include <jsoncpp/json/json.h>

using namespace std;
using namespace emp;
using namespace Json;

class FluidSharmirServer {
public:
    FluidSharmirServer(int server_id, int committee_size, int client_size, int port_snd, int port_rcv)
        : server_id(server_id), committee_size(committee_size), client_size(client_size) {
        receive_connection_from_client(port_rcv);
    }

    ~FluidSharmirServer() {}

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
            this->streams_rcv_cli.push_back(new RecverSubChannel(conn));
        }
        close(socketfd);
        cout << "All " << client_size << " clients have been connected. The server has been initiated" << endl;
    }

    // 从客户端接受份额以及密钥
    void receive_data_from_client(block** &shares, const int data_num) {
        shares = new block*[this->client_size];
        for(int i = 0; i < streams_rcv_cli.size(); i++) {
            shares[i] = new block[data_num];
            this->streams_rcv_cli[i]->recv_data(shares[i], sizeof(block) * data_num);
            cout << "received " << data_num << " data from the " << i + 1 << "th client" << endl;
        }
    }

    void epoch(block** &shares, block** &randomness, int multi_num){
        block** r_shares = new block*[this->client_size];
        // r·z
        for(int i = 0; i < this->client_size; i++) {
            r_shares[i] = new block[multi_num];
            for(int j = 0; j < multi_num; j++) {
                r_shares[i][j] = shares[i][j] * randomness[i][j];
            }
        }
        // z·z
        block** result = new block*[this->client_size];
        for(int i = 0; i < this->client_size; i++) {
            result[i] = new block[multi_num];
            for(int j = 0; j < multi_num; j++) {
                r_shares[i][j] * r_shares[i][j];
            }
        }
        // alphi
        for(int i = 0; i < this->client_size; i++) {
            for(int j = 0; j < multi_num; j++) {
                randomness[i][j] *= randomness[i][multi_num];
            }
        }
        // alphi·z
        for(int i = 0; i < this->client_size; i++) {
            for(int j = 0; j < multi_num; j++) {
                result[i][j] * randomness[i][j];
            }
        }
        // alphi·z·r
        for(int i = 0; i < this->client_size; i++) {
            for(int j = 0; j < multi_num; j++) {
                result[i][j] * randomness[i][j] * randomness[i][multi_num + 1];
            }
        }
        cout << "epoch is over" << endl;
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