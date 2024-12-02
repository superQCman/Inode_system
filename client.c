#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT 8000
#define BUFFER_SIZE 1024

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    // 创建客户端套接字
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation error\n");
        return -1;
    }

    // 设置服务器地址和端口
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // 将服务器地址转换为二进制格式
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("Invalid address/ Address not supported\n");
        return -1;
    }

    // 连接到服务器
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connection Failed\n");
        return -1;
    }

    // 接收并显示服务器的欢迎消息
    // int valread;
    // valread = read(sock, buffer, BUFFER_SIZE);
    // printf("%s", buffer);
    // memset(buffer, 0, BUFFER_SIZE);

    // 主循环，等待用户输入并发送给服务器
    while (1) {
        // 接收信号，判断接下来是接收数据还是发送数据
        memset(buffer, 0, BUFFER_SIZE);
        int valread = recv(sock, buffer, BUFFER_SIZE,0);
        // printf("Signal received: %s\n", buffer);
        if (strcmp(buffer, "OUTPUT") == 0) {
            send(sock, "0", 2, 0);
            memset(buffer, 0, BUFFER_SIZE);
            valread = recv(sock, buffer, BUFFER_SIZE,0);
            send(sock, "0", 2, 0);
            printf("%s", buffer);
        } else if (strcmp(buffer, "INPUT") == 0) {
            memset(buffer, 0, BUFFER_SIZE);
            fgets(buffer, BUFFER_SIZE, stdin);
            // buffer[strcspn(buffer, "\n")] = 0; // 去掉换行符

            // 发送用户输入到服务器
            send(sock, buffer, strlen(buffer), 0);

            // 如果用户输入"quit"，则退出程序
            if (strcmp(buffer, "quit\n") == 0) {
                break;
            }
        }else{
            printf("Invalid signal received: %s\n", buffer);
        }
        memset(buffer, 0, BUFFER_SIZE);
    }

    // 关闭套接字
    close(sock);
    return 0;
}


