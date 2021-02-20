#include "header.h"


void udp_msg_sender(int fd, struct sockaddr* dst)
{
  socklen_t len;
  struct sockaddr_in src;
  char buf[BUFF_LEN] = {},answer[BUFF_LEN]="nak";
  int i,j;
  while(1)
  {
    cout << "请输入要发送的信息（不得超过512字），输入exit退出" << endl;
    char c;
    for(i = 0 ; i < BUFF_LEN-1 ; i++){
      c = getchar();
      if(c != '\n')
        buf[i] = c;
      else
        break;
    }
    buf[i]='\0';
    //char buf[BUFF_LEN] = "TEST UDP MSG!\n";
    if (strcmp(buf,"exit") == 0){
      break;
    }
    len = sizeof(*dst);
    //printf("client:%s\n",buf);  //打印自己发送的信息
    for(j = 0 ; j < 10 && strcmp(answer,"ack") != 0 ; j++){
      sendto(fd, buf, BUFF_LEN, 0, dst, len);
      memset(answer, 0, BUFF_LEN);
      recvfrom(fd, answer, BUFF_LEN, 0, (struct sockaddr*)&src, &len);  //接收来自server的信息
    }
    if( strcmp(answer,"ack") == 0){
      cout << "client:" << buf << "                            >>>DELIVERED<<<" << endl;
    }
    else{
      cout << "client:" << buf << "                            >>>ERROR<<<" << endl;
    }
  }
    memset(buf, 0, BUFF_LEN);
    memset(answer, 0, BUFF_LEN);
}

void setLog(string s, string t, double rate){
  time_t timeSec=time (NULL); //获取1970.1.1至当前秒数time_t
  struct tm * timeinfo= localtime ( &timeSec ); 
  ofstream outfile("/home/idaguo/codes/network/lab3-1/log.txt",ios::app);
  outfile << timeinfo->tm_year+1900 << "-" << timeinfo->tm_mon+1 << "-" << timeinfo->tm_mday << " " << timeinfo->tm_hour << ":" << timeinfo->tm_min << ":" << timeinfo->tm_sec << "          " << s << "          " << t << "          " << rate << endl;
};

void recv_file(int client_fd, struct sockaddr_in server_addr){
  socklen_t server_addr_length = sizeof(server_addr);
  Pack data;
  int currentId=1;

  /* 输入文件名到缓冲区 */
	char source_file_name[FILE_NAME_LEN+1],target_file_name[FILE_NAME_LEN+1];
	bzero(source_file_name, FILE_NAME_LEN+1);
	cout << "请输入所需客户端文件名：";
	cin >> source_file_name;
  cout << "请输入目标用户端文件名：";
	cin >> target_file_name;
  cout << "-------------------------------------------------------------------" << endl;

	char buffer[BUFF_LEN];
	bzero(buffer, BUFF_LEN);
	strncpy(buffer, source_file_name, strlen(source_file_name)>BUFF_LEN?BUFF_LEN:strlen(source_file_name));

	/* 发送文件名 */
	if(sendto( client_fd, buffer, BUFF_LEN,0,(struct sockaddr*)&server_addr,server_addr_length) < 0)
	{
    cout << "ERROR:\tSend File Name Failed"  << endl;
		exit(1);
	}

  ofstream outfile(target_file_name, ios::binary|ios::app);
  if (!outfile.is_open()) 
    { 
        cout << "ERROR:\tFile:\t" << target_file_name << " Can Not Open To Write" << endl;
    }

	/* 从服务器接收数据，并写入文件 */
	int len = 0;
	while(1)
	{
		PackInfo pack_info;

    if((len = recvfrom( client_fd, (char*)&data, sizeof(data), 0, (struct sockaddr*)&server_addr,&server_addr_length)) > 0)
		{
      if(data.head.id == -1){
        //文件发完了
        break;
      }
      if(data.head.id != currentId){
        //收到的不需要，扔掉，返回currentId
        cout << "-收到了编号为" << data.head.id << "的数据包但不需要-" << endl;
        cout << data.head.id << " " << currentId << endl;
        pack_info.id = currentId;
				//pack_info.buf_size = data.head.buf_size;
      }
      else{
        //收到的是需要的
        if(data.head.checksum != ~(SERVER_PORT & SERVER_IP_1 & SERVER_IP_2 & SERVER_IP_3 & SERVER_IP_4 & data.head.id & data.head.buf_size)){
          //包坏了
          cout << "-收到了编号为" << data.head.id << "的数据包，经校验是损坏包-" << endl;
          pack_info.id = currentId;
        }
        else{
          //包可用
          cout << "-收到了编号为" << data.head.id << "的数据包，可用-" << endl;
          //cout << data.buf;
          //更新返回信息
  				pack_info.id = data.head.id;
	  			pack_info.buf_size = data.head.buf_size;
		  		currentId++;
          //写文件
          for(int dataIndex = 0 ; dataIndex < data.head.buf_size ; dataIndex++){
            outfile << data.buf[dataIndex];
          }
          cout << "-编号为"  << data.head.id << "的数据包已写入文件-" << endl;
        }
      }

      //发送ack
      if(sendto( client_fd, (char*)&pack_info, sizeof(pack_info), 0, (struct sockaddr*)&server_addr, server_addr_length) < 0)
      {
        printf("Send confirm information failed!");
      }
      else{
        cout << "-ACK" << pack_info.id << endl;
      }
		}
		else
		{
			/*Do nothing.*/
		}
	}
  cout << "----------------------------------------------------------------" << endl;
	cout << "Receive File:\t" << source_file_name << " From Server IP Successful!\n";
  outfile.close();
}

int main()
{
    int client_fd;
    struct sockaddr_in server_addr;

    //服务端地址获取
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);  //注意网络序转换
    server_addr.sin_port = htons(SERVER_PORT);  //注意网络序转换

    //创建socket
    client_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(client_fd < 0)
    {
        cout << "create socket fail!\n";
        return -1;
    }
    cout << ">>>套接字已生成<<<" << endl;

    recv_file(client_fd, server_addr);

    //udp_msg_sender(client_fd, (struct sockaddr*)&server_addr);

    close(client_fd);

    return 0;
}