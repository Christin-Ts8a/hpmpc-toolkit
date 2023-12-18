#include <iostream>
#include <sys/time.h>
#include "io/fluid_sharmir_server.h"

#define DATA_NUM 10000000

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
    FluidSharmirServer ser(id, committee_size, client_size, port_snd, port_rcv);

    struct timeval start, end;
    gettimeofday(&start, NULL);

    block** shares;
    ser.receive_data_from_client(shares, DATA_NUM);

    block** randomness;
    ser.receive_data_from_client(randomness, DATA_NUM + 2);

    ser.epoch(shares, randomness, DATA_NUM);

    gettimeofday(&end, NULL);
    long timeuse_u = end.tv_usec - start.tv_usec;
    long timeuse = end.tv_sec - start.tv_sec;
    cout << "generate share and send the keys: " << timeuse << "s" << endl;
    cout << "generate shares and send the keys: " << timeuse_u << "us" << endl;

    for(int i = 0; i < client_size; i++) {
        delete[] shares[i];
    }
    delete[] shares;
    
    return 0;
}