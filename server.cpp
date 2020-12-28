#include "header.h"
clock_t timeStart, timeEnd;
double duration;
packWindow *window = new packWindow;
Pack *sendPack, *curPack;
PackInfo pack_info;
int server_fd;
FILE* fp = NULL;

int receive_id = 1;
// ����ͻ��˵�ַ
struct sockaddr_in client_addr;
socklen_t client_addr_length = sizeof(client_addr);
int breakWile = 0;

void send_pack() {
    /*չʾ��ǰ����*/
    window->display();
    /*
    sendPack = window->start;
    for (int i = 0 ; sendPack && i < window->cwnd; sendPack = sendPack->next) {
        //����send���ݰ�
        if (sendPack->head.buf_size > 0)
        {
            cout << "goodPack" << endl;
            sendPack->getCheckSum();

            if (sendto(server_fd, (char*)sendPack, sizeof(*sendPack), 0, (struct sockaddr*)&client_addr, client_addr_length) < 0)
            {
                perror("Send File Failed:");
                breakWile = 1;
            }
            cout << "-���Ϊ" << sendPack->head.id << "�����ݰ����ͳɹ�-" << endl;
        }
        else
        {
            cout << "endPack" << endl;
            //�����ˣ����ǽ�����־
            sendPack->head.id = 0;
            //data.head.buf_size = 0;
            sendto(server_fd, (char*)sendPack, sizeof(*sendPack), 0, (struct sockaddr*)&client_addr, client_addr_length);
            breakWile = 1;
            break;
        }
    }*/

    /**/
    //��ȡҪ���İ���Ϊsend
    sendPack = window->start;
    if (receive_id != sendPack->head.id) {
        for (curPack = sendPack->next; curPack && receive_id != curPack->head.id; curPack = curPack->next);
        if (curPack && receive_id == curPack->head.id) {
            sendPack = curPack;
        }
        cout << "-��ǰ���ͱ��Ϊ" << sendPack->head.id << "�����ݰ�-" << endl;
    }

    //����send���ݰ�
    if (sendPack->head.buf_size > 0)
    {
        cout << "goodPack" << endl;
        sendPack->getCheckSum();

        if (sendto(server_fd, (char*)sendPack, sizeof(*sendPack), 0, (struct sockaddr*)&client_addr, client_addr_length) < 0)
        {
            perror("Send File Failed:");
            breakWile = 1;
        }
        cout << "-���Ϊ" << sendPack->head.id << "�����ݰ����ͳɹ�-" << endl;
    }
    else
    {
        cout << "endPack" << endl;
        //�����ˣ����ǽ�����־
        sendPack->head.id = 0;
        //data.head.buf_size = 0;
        sendto(server_fd, (char*)sendPack, sizeof(*sendPack), 0, (struct sockaddr*)&client_addr, client_addr_length);
        breakWile = 1;
    }
    //for (int i = 0; i < 10000000; i++);
}

void receive_pack() {
    int length = 0, newPackNum = 0;
    /*����client��Ϣ*/
    recvfrom(server_fd, (char*)&pack_info, sizeof(pack_info), 0, (struct sockaddr*)&client_addr, &client_addr_length);
    receive_id = pack_info.id;
    cout << "-�յ����Կͻ�����Ϣ���յ�������Ϊ" << receive_id << "�����ݰ�-" << endl;
    window->confirm(receive_id);

    /*����Ƿ���Ҫ��������*/
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
    /* ÿ��ȡһ�����ݣ��㽫�䷢���ͻ��� */
    
    while (1)
    {
        int length = 0;
        /* �������� */
        char buffer[BUFF_LEN];
        //bzero(buffer, BUFF_LEN);
        memset(buffer, '\0', sizeof(buffer));
        if (recvfrom(server_fd, buffer, BUFF_LEN, 0, (struct sockaddr*)&client_addr, &client_addr_length) == -1)
        {
            perror("Receive Data Failed:");
            exit(1);
        }

        /* ��buffer�п�����file_name */
        char file_name[FILE_NAME_LEN + 1];
        memset(file_name, '\0', sizeof(file_name));
        strncpy_s(file_name, buffer, strlen(buffer) > FILE_NAME_LEN ? FILE_NAME_LEN : strlen(buffer));
        printf("%s\n", file_name);

        /* ���ļ� */
        fp = fopen(file_name, "rb");
        if (NULL == fp)
        {
            printf("File:%s Not Found.\n", file_name);
        }
        else
        {
            
            //��ʼ������
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
            /* �ر��ļ� */
            fclose(fp);
            cout << ">>>�ļ�" << file_name << "�ѳɹ�����<<<" << endl;
            duration = (double)(timeEnd - timeStart) / CLOCKS_PER_SEC;
            cout << "�����ļ���ʱ(s):" << duration << endl;		//sΪ��λ
            cout << "�����ļ���ʱ(ms):" << duration * 1000 << endl;	//msΪ��λ
            break;
        }
    }
}

int serverSocketInit() {
    WSADATA WSAData;
    //��ʼ��
    if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0) {
        cout << "��ʼ��ʧ��!" << endl;
        return -1;
    }

    SOCKET server_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (server_socket == INVALID_SOCKET)
    {
        cout << "������socket����ʧ�ܣ�" << endl;
        return 0;
    }
    cout << ">>>�׽���������<<<" << endl;

    //��ַ��������
    SOCKADDR_IN server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);//�˿ں�Ϊ1207
    inet_pton(AF_INET, SERVER_IP, (void*)&server_addr.sin_addr.S_un.S_addr);
    //��
    if (bind(server_socket, (SOCKADDR*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {//�������뱾�ص�ַ��
        cout << "������socket��ʧ�ܣ�" << WSAGetLastError() << endl;
        closesocket(server_socket);
        return 0;
    }
    cout << ">>>�׽ӿ��Ѱ�<<<" << endl;

    return server_socket;
}

int main()
{
    //��ʼ��
    server_fd = serverSocketInit();
    cout << ">>>�ͻ����׽��������ú�<<<" << endl;
    

    //������
    transfer_file();
    
    //��β
    closesocket(server_fd);
    WSACleanup();
    cout << ">>>�ͻ���socket�ѳɹ�����<<<" << endl;
    return 0;
}