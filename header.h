#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include<ws2tcpip.h>
#include<winsock2.h>
#include <Windows.h>
using namespace std;

#define BUFF_LEN 1024
#define FILE_NAME_LEN 512
#define SERVER_PORT 1207
#define SERVER_IP "127.0.0.1"
#define SERVER_IP_1 127
#define SERVER_IP_2 0
#define SERVER_IP_3 0
#define SERVER_IP_4 1
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
#pragma comment(lib, "ws2_32.lib")

/* 包头 */
class PackInfo
{
public:
    int id;
    int buf_size;
    int checksum;
    int got;
    PackInfo(int index = 0) :id(index), buf_size(0), checksum(0), got(0) {};
};

/* 接收包 */
class Pack
{
public:
    PackInfo head;
    //int len;
    char buf[BUFF_LEN];
    Pack* next;

    Pack(int id = 0) :head(id), next(NULL) {};

    void copyPack(Pack* p) {
        //copy head
        head = p->head;
        //copy buf
        int i;
        for (i = 0; i < BUFF_LEN; i++)//&& p->buf[i] != '\0';i++)
            buf[i] = p->buf[i];
        /*
          if(i < BUFF_LEN)
          buf[i] = '\0';*/
    };

    void getCheckSum() {
        head.checksum = SERVER_PORT + SERVER_IP_1 + SERVER_IP_2 + SERVER_IP_3 + SERVER_IP_4 + head.id + head.buf_size;
    };
};

class packWindow {
public:
    Pack* start;
    Pack* end;
    int cwnd, threshold;
    packWindow() :start(NULL), end(start), cwnd(10), threshold(0) {};
    void confirm(int i) {
        Pack* cur;
        for (cur = start; cur; cur = cur->next) {
            if (cur->head.id <= i) {
                cur->head.got = 1;
            }
            else {
                break;
            }
        }

    };
    int refresh() {
        Pack* cur = start;
        int newPackNum = 0;
        for (int i = 0; i < cwnd; i++) {
            if (!cur || cur->head.got == 0) {
                break;
            }
            start = cur->next;
            delete cur;
            cur = start;
            newPackNum++;
        }
        return newPackNum;
    };
    void display() {
        cout << ">>>当前分组：";
        for (Pack* cur = start; cur != end; cur = cur->next) {
            cout << cur->head.id << " ";
        }
        cout << end->head.id << "<<<" << endl;
    };
};