#ifndef __NET_H
#define __NET_H
#include <iostream>
#include <vector>
#include <arpa/inet.h>
#include <emp-tool/emp-tool.h>

using namespace std;
using namespace emp;

void get_connection(string ip, int port_rcv, vector<SenderSubChannel*> &streams){
    struct sockaddr_in ser;
    // base on IPV4
    ser.sin_family = AF_INET;
    // character order transfer
    ser.sin_port = htons(port_rcv);
    ser.sin_addr.s_addr = inet_addr(ip.c_str());
    cout << "start connecting committee: " << ip << ":" << port_rcv << endl;
    while(1) {
        int socketfd = socket(AF_INET, SOCK_STREAM, 0);
        if(socketfd == -1) {
            cerr << "Prepare socket error" << endl;
            return;
        }
        int conn = connect(socketfd, (struct sockaddr*)&ser, sizeof(ser));
        if(conn == 0) {
            streams.push_back(new SenderSubChannel(socketfd));
            cout << "create a connection to the committee " << inet_ntoa(ser.sin_addr) << ":" << port_rcv << endl;
            break;
        }
        close(socketfd);
    }
}
void get_connection(string ip, int port_rcv, vector<int> &streams){
    struct sockaddr_in ser;
    // base on IPV4
    ser.sin_family = AF_INET;
    // character order transfer
    ser.sin_port = htons(port_rcv);
    ser.sin_addr.s_addr = inet_addr(ip.c_str());
    cout << "start connecting committee: " << ip << ":" << port_rcv << endl;
    while(1) {
        int socketfd = socket(AF_INET, SOCK_STREAM, 0);
        if(socketfd == -1) {
            cerr << "Prepare socket error" << endl;
            return;
        }
        int conn = connect(socketfd, (struct sockaddr*)&ser, sizeof(ser));
        if(conn == 0) {
            streams.push_back(socketfd);
            cout << "create a connection to the committee " << inet_ntoa(ser.sin_addr) << ":" << port_rcv << endl;
            break;
        }
        close(socketfd);
    }
}
#endif