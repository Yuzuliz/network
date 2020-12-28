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
#include <thread>
using namespace std;

#define BUFF_LEN 1024
#define FILE_NAME_LEN 512
#define SERVER_PORT 1207
#define SERVER_IP "127.0.0.1"
//127.0.0.1Ϊ������IP��ַ
#define SERVER_IP_1 127
#define SERVER_IP_2 0
#define SERVER_IP_3 0
#define SERVER_IP_4 1
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
#pragma comment(lib, "ws2_32.lib")

/* ��ͷ */
class PackInfo
{
public:
    int id;
    int buf_size;
    int checksum;
    int got;
    PackInfo(int index = 0) :id(index), buf_size(0), checksum(0), got(0) {};
};

/* ���հ� */
class Pack
{
public:
    PackInfo head;
    //int len;
    char buf[BUFF_LEN];
    Pack* next;

    Pack(int id = 0) :head(id), next(NULL) {
        memset(buf, '\0', sizeof(buf));
    };

    void copyPack(Pack* p) {
        //copy head
        head = p->head;
        //copy buf
        int i;
        for (i = 0; i < BUFF_LEN; i++)//&& p->buf[i] != '\0';i++)
            buf[i] = p->buf[i];
    };

    void getCheckSum() {
        head.checksum = SERVER_PORT + SERVER_IP_1 + SERVER_IP_2 + SERVER_IP_3 + SERVER_IP_4 + head.id + head.buf_size;
    };
};

class packWindow {
public:
    //��һ����
    Pack* start;
    // ���һ�������������ɰ�ʱ���
    Pack* end;
    // cwnd����ǰ�����������Ͱ��������ڴ�С��
    // threshold��ӵ������״̬�л���׼��
    // currentBags����ǰ���еİ���������Ϊ��״̬�л�ʱ�����ܻ��л���һ�����ȣ�����Щ����û�յ�
    int cwnd, threshold , currentBags;
    packWindow() :start(NULL), end(start), cwnd(1), threshold(32), currentBags(0) {};
    void confirm(int i) {
        //bool found = false;
        Pack* cur;
        for (cur = start; cur; cur = cur->next) {
            if (cur->head.id <= i) {
                cur->head.got = 1;
                //found = true;
            }
        }
        //return found;
    };
    void refresh() {
        if (start->head.got == 0) {
            return;
        }
        Pack* cur = start;
        for (int i = 0; i < cwnd; i++) {
            if (!cur || cur->head.got == 0) {
                break;
            }
            start = cur->next;
            delete cur;
            cur = start;
            currentBags--;
        }
        //cout << "newPackNum:" << newPackNum << endl;
        //return newPackNum;
    };
    void display() {
        cout << ">>>��ǰ���飺";
        for (Pack* cur = start; cur != end; cur = cur->next) {
            cout << cur->head.id << " ";
        }
        cout << end->head.id << "<<<" << endl;
    };
};