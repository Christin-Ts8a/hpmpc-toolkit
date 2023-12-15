#include <iostream>
#include <pthread.h>
#include <sys/time.h>
#include "io/fluid_rss_server.h"
#include "utils/constant.h"

#define DATA_NUM 1000000

using namespace std;

struct Connection {
    FluidRSSServer& server;
    int port_rcv;
    int threshould;
};

void* receive_committee_permute(void* arg) {
    cout << "this is child thread" << endl;
    ((Connection*)arg)->server.receive_connection_from_committee(
        ((Connection*)arg)->port_rcv, ((Connection*)arg)->threshould
        );
    cout << "================================" << endl;
    return NULL;
}

int main(int argc, const char* argv[]) {
    if(argc < 6) {
        cerr << "at least 6 parameters: ./server <server_id> <committee_size> <client_size> <send port> <receive port>" << endl;
        return -1;
    }
    int server_id =  atoi(argv[1]);
    int committee_size = atoi(argv[2]);
    int client_size = atoi(argv[3]);
    int port_snd = atoi(argv[4]);
    int port_rcv = atoi(argv[5]);
    // 初始化服务器，并接收客户端连接
    FluidRSSServer ser(server_id, committee_size, client_size, port_rcv);

    struct timeval start, end;
    gettimeofday(&start, NULL);

    // 接收客户端份额及密钥
    int threshould = committee_size % 2 == 0 ? (committee_size / 2 - 1) : (committee_size / 2);
    int share_num = C(threshould, committee_size);
    block** shares = NULL;
    ser.receive_data_from_client(shares, DATA_NUM);

    // 生成三元组
    block** a = NULL;
    block** b = NULL;
    block* c = NULL;
    ser.get_triples_rss(a, b, c, DATA_NUM * TRIPLES_VERIFY_B + TRIPLES_VERIFY_C);

    Value mappings;
    #ifdef SOURCE_DIR
        string path(SOURCE_DIR);
        path += "/resources/servers.json";
        ifstream ifs(path);
        ifs >> mappings;
    #endif
    vector<string> ips;
    vector<int> ports_rcv;
    for(int i = 0; i < committee_size; i++) {
        ips.push_back(mappings[i]["ip"].asString());
        ports_rcv.push_back(mappings[i]["port_rcv"].asInt());
    }

    // 接收committee连接
    Connection connect_permute = {ser, port_rcv, threshould};
    pthread_t receive_permute;
    pthread_create(&receive_permute, NULL, receive_committee_permute, &connect_permute);

    // 连接committee
    ser.get_connection_to_committee_permute(ips, ports_rcv);
    pthread_join(receive_permute, NULL);

    // 随机置换三元组
    ser.triples_permutation(a, b, c, DATA_NUM * TRIPLES_VERIFY_B + TRIPLES_VERIFY_C);

    // 接收剩余committee连接
    Connection connect_remain = {ser, port_rcv, committee_size - threshould - 1};
    pthread_t receive_remain;
    pthread_create(&receive_remain, NULL, receive_committee_permute, &connect_remain);

    // 连接committee
    ser.get_connection_to_committee_remain(ips, ports_rcv);
    pthread_join(receive_remain, NULL);

    // 打开C个三元组验证
    // ser.verify_with_open(a, b, c, DATA_NUM);

    // 验证剩余三元组
    // ser.verity_without_open(a, b, c, DATA_NUM * TRIPLES_VERIFY_B);
    gettimeofday(&end, NULL);
    long timeuse_u = end.tv_usec - start.tv_usec;
    long timeuse = end.tv_sec - start.tv_sec;
    cout << "generate share and send the keys: " << timeuse << "s" << endl;
    cout << "generate shares and send the keys: " << timeuse_u << "us" << endl;
    // if(shares != NULL) {
    //     for(int i = 0; i < share_num; i++) {
    //         delete[] shares[i];
    //     }
    //     delete[] shares;
    //     shares = NULL;
    // }
    
    return 0;
}