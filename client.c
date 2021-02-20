#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// unistd.h为Linux/Unix系统中内置头文件，包含许多系统服务的函数原型，如read函数、write函数和getpid函数等。
// 其作用相当于"windows.h"，是操作系统为用户提供的统一API接口，方便调用系统提供的一些服务
#include <unistd.h>
//定义了socket的一些地址信息
#include <netinet/in.h>
#include <arpa/inet.h>
// 基础的socket库
#include <sys/socket.h>
// Linux系统下的多线程遵循POSIX线程接口，称为pthread
#include <pthread.h>
#include <time.h>

int clientSocket;//客户端socket
char* IP = "127.0.0.1";//服务器的IP
short PORT = 1207;//服务器服务端口
typedef struct sockaddr meng;
char name[16];//设置支持的用户名长度
time_t nowtime;

void init(){
  // socket：创建套接字
  // 地址类型：PF_INET:protocol family协议族，AF_INET:address family地址族，bits/socket_type.h中有#define AF_INET		PF_INET，故无差别
  // 服务类型：SOCK_STREAM，为流式套接字
  // 协议类型：为0，由系统自动选择
  clientSocket = socket(PF_INET,SOCK_STREAM,0);

  /*地址设置*/
  //将套接字存在sockaddr_in结构体中
  struct sockaddr_in addr;
  addr.sin_family = PF_INET;
  addr.sin_port = htons(PORT);
  //inet_addr()：将点分十进制的字符串转换为32位网络字节顺序ip信息
  addr.sin_addr.s_addr = inet_addr(IP);

  // connect：发起连接
  if (connect(clientSocket,(meng*)&addr,sizeof(addr)) == -1){
    perror("---------------无法连接服务器---------------\n");
    exit(-1);
  }
  printf("---------------客户端启动成功---------------\n");
}

// 收消息
void* recv_thread(void* p){
  while(1){
    char inputBuf[100] = {};

    // recv函数，内核从对端接受数据，放在socket的缓存中，然后复制到应用层的buffer，共两个buffer
    if (recv(clientSocket,inputBuf,sizeof(inputBuf),0) <= 0){
      break;
    }
    // 打印
    printf("%s\n",inputBuf);
  }
}

void start(){
  int i;
  pthread_t id;
  void* recv_thread(void*);
  //创建线程用于收消息
  pthread_create(&id,0,recv_thread,0);

  // 初次使用send函数，显示有人上线了
  char memberBuf[100] = {};
  sprintf(memberBuf,"\n                          %s进入了聊天室\n", name);
  /*time(&nowtime);
  printf("%s************************\n", ctime(&nowtime));*/
  // socket：由此socket发出send请求
  // inputBuf：所发送数据的缓冲区
  // len：缓冲区长度
  // flags：对调用的处理方式，默认为0
  send(clientSocket, memberBuf, strlen(memberBuf), 0);

  while(1){
    //输入
    char inputBuf[100] = {};
    char c;
    for(i = 0 ; i < 99 ; i++){
      c = getchar();
      if(c != '\n')
        inputBuf[i] = c;
      else
        break;
    }
    inputBuf[i]='\0';
    //scanf("%s",inputBuf);

    // 输入exit，退出群聊
    if (strcmp(inputBuf,"exit") == 0){
      memset(memberBuf,0,sizeof(memberBuf));//初始化
      sprintf(memberBuf,"\n                          %s退出了聊天室\n",name);
      send(clientSocket,memberBuf,strlen(memberBuf),0);
      break;
    }
    else{
      char msg[100] = {};
      sprintf(msg,"%s：%s",name,inputBuf);
      send(clientSocket,msg,strlen(msg),0);
    }
  }

  //关闭socket
  close(clientSocket);
}

int main(){
  int nameIndex;
  char s;

  //初始化客户端
  init();

  // 客户端启动成功，用户输入昵称
  printf("请输入用户名(不超过15个字)：");
  s = getchar();
  for(nameIndex = 0 ; nameIndex < 15 & s != '\n'; nameIndex++){
    name[nameIndex] = s;
    s = getchar();
  }
  name[nameIndex]='\0';
  // 超过15个字的不算
  while(s != '\n'){
    s = getchar();
  }

  // 进入聊天室
  printf(">>>欢迎 %s 进入聊天室，您可输入exit退出<<<",name);
  printf("\n********************************************\n");
  start();
  printf("\n****************************\n");
  printf("\n*******您已退出聊天室*******\n");
  printf("\n****************************\n");
  return 0;
}