#include "header.h"

void recv_file(int client_fd, struct sockaddr_in server_addr) {
    socklen_t server_addr_length = sizeof(server_addr);
    Pack data;
    //ĿǰΪֹ�յ��İ����
    int currentId;

    /* �����ļ����������� */
    char source_file_name[FILE_NAME_LEN + 1], target_file_name[FILE_NAME_LEN + 1];
    memset(source_file_name, '\0', sizeof(source_file_name));
    cout << "����������ͻ����ļ�����";
    cin >> source_file_name;
    cout << "������Ŀ���û����ļ�����";
    cin >> target_file_name;
    cout << "-------------------------------------------------------------------" << endl;

    char buffer[BUFF_LEN];
    memset(buffer, '\0', sizeof(buffer));
    strncpy_s(buffer, source_file_name, strlen(source_file_name) > BUFF_LEN ? BUFF_LEN : strlen(source_file_name));

    /* �����ļ��� */
    if (sendto(client_fd, buffer, BUFF_LEN, 0, (struct sockaddr*)&server_addr, server_addr_length) < 0)
    {
        cout << "ERROR:\tSend File Name Failed" << endl;
        exit(1);
    }

    ofstream outfile(target_file_name, ios::binary | ios::app);
    if (!outfile.is_open())
    {
        cout << "ERROR:\tFile:\t" << target_file_name << " Can Not Open To Write" << endl;
    }

    /* �ӷ������������ݣ���д���ļ� */
    int len = 0;
    currentId = 0;
    while (1)
    {
        PackInfo pack_info;

        if ((len = recvfrom(client_fd, (char*)&data, sizeof(data), 0, (struct sockaddr*)&server_addr, &server_addr_length)) > 0)
        {
            if (data.head.id == 0) {
                //�ļ�������
                break;
            }
            if (data.head.id != currentId + 1) {
                //�յ��Ĳ���Ҫ���ӵ�������currentId
                cout << "-�յ��˱��Ϊ" << data.head.id << "�����ݰ�������Ҫ-" << endl;
                cout << data.head.id << " " << currentId << endl;
                pack_info.id = currentId;
                //pack_info.buf_size = data.head.buf_size;
            }
            else {
                //�յ�������Ҫ��
                if (data.head.checksum != SERVER_PORT + SERVER_IP_1 + SERVER_IP_2 + SERVER_IP_3 + SERVER_IP_4 + data.head.id + data.head.buf_size) {
                    //������
                    cout << "-�յ��˱��Ϊ" << data.head.id << "�����ݰ�����У�����𻵰�-" << endl;
                    pack_info.id = currentId;
                }
                else {
                    //������
                    cout << "-�յ��˱��Ϊ" << data.head.id << "�����ݰ�������-" << endl;
                    //cout << data.buf;
                    //���·�����Ϣ
                    pack_info.id = data.head.id;
                    pack_info.buf_size = data.head.buf_size;
                    currentId++;
                    //д�ļ�
                    for (int dataIndex = 0; dataIndex < data.head.buf_size; dataIndex++) {
                        outfile << data.buf[dataIndex];
                    }
                    cout << "-���Ϊ" << data.head.id << "�����ݰ���д���ļ�-" << endl;
                }
            }

            /* ACK */

            if (sendto(client_fd, (char*)&pack_info, sizeof(pack_info), 0, (struct sockaddr*)&server_addr, server_addr_length) < 0)
            {
                printf("Send confirm information failed!");
            }
            else {
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
    WSADATA WSAData;

    //��ʼ��
    if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0) {
        cout << "��ʼ��ʧ��!" << endl;
        return -1;
    }
    cout << ">>>��ʼ���ɹ�<<<" << endl;

    SOCKET client_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (client_socket == INVALID_SOCKET)
    {
        cout << "������socket����ʧ�ܣ�" << endl;
        return 0;
    }
    cout << ">>>�׽���������<<<" << endl;

    //��ȡ��������ַ
    SOCKADDR_IN server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);//�˿ں�Ϊ1207
    inet_pton(AF_INET, SERVER_IP, (void*)&server_addr.sin_addr.S_un.S_addr);

    //������
    recv_file(client_socket, server_addr);

    //��β
    closesocket(client_socket);
    WSACleanup();

    return 0;
}