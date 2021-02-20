#include "header.h"

/*
void handle_udp_msg(int fd)
{
    char buf[BUFF_LEN];  //接收缓冲区，1024字节
    socklen_t len;
    int count;
    struct sockaddr_in clent_addr;  //clent_addr用于记录发送方的地址信息
    while(1)
    {
      memset(buf, 0, BUFF_LEN);
      len = sizeof(clent_addr);
      count = recvfrom(fd, buf, BUFF_LEN, 0, (struct sockaddr*)&clent_addr, &len);  //recvfrom是拥塞函数，没有数据就一直拥塞
      if(count == -1)
      {
        printf("recieve data fail!\n");
        return;
      }
      printf("client:%s\n",buf);  //打印client发过来的信息
      memset(buf, 0, BUFF_LEN);
      //sprintf(buf, "I have recieved %d bytes data!\n", count);  //回复client
      sprintf(buf, "ack");
      //printf("server:%s\n",buf);  //打印自己发送的信息给
      sendto(fd, buf, BUFF_LEN, 0, (struct sockaddr*)&clent_addr, len);  //发送信息给client，注意使用了clent_addr结构体指针
    }
}*/

void send_file(int server_fd){
  int receive_id = 0, send_id = 0 ;
  srand(time(0));
  while(1)
	{	
		// 捕获客户端地址
		struct sockaddr_in client_addr;
		socklen_t client_addr_length = sizeof(client_addr);

		/* 接收数据 */
		char buffer[BUFF_LEN];
		bzero(buffer, BUFF_LEN);
		if(recvfrom(server_fd, buffer, BUFF_LEN,0,(struct sockaddr*)&client_addr, &client_addr_length) == -1)
		{
			perror("Receive Data Failed:");
			exit(1);
		}

		/* 从buffer中拷贝出file_name */
		char file_name[FILE_NAME_LEN+1];
		bzero(file_name,FILE_NAME_LEN+1);
		strncpy(file_name, buffer, strlen(buffer)>FILE_NAME_LEN?FILE_NAME_LEN:strlen(buffer));
		printf("%s\n", file_name);

		/* 打开文件 */
		FILE *fp = fopen(file_name, "rb");
		if(NULL == fp)
		{
			printf("File:%s Not Found.\n", file_name);
		}
		else
		{
			int len = 0;
			/* 每读取一段数据，便将其发给客户端 */
      PackInfo pack_info;
      Pack data;
			
      while(1)
			{
        //正常输出
        if(receive_id == send_id){
          data.head.id = ++send_id;
          cout << "-当前即将发送编号为" << send_id << "的数据包，为正常发送-" << endl;
          len = fread(data.buf, sizeof(char), BUFF_LEN, fp);
          if(len > 0){
            data.head.buf_size = len;
            data.getCheckSum();
          }
          else{
            //所有的包传完了
            data.head.id = -1;
            if(sendto(server_fd, (char*)&data, sizeof(data), 0, (struct sockaddr*)&client_addr, client_addr_length) < 0)
            {
              perror("Send File Failed:");
              break;
            }
            break;
          }
        }

        else if(receive_id < send_id){
          data.getCheckSum();
        }

        if(sendto(server_fd, (char*)&data, sizeof(data), 0, (struct sockaddr*)&client_addr, client_addr_length) < 0)
        {
          perror("Send File Failed:");
          break;
        }
        cout << "-编号为" << send_id << "的数据包发送成功-" << endl;

				/* 接收ACK */
				recvfrom(server_fd, (char*)&pack_info, sizeof(pack_info), 0, (struct sockaddr*)&client_addr, &client_addr_length);
				receive_id = pack_info.id;
        cout << "-收到来自客户端的需求，需要编号为" << receive_id  << "的数据包-" << endl;
			}

			/* 关闭文件 */
			fclose(fp);
      cout << ">>>文件" << file_name << "已成功发送<<<" << endl;
      break;
		}
	}
}

int main()
{
    int server_fd, ret;
    struct sockaddr_in ser_addr;

    //创建UDP套接口
    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    //IP地址，需要进行网络序转换，INADDR_ANY：本地地址
    ser_addr.sin_port = htons(SERVER_PORT);  //端口号，需要网络序转换

    //创建socket
    server_fd = socket(AF_INET, SOCK_DGRAM, 0); //AF_INET:IPV4;SOCK_DGRAM:UDP
    if(server_fd < 0)
    {
        printf(">>>Create socket failed<<<\n");
        return -1;
    }
    cout << ">>>套接字已生成<<<" << endl;

    //绑定套接口
    ret = bind(server_fd, (struct sockaddr*)&ser_addr, sizeof(ser_addr));
    if(ret < 0)
    {
        cout << ">>>Socket bind failed<<<\n";
        return -1;
    }
    cout << ">>>套接口已绑定<<<" << endl;

    //handle_udp_msg(server_fd);   //处理接收到的数据

    send_file(server_fd);

    close(server_fd);
    return 0;
}