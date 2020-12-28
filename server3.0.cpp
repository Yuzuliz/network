#include "header.h"
clock_t timeStart, timeEnd;
double duration;
packWindow *window = new packWindow;
Pack *sendPack, *curPack;
PackInfo pack_info;
int server_fd;
FILE* fp = NULL;

//客户端最新收到的包
int receive_id = 1;
// 客户端地址
struct sockaddr_in client_addr;
socklen_t client_addr_length = sizeof(client_addr);
//收发循环标志位
int breakWile = 0;
//收发锁
int sendable = 1, receiveable = 0;
//reno算法标志位，1为慢启动2为线性增长
int posSign = 1;

void send_pack() {
    while (!breakWile && sendable) {
        receiveable = 0;
        cout << "------------send-----------" << endl;
        //展示当前分组
        window->display();
        if (receive_id < window->start->head.id - 1) {
            perror("send:::ERROR");
            return;
        }
        sendPack = window->start;
        for (int i = 0 ; sendPack && i < window->cwnd; sendPack = sendPack->next) {
            //发送send数据包
            if (sendPack->head.got != 1 && sendPack->head.buf_size > 0)
            {
                cout << "---goodPack " << sendPack->head.id << " : ";
                sendPack->getCheckSum();

                if (sendto(server_fd, (char*)sendPack, sizeof(*sendPack), 0, (struct sockaddr*)&client_addr, client_addr_length) < 0)
                {
                    perror("send:::Send File Failed:");
                    breakWile = 1;
                }
                cout << "success---" << endl;
            }
            else
            {
                cout << "send:::endPack" << endl;
                //发完了，这是结束标志
                sendPack->head.id = 0;
                //data.head.buf_size = 0;
                sendto(server_fd, (char*)sendPack, sizeof(*sendPack), 0, (struct sockaddr*)&client_addr, client_addr_length);
                breakWile = 1;
                break;
            }
        }
        cout << "---------------------------" << endl;
        receiveable = 1;
        sendable = 0;
    }

    //for (int i = 0; i < 10000000; i++);
}

void receive_pack() {
    int length = 0, newPackNum = 0;
    while (!breakWile && receiveable) {
        sendable = 0;
        cout << "----------receive----------" << endl;
        for (int i = 0; i < window->cwnd; i++) {
            //接收client信息
            recvfrom(server_fd, (char*)&pack_info, sizeof(pack_info), 0, (struct sockaddr*)&client_addr, &client_addr_length);
            receive_id = pack_info.id;
            cout << "-来自客户端，收到最新为" << receive_id << "号包-" << endl;
            window->confirm(receive_id);
        }
        if (receive_id < window->start->head.id - 1) {
            perror("receive:::ERROR");
        }

        cout << "---------更新窗口----------" << endl;
        // 检查是否需要滑动窗口
        int lastPack = window->end->head.id;
        newPackNum = window->refresh();
        // 慢启动
        if (posSign == 1) {
            newPackNum += window->cwnd;
            window->cwnd *= 2;
            if (window->cwnd >= window->threshold) {
                posSign = 2;
            }
        }
        // 拥塞避免，线性增加或回到慢启动
        if (posSign == 2) {
            newPackNum++;
            window->cwnd++;
            // 模拟loss事件
            if (window->cwnd == 64) {
                window->threshold = window->cwnd / 2;
                newPackNum -= window->cwnd / 2;
                // timeout
                // window->cwnd = 1;
                // posSign = 1;
                // triple duplicate ACK
                window->cwnd = window->threshold + 3;
                newPackNum += 3;
            }
        }
        cout << "cwnd : " << window->cwnd << endl;
        cout << "newPackNum : " << newPackNum << endl;
        if (newPackNum == window->cwnd) {
            window->start = new Pack(lastPack + 1);
            length = fread(window->start->buf, sizeof(char), BUFF_LEN, fp);
            window->start->head.buf_size = length;
            window->end = window->start;
            newPackNum--;
        }
        if (length >= 0) {
            for (int i = 0; i < newPackNum; i++) {
                Pack* newPack = new Pack(window->end->head.id + 1);
                length = fread(newPack->buf, sizeof(char), BUFF_LEN, fp);
                newPack->head.buf_size = length;
                //newPack->getCheckSum();
                window->end->next = newPack;
                window->end = newPack;
                if (length < 0) {
                    break;
                }
            }
        }
        
        window->display();
        cout << "---------------------------" << endl;
        receiveable = 0;
        sendable = 1;
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
                //cout << i << endl;
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