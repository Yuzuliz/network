#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <time.h>

#define member 2// 聊天室最多人数

int serverSocket;//服务器socket
int clients[2*member];//客户端
char* IP = "127.0.0.1";//主机ip地址
short PORT = 1207;//端口号
typedef struct sockaddr orin;
time_t nowtime;


void init(){
  // socket：创建套接字
  serverSocket = socket(PF_INET,SOCK_STREAM,0);
  if (serverSocket == -1){
    perror("创建socket失败");
    exit(-1);
  }

  // 地址设置
  //为套接字设置ip协议 设置端口号  并自动获取本机ip转化为网络ip
  struct sockaddr_in addr;//存储套接字的信息
  addr.sin_family = PF_INET;//地址族
  addr.sin_port = htons(PORT);//设置server端端口号，当sin_port = 0时，系统随机选择一个未被使用的端口号
  addr.sin_addr.s_addr = inet_addr(IP);//把127.0.0.1改为自己的server端的ip地址,当sin_addr = INADDR_ANY时，表示从本机的任一网卡接收数据

  /*绑定套接字*/
  // socket：等待被绑定的socket
  // addr：本地的地址&端口号
  // namelen：限制长度
  if (bind(serverSocket,(orin*)&addr,sizeof(addr)) == -1){
    perror("绑定失败");
   exit(-1);
  }

  /***********listen：监听客户端socket***********/
  // s：服务器的socket
  // backlog：等待队列最大长度——最多可连接服务器数
  if (listen(serverSocket,member) == -1){
    //监听最大连接数
    perror("设置监听失败");
    exit(-1);
  }
}

void SendAll(char* msg){
  int i;
  for (i = 0;i < member;i++){
    // 有对应的客户端
    if (clients[i] != 0){
      printf("发送给%d： %s\n",clients[i],msg);
      
      char buf[1024];
      // 追加形式填写服务器监听日志
      FILE *logs = fopen("log.txt", "a+");
      if(!logs){
        printf(">>>open file error<<< \n");
      }
      else{
        time(&nowtime);
        sprintf(buf, "信息时间：%s\tIP地址：%s\n",ctime(&nowtime),IP);
        fputs(buf,logs);
        sprintf(buf, "信息内容：%s\n\n",msg);
        fputs(buf,logs);
        fclose(logs);
      }
      
      // 给连接的所有客户端发送消息
      send(clients[i],msg,strlen(msg),0);
      }
  }
}

void* server_thread(void* p){
  // 将p转换成int指针，再取其指向的值
  int fd = *(int*)p;
  printf("pthread = %d\n",fd);
  while(1){
    char buf[100] = {};
    // recv函数：从socket接收数据，缓存至长度为len的buf缓冲区，对调用的处理方式默认为0
    if (recv(fd,buf,sizeof(buf),0) <= 0){
      // 断开服务器与该客户端的连接
      int i;
      for (i = 0;i < member;i++){
        if (fd == clients[i]){
          clients[i] = 0;
          break;
        }
      }

      // 监听日志中记录退出信息
      printf("退出：fd = %d 退出了。\n",fd);
      char buf[1024];
      FILE *logs = fopen("log.txt", "a");
      if(logs== NULL){
        printf("open file error! \n");
      }else{
        sprintf(buf, "退出时间：%s\tIP地址：%s\n",
        ctime(&nowtime),IP);
        fputs(buf,logs);
        fclose(logs);
      }
      // 终止线程
      pthread_exit(0);
    }

    //把服务器接收到的信息群发
    SendAll(buf);
  }
}

void server(){
  printf("服务器启动，开始监听...\n");
  while(1){

    struct sockaddr_in fromAddr;
    socklen_t len = sizeof(fromAddr);
    /***************accept函数***************
     * s：发起请求的socket                                       *
     * addr：socket的地址                                        *
     * addrlen：socket的地址长度                         *
     * 返回服务器为客户端生成的对应小socket *
     *****************************************/
    int newClient = accept(serverSocket,(orin*)&fromAddr,&len),currentIndex;
    //调用accept进入堵塞状态，等待客户端的连接
    if (newClient == -1){
      printf("客户端连接出错...\n");
      continue;
    }
    for(currentIndex = 0 ; currentIndex<member && clients[currentIndex] != 0; currentIndex++);
    // 有空位，新客户端成功连接
    if(currentIndex < member){
      // 记录客户端的socket
      clients[currentIndex] = newClient;
      printf("线程号：%d\n",newClient);
      // 启动线程
      pthread_t threadId;
      pthread_create(&threadId,0,server_thread,&newClient);
    }

    // 聊天室满员
    else{
      char* str = "聊天室满啦，等会儿再来吧~";
      send(newClient,str,strlen(str),0);
      close(newClient);
    }
  }
}

int main(){
  init();
  server();
  return 0;
}