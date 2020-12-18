#include "header.h"
clock_t timeStart, timeEnd;
double duration;
packWindow *window = new packWindow;
Pack *sendPack, *curPack;
PackInfo pack_info;
int server_fd;
FILE* fp = NULL;

int receive_id = 1;
// 捕获客户端地址
struct sockaddr_in client_addr;
socklen_t client_addr_length = sizeof(client_addr);
int breakWile = 0;

void send_pack() {
    /*展示当前分组*/
    window->display();
    /*
    sendPack = window->start;
    for (int i = 0 ; sendPack && i < window->cwnd; sendPack = sendPack->next) {
        //发送send数据包
        if (sendPack->head.buf_size > 0)
        {
            cout << "goodPack" << endl;
            sendPack->getCheckSum();

            if (sendto(server_fd, (char*)sendPack, sizeof(*sendPack), 0, (struct sockaddr*)&client_addr, client_addr_length) < 0)
            {
                perror("Send File Failed:");
                breakWile = 1;
            }
            cout << "-编号为" << sendPack->head.id << "的数据包发送成功-" << endl;
        }
        else
        {
            cout << "endPack" << endl;
            //发完了，这是结束标志
            sendPack->head.id = 0;
            //data.head.buf_size = 0;
            sendto(server_fd, (char*)sendPack, sizeof(*sendPack), 0, (struct sockaddr*)&client_addr, client_addr_length);
            breakWile = 1;
            break;
        }
    }*/

    /**/
    //获取要发的包，为send
    sendPack = window->start;
    if (receive_id != sendPack->head.id) {
        for (curPack = sendPack->next; curPack && receive_id != curPack->head.id; curPack = curPack->next);
        if (curPack && receive_id == curPack->head.id) {
            sendPack = curPack;
        }
        cout << "-当前发送编号为" << sendPack->head.id << "的数据包-" << endl;
    }

    //发送send数据包
    if (sendPack->head.buf_size > 0)
    {
        cout << "goodPack" << endl;
        sendPack->getCheckSum();

        if (sendto(server_fd, (char*)sendPack, sizeof(*sendPack), 0, (struct sockaddr*)&client_addr, client_addr_length) < 0)
        {
            perror("Send File Failed:");
            breakWile = 1;
        }
        cout << "-编号为" << sendPack->head.id << "的数据包发送成功-" << endl;
    }
    else
    {
        cout << "endPack" << endl;
        //发完了，这是结束标志
        sendPack->head.id = 0;
        //data.head.buf_size = 0;
        sendto(server_fd, (char*)sendPack, sizeof(*sendPack), 0, (struct sockaddr*)&client_addr, client_addr_length);
        breakWile = 1;
    }
    //for (int i = 0; i < 10000000; i++);
}

void receive_pack() {
    int length = 0, newPackNum = 0;
    /*接收client信息*/
    recvfrom(server_fd, (char*)&pack_info, sizeof(pack_info), 0, (struct sockaddr*)&client_addr, &client_addr_length);
    receive_id = pack_info.id;
    cout << "-收到来自客户端信息，收到最近编号为" << receive_id << "的数据包-" << endl;
    window->confirm(receive_id);

    /*检查是否需要滑动窗口*/
    int lastPack = window->end->head.id;
    newPackNum = window->refresh();
    if (newPackNum == window->cwnd) {
        window->start = new Pack(lastPack + 1);
        length = fread(window->start->buf, sizeof(char), BUFF_LEN, fp);
        window->start->head.buf_size = length;
        window->end = window->start;
        newPackNum--;
    }
    for (int i = 0; i < newPackNum; i++) {
        Pack* newPack = new Pack(window->end->head.id + 1);
        length = fread(newPack->buf, sizeof(char), BUFF_LEN, fp);
        newPack->head.buf_size = length;
        //newPack->getCheckSum();
        window->end->next = newPack;
        window->end = newPack;
    }
}


void transfer_file() {
    /* 每读取一段数据，便将其发给客户端 */
    
    while (1)
    {
        int length = 0;
        /* 接收数据 */
        char buffer[BUFF_LEN];
        //bzero(buffer, BUFF_LEN);
        memset(buffer, '\0', sizeof(buffer));
        if (recvfrom(server_fd, buffer, BUFF_LEN, 0, (struct sockaddr*)&client_addr, &client_addr_length) == -1)
        {
            perror("Receive Data Failed:");
            exit(1);
        }

        /* 从buffer中拷贝出file_name */
        char file_name[FILE_NAME_LEN + 1];
        memset(file_name, '\0', sizeof(file_name));
        strncpy_s(file_name, buffer, strlen(buffer) > FILE_NAME_LEN ? FILE_NAME_LEN : strlen(buffer));
        printf("%s\n", file_name);

        /* 打开文件 */
        fp = fopen(file_name, "rb");
        if (NULL == fp)
        {
            printf("File:%s Not Found.\n", file_name);
        }
        else
        {
            
            //初始化窗口
            window->start = new Pack(1);
            length = fread(window->start->buf, sizeof(char), BUFF_LEN, fp);
            window->start->head.buf_size = length;
            window->end = window->start;
            for (int i = 1; i < window->cwnd; i++) {
                cout << i << endl;
                Pack* newPack = new Pack(i + 1);
                length = fread(newPack->buf, sizeof(char), BUFF_LEN, fp);
                newPack->head.buf_size = length;
                //newPack->getCheckSum();
                window->end->next = newPack;
                window->end = newPack;
                //cout << ">>>the " << newPack->head.id << "th pack<<<" << endl;
            }
            int ssthresh, cwnd, mss;

            timeStart = clock();
            
            while (!breakWile)
            {
                thread t1(send_pack);
                thread t2(receive_pack);
                t1.join();
                t2.join();
            }
            timeEnd = clock();
            /* 关闭文件 */
            fclose(fp);
            cout << ">>>文件" << file_name << "已成功发送<<<" << endl;
            duration = (double)(timeEnd - timeStart) / CLOCKS_PER_SEC;
            cout << "发送文件用时(s):" << duration << endl;		//s为单位
            cout << "发送文件用时(ms):" << duration * 1000 << endl;	//ms为单位
            break;
        }
    }
}

int serverSocketInit() {
    WSADATA WSAData;
    //初始化
    if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0) {
        cout << "初始化失败!" << endl;
        return -1;
    }

    SOCKET server_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (server_socket == INVALID_SOCKET)
    {
        cout << "服务器socket创建失败！" << endl;
        return 0;
    }
    cout << ">>>套接字已生成<<<" << endl;

    //地址参数配置
    SOCKADDR_IN server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);//端口号为1207
    inet_pton(AF_INET, SERVER_IP, (void*)&server_addr.sin_addr.S_un.S_addr);
    //绑定
    if (bind(server_socket, (SOCKADDR*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {//服务器与本地地址绑定
        cout << "服务器socket绑定失败！" << WSAGetLastError() << endl;
        closesocket(server_socket);
        return 0;
    }
    cout << ">>>套接口已绑定<<<" << endl;

    return server_socket;
}

int main()
{
    //初始化
    server_fd = serverSocketInit();
    cout << ">>>客户端套接字已配置好<<<" << endl;
    

    //传数据
    transfer_file();
    
    //收尾
    closesocket(server_fd);
    WSACleanup();
    cout << ">>>客户端socket已成功销毁<<<" << endl;
    return 0;
}