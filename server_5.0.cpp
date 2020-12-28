#include "header.h"
clock_t timeStart, timeEnd;
double duration;
packWindow *window = new packWindow;
Pack *sendPack, *curPack;
PackInfo pack_info;
int server_fd;
FILE* fp = NULL;

//�ͻ��������յ��İ�
int receive_id = 1;
// �ͻ��˵�ַ
struct sockaddr_in client_addr;
socklen_t client_addr_length = sizeof(client_addr);
//�շ�ѭ����־λ
int breakWile = 0;
//�շ���
int sendable = 1, receiveable = 0;
//reno�㷨��־λ��1Ϊ������2Ϊ��������
int posSign = 1;
//����ack�жϣ�ack���ظ��İ�id��duplicateACK�Ǹ�id���ִ���
int ack = 0, duplicateACK = 1;
int timeOut = 200;

//��ʱλ
int timevalid = 1,recvSig = 2;

void send_pack() {
    while (!breakWile && sendable) {
        receiveable = 0;
        cout << "------------send-----------" << endl;
        //չʾ��ǰ����
        window->display();
        if (receive_id < window->start->head.id - 1) {
            perror("send:::ERROR");
            return;
        }
        sendPack = window->start;
        int sendID;
        //����send���ݰ�
        cout << window->start->head.id << " ";
        for (int i = 0 ; sendPack && i < window->cwnd; sendPack = sendPack->next) {
            if (sendPack->head.buf_size > 0)
            {
                sendID = sendPack->head.id;
                sendPack->getCheckSum();

                if (sendto(server_fd, (char*)sendPack, sizeof(*sendPack), 0, (struct sockaddr*)&client_addr, client_addr_length) < 0)
                {
                    perror("send:::Send File Failed:");
                    breakWile = 1;
                }
            }
            else
            {
                //�����ˣ����ǽ�����־
                sendID = sendPack->head.id;
                sendPack->head.id = 0;
                //data.head.buf_size = 0;
                sendto(server_fd, (char*)sendPack, sizeof(*sendPack), 0, (struct sockaddr*)&client_addr, client_addr_length);
                //cout << " END";
                breakWile = 1;
                break;
            }
        }
        cout << sendID << " " << endl;
        receiveable = 1;
        sendable = 0;
    }
}

void receive_pack() {
    int length = 0;
    while (!breakWile && receiveable) {
        sendable = 0;
        cout << "----------receive----------" << endl;
        for (int i = 0; i < window->cwnd; i++) {
            //����client��Ϣ
            recvSig = recvfrom(server_fd, (char*)&pack_info, sizeof(pack_info), 0, (struct sockaddr*)&client_addr, &client_addr_length);
            
            if (recvSig <= 0) {
                cout << ">>>��ʱ<<<" << endl;
                timevalid = 0;
                posSign = 2;
                break;
            }
            receive_id = pack_info.id;
            if (ack == receive_id) {
                if (ack == 1) {
                    window->confirm(1);
                }
                duplicateACK++;
            }
            else {
                ack = receive_id;
                window->confirm(receive_id);
            }
            if (duplicateACK >= 3) {
                posSign = 2;
                break;
            }
        }
        cout << receive_id << " \n";

        if (receive_id < window->start->head.id - 1) {
            perror("receive:::ERROR");
        }

        cout << "----------refresh----------" << endl;
        // ����Ƿ���Ҫ��������
        int lastPack = window->end->head.id;
        window->refresh();
        // ������
        if (posSign == 1) {
            window->cwnd *= 2;
            if (window->cwnd >= window->threshold) {
                posSign = 2;
            }
        }
        // ӵ�����⣬�������ӻ�ص�������
        if (posSign == 2) {
            window->cwnd++;
            window->threshold = window->cwnd / 2;
            // timeout
            if (!timevalid) {
                timevalid = 1;
                window->cwnd = 1;
                posSign = 1;
            }
            // tripple duplicate ACK
            if (duplicateACK == 3) {
                window->cwnd = window->threshold + 3;
                duplicateACK = 1;
                ack = 0;
            }    
        }
        cout << "cwnd : " << window->cwnd;
        
        if (window->currentBags == 0) {
            window->start = new Pack(lastPack + 1);
            length = fread(window->start->buf, sizeof(char), BUFF_LEN, fp);
            window->start->head.buf_size = length;
            window->end = window->start;
            window->currentBags++;
        }
        cout << " currentBags:" << window->currentBags;
        cout << " posSign: " << posSign << endl;
        if (length >= 0) {
            for (; length > 0 && window->cwnd > window->currentBags; window->currentBags++) {
                Pack* newPack = new Pack(window->end->head.id + 1);
                length = fread(newPack->buf, sizeof(char), BUFF_LEN, fp);
                newPack->head.buf_size = length;
                window->end->next = newPack;
                window->end = newPack;
                if (length < 0) {
                    window->cwnd = window->currentBags;
                    break;
                }
            }
        }
        
        window->display();
        cout << "---------------------------" << endl;
        cout << "---------------------------" << endl;

        receiveable = 0;
        sendable = 1;
    }
}


void transfer_file() {
    /* ÿ��ȡһ�����ݣ��㽫�䷢���ͻ��� */
    
    while (1)
    {
        int length = 0;
        /* �������� */
        char buffer[BUFF_LEN];
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
                Pack* newPack = new Pack(i + 1);
                length = fread(newPack->buf, sizeof(char), BUFF_LEN, fp);
                newPack->head.buf_size = length;
                window->end->next = newPack;
                window->end = newPack;
            }
            window->currentBags = window->cwnd;

            setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeOut, sizeof(struct timeval));
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