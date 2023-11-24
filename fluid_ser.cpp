#include <iostream>
#include "io/fluid_rss_server.h"

#define DATA_NUM 100

using namespace std;

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
    FluidRSSServer ser(id, committee_size, client_size, port_snd, port_rcv);
    block* shares[DATA_NUM];
    ser.receive_data_from_client(shares, DATA_NUM);
    // ser.receive_data_one();
    return 0;
}