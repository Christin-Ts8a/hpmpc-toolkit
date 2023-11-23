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
    int committee_size = atoi(argv[2]);

    // 初始化客户端
    FluidRSSClient cli(id, committee_size);

    // 生成随机数作为计算任务的输入
    cli.generate_random_int(DATA_NUM);

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
    #endif

    return 0;
}