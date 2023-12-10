#include <iostream>
#include <pthread.h>
#include "io/fluid_rss_server.h"

#define DATA_NUM 100

using namespace std;

struct Connection {
    FluidRSSServer& server;
    int port_snd;
    int port_rcv;
};

void* receive_committee(void* arg) {
    cout << "this is child thread" << endl;
    ((Connection*)arg)->server.receive_connection_from_committee(((Connection*)arg)->port_snd, ((Connection*)arg)->port_rcv);
    cout << "================================" << endl;
    return NULL;
}

int main(int argc, const char* argv[]) {
    if(argc < 6) {
        cerr << "at least 6 parameters: ./server <server_id> <committee_size> <client_size> <send port> <receive port>" << endl;
        return -1;
    }
    int id =  atoi(argv[1]);
    int committee_size = atoi(argv[2]);
    int client_size = atoi(argv[3]);
    int port_snd = atoi(argv[4]);
    int port_rcv = atoi(argv[5]);
    // 初始化服务器，并接收客户端连接
    FluidRSSServer ser(id, committee_size, client_size, port_snd, port_rcv);

    // 接收客户端份额及密钥
    int threshould = committee_size % 2 == 0 ? (committee_size / 2 - 1) : (committee_size / 2);
    int share_num = C(threshould, committee_size);
    block** shares = NULL;
    ser.receive_data_from_client(shares, DATA_NUM);

    // 生成三元组
    block** a = NULL;
    block** b = NULL;
    block* c = NULL;
    ser.get_triples_rss(a, b, c, DATA_NUM);

    Value mappings;
    #ifdef SOURCE_DIR
        string path(SOURCE_DIR);
        path += "/resources/servers.json";
        ifstream ifs(path);
        ifs >> mappings;
    #endif
    vector<string> ips;
    vector<int> ports_snd;
    vector<int> ports_rcv;
    for(int i = 0; i < committee_size; i++) {
        ips.push_back(mappings[i]["ip"].asString());
        ports_snd.push_back(mappings[i]["port_snd"].asInt());
        ports_rcv.push_back(mappings[i]["port_rcv"].asInt());
    }

    // 接收committee连接
    Connection connect = {ser, port_snd, port_rcv};
    pthread_t receive_p;
    pthread_create(&receive_p, NULL, receive_committee, &connect);

    // 连接committee
    ser.get_connection_to_committee(ips, ports_snd, ports_rcv);
    pthread_join(receive_p, NULL);

    // 随机置换三元组
    ser.triples_permutation(a, b, c, DATA_NUM);

    // 打开C个三元组验证
    ser.verify_with_open(a, b, c, DATA_NUM);
    // if(shares != NULL) {
    //     for(int i = 0; i < share_num; i++) {
    //         delete[] shares[i];
    //     }
    //     delete[] shares;
    //     shares = NULL;
    // }
    
    return 0;
}