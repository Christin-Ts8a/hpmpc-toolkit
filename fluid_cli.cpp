#include <iostream>
#include <string>
#include <emp-tool/emp-tool.h>
#include <jsoncpp/json/json.h>
#include "io/fluid_rss_client.h"

#define DATA_NUM 100

using namespace std;
using namespace emp;
using namespace Json;

int main(int argc, const char* argv[]) {
    int id = atoi(argv[1]);
    int server_size = atoi(argv[2]);

    // 初始化客户端
    FluidRSSClient cli(id, server_size);

    // 生成随机数作为计算任务的输入
    // cli.generate_random_int(DATA_NUM);

    // 获取计算任务输入
    int* data = new int[DATA_NUM];
    #ifdef SOURCE_DIR
        string path(SOURCE_DIR);
        path += "/resources/data.json";
        cli.get_dataset(path, data, DATA_NUM);
    #endif
    
    // 生成输入对应份额
    block* share = new block[DATA_NUM];
    cli.get_shares_from_dataset(share, data, DATA_NUM);

    // 获取与服务器的连接
    Value value;
    #ifdef SOURCE_DIR
        string filepath(SOURCE_DIR);
        filepath += "/resources/servers.json";
        ifstream ifs(filepath);
        ifs >> value;
        cout << "server address has been read" << endl;
        vector<string> ips;
        vector<int> ports;
        for(int i = 0; i < server_size; i++) {
            ips.push_back(value[i]["ip"].asString());
            ports.push_back(value[i]["port_rcv"].asInt());
        }
        cli.get_connection_to_servers(ips, ports, server_size);
    #endif

    // 向服务器发送密钥以及份额
    cli.send_data_to_server(share, DATA_NUM);
    // cli.send_data_one();
    

    return 0;
}