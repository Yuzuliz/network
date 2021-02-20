#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <time.h>
#include <pthread.h>
#include <arpa/inet.h>

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
using namespace std;

#define BUFF_LEN 1024
#define FILE_NAME_LEN 512
#define SERVER_PORT 8888
#define SERVER_IP "127.0.0.1"
#define SERVER_IP_1 127
#define SERVER_IP_2 0
#define SERVER_IP_3 0
#define SERVER_IP_4 1

/* 包头 */
class PackInfo
{
public:
	int id;
	int buf_size;
  int checksum;
  PackInfo(int index=0):id(index),buf_size(0),checksum(0){};
};

/* 接收包 */
class Pack
{
public:
	PackInfo head;
	char buf[BUFF_LEN];
  void getCheckSum(){
    head.checksum = ~(SERVER_PORT & SERVER_IP_1 & SERVER_IP_2 & SERVER_IP_3 & SERVER_IP_4 & head.id & head.buf_size);
  };
};

//int socket(int domain, int type, int protocol);