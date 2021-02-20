#include "header.h"

void send_file(int server_fd){
  int receive_id = 1;//, send_id = 0 ;
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
			//int length = 0;
			/* 每读取一段数据，便将其发给客户端 */
      int k = 0;

      PackInfo pack_info;
      Pack *data1=new Pack(1),*data2=new Pack(2),*data3=new Pack(3),*data4=new Pack(4),*send,*cur;
      data1->next = data2;
      data2->next = data3;
      data3->next = data4;

      cur = data1;
      for(int length = 1 ; length > 0 && cur ; cur = cur->next){
        length = fread(cur->buf, sizeof(char), BUFF_LEN, fp);
        cur->head.buf_size = length;
      }

      while(1)
			{
        /*展示当前分组*/
        cout << ">>>当前分组：" << data1->head.id << " " << data2->head.id << " " << data3->head.id << " " << data4->head.id << "<<<" << endl;

        /*获取要发的包，为send*/
        send = data1;
        if(receive_id != data1->head.id){
          for(cur = data2; cur &&  receive_id != cur->head.id; cur = cur->next);
          if(cur &&  receive_id == cur->head.id){
            send = cur;
          }
          cout << "-当前发送编号为" << send->head.id << "的数据包-" << endl;
        }

        /*发送send数据包*/
        if(send->head.buf_size > 0)
        {
          send->getCheckSum();

          if(sendto(server_fd, (char*)send, sizeof(*send), 0, (struct sockaddr*)&client_addr, client_addr_length) < 0)
					{
						perror("Send File Failed:");
						break;
					}
          cout << "-编号为" << send->head.id << "的数据包发送成功-" << endl;
        }
        else
				{
          //发完了，这是结束标志
          send->head.id = -1;
          //data.head.buf_size = 0;
          sendto(server_fd, (char*)send, sizeof(*send), 0, (struct sockaddr*)&client_addr, client_addr_length) ;
					break;
				}

        /*接收client信息*/
        recvfrom(server_fd, (char*)&pack_info, sizeof(pack_info), 0, (struct sockaddr*)&client_addr, &client_addr_length);
				receive_id = pack_info.id;
        cout << "-收到来自客户端信息，收到最近编号为" << receive_id  << "的数据包-" << endl;
        for(cur = data1 ; cur && cur->head.id != receive_id ; cur = cur->next);
        if(cur && cur->head.id == receive_id){
          cur->head.used = 1;
        }

        /*检查是否需要滑动窗口*/
        if(data1->head.used && !data2->head.used){
          //1234 -> 234*
          data1->copyPack(data2);
          data2->copyPack(data3);
          data3->copyPack(data4);
          data4->head.id = data3->head.id + 1;
          data4->head.buf_size = fread(data4->buf, sizeof(char), BUFF_LEN, fp);
          data4->getCheckSum();
          data4->head.used = 0;
        }
        else if(data1->head.used && data2->head.used && !data3->head.used){
          //1234 -> 34**
          data1->copyPack(data3);
          data2->copyPack(data4);

          data3->head.id = data2->head.id + 1;
          data3->head.buf_size = fread(data3->buf, sizeof(char), BUFF_LEN, fp);
          data3->getCheckSum();
          data3->head.used = 0;

          data4->head.id = data3->head.id + 1;
          data4->head.buf_size = fread(data4->buf, sizeof(char), BUFF_LEN, fp);
          data4->getCheckSum();
          data4->head.used = 0;
        }
        else if(data1->head.used && data2->head.used && data3->head.used && !data4->head.used){
          //1234 -> 4***
          data1->copyPack(data4);
          data2->head.id = data1->head.id + 1;
          data2->head.buf_size = fread(data2->buf, sizeof(char), BUFF_LEN, fp);
          data2->getCheckSum();
          data2->head.used = 0;

          data3->head.id = data2->head.id + 1;
          data3->head.buf_size = fread(data3->buf, sizeof(char), BUFF_LEN, fp);
          data3->getCheckSum();
          data3->head.used = 0;

          data4->head.id = data3->head.id + 1;
          data4->head.buf_size = fread(data4->buf, sizeof(char), BUFF_LEN, fp);
          data4->getCheckSum();
          data4->head.used = 0;

        }
        else if(data1->head.used && data2->head.used && data3->head.used && data4->head.used){
          //1234 -> ****
          data1->head.id = data4->head.id + 1;
          data1->head.buf_size = fread(data1->buf, sizeof(char), BUFF_LEN, fp);
          data1->getCheckSum();
          data1->head.used = 0;

          data2->head.id = data1->head.id + 1;
          data2->head.buf_size = fread(data2->buf, sizeof(char), BUFF_LEN, fp);
          data2->getCheckSum();
          data2->head.used = 0;

          data3->head.id = data2->head.id + 1;
          data3->head.buf_size = fread(data3->buf, sizeof(char), BUFF_LEN, fp);
          data3->getCheckSum();
          data3->head.used = 0;

          data4->head.id = data3->head.id + 1;
          data4->head.buf_size = fread(data4->buf, sizeof(char), BUFF_LEN, fp);
          data4->getCheckSum();
          data4->head.used = 0;
        }
        /*
        for(cur = data1 ; cur && cur->head.used ; cur = cur->next);
        if(cur && !cur->head.used){
          cout << "--" << endl;
        }*/
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