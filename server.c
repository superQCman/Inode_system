#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fileSystem.h"
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>

#define PORT 8080

struct ThreadArgs {
    int clientSocket;
    struct DirectoryBlock *root;
};


void *userFunction(void* arg) {
    struct ThreadArgs *args = (struct ThreadArgs *)arg;
    int clientSocket = args->clientSocket;
    struct DirectoryBlock *root = args->root;
    free(arg);

    // Initialization
    int i, flag = 0;
    for (i = 0; i < ENTRY_NUMBER; i++) {
        if (strcmp(root->fileName[i], "pwd") == 0) {
            flag = 1;
            break;
        }
    }

    char *username, *password, *permission;
    char current_path[256] = "/";
    char last_path[256] = "/";
    int login = 0;
    char file_permission_char[2];

    if (flag == 0) {
        char content[BLOCK_SIZE * MAX_BLCK_NUMBER_PER_FILE];
        struct Inode *pointer = inodeMem;
        struct DirectoryBlock *block;
        block = (struct DirectoryBlock *)&blockMem[pointer->blockID[0]];
        block->inodeID[0] = 0;
        strcpy(block->fileName[0], ".");
        block->inodeID[1] = 0;
        strcpy(block->fileName[1], "..");

        char *path = "/pwd";
        char password[256];
        
        // Request password from client
        sendMessage(clientSocket, "Input the administrator's password:\n");
        receiveInput(clientSocket, password, sizeof(password));

        // Continue as in your original code, using sendMessage and receiveInput to replace printf and fgets
        password[strcspn(password, "\n")] = '\0';
        char *admin = "admin-";
        strcpy(content, admin);
        strcat(content, password);
        strcat(content, "-2/");

        while (strlen(content) == 0) {
            sendMessage(clientSocket, "The content is empty!\nInput the administrator's password:\n");
            receiveInput(clientSocket, password, sizeof(password));
            password[strcspn(password, "\n")] = '\0';
            strcpy(content, admin);
            strcat(content, password);
            strcat(content, "-2/");
        }
        
        char* printWord = createFile(path, 22, content);
        sendMessage(clientSocket, printWord);
        username = "admin";
    } else {
        char input_username[256], input_password[256];
        
        // Authenticate user
        sendMessage(clientSocket, "username:");
        receiveInput(clientSocket, input_username, sizeof(input_username));
        sendMessage(clientSocket, "password:");
        receiveInput(clientSocket, input_password, sizeof(input_password));

        while (checkUser(input_username, input_password, current_path, last_path) == 0) {
            sendMessage(clientSocket, "用户名或密码错误，请重新输入！\nusername:");
            receiveInput(clientSocket, input_username, sizeof(input_username));

            sendMessage(clientSocket, "password:");
            receiveInput(clientSocket, input_password, sizeof(input_password));
        }
    }

    int running = 1;
    char command[1024] = {'\0'};
    char command_word[1024] = {'\0'};
    char *cmd, *path;

    // Send welcome message and available commands
    sendMessage(clientSocket, "\n##################################################\n"
                               "Welcome to the inode file system!\n"
                               "##################################################\n"
                               "The system supports following commands:\n"
                               "mkdir <path> - Create a directory\n"
                               "rmdir <path> - Delete a directory\n"
                               "ls <path> - List files\n"
                               "mkfile <path> - Create a file\n"
                               "rmfile <path> - Delete a file\n"
                               "read <path> - Read a file\n"
                               "write <path> - Write a file\n"
                               "cd <path> - Change directory\n"
                               "createUser - Create a new user\n"
                               "quit - Exit\n"
                               "Input your command:\n");
    while (running) {
        // Prompt user for command input
        snprintf(command_word, sizeof(command_word), "- %s: %s$ ", user.username, current_path);
        sendMessage(clientSocket, command_word);
        
        // Receive command from client
        receiveInput(clientSocket, command, sizeof(command));
        cmd = strtok(command, " ");
        path = strtok(NULL, "");

        if (cmd == NULL) {
            continue;
        }

        char *new_path, full_path[256]={'\0'};
        strcpy(full_path, current_path);
        if (path != NULL && strcmp(cmd, "su") != 0) {
            if (path[0] != '/'){
                if(path[0]!='.')strcat(full_path, path);
                else{
                    new_path = strtok(path, "/");
                    while (new_path != NULL) {
                        if (strcmp(new_path, "..") == 0) {
                            int len = strlen(full_path);
                            for (i = len - 2; i >= 0; i--) {
                                if (full_path[i] == '/') {
                                    full_path[i + 1] = '\0';
                                    break;
                                }
                            }
                        } else if (strcmp(new_path, ".") != 0) {
                            strcat(full_path, new_path);
                            strcat(full_path, "/");
                        }
                        new_path = strtok(NULL, "/");
                    }
                }
            }
            else{
                strcpy(full_path, path);
            }
        }


        if (strcmp(cmd, "mkdir") == 0) {
            char* printWord = createDirectory(full_path, user.permission);
            sendMessage(clientSocket, printWord);
        } else if (strcmp(cmd, "rmdir") == 0) {
            char* printWord = deleteDirectory(full_path, user.permission);
            sendMessage(clientSocket, printWord);
        } else if (strcmp(cmd, "ls") == 0) {
            listFiles(full_path, clientSocket);  // Assume listFiles outputs directly to client
        } else if (strcmp(cmd, "mkfile") == 0) {
            char content[BLOCK_SIZE * MAX_BLCK_NUMBER_PER_FILE];
            int permission_int = atoi(user.permission);
            sendMessage(clientSocket, "Input the content:\n");
            receiveInput(clientSocket, content, sizeof(content));

            sendMessage(clientSocket, "Input the file permission (文件权限11表示医生创建且只有医生可以访问，22表示管理员创建且只有管理员可以访问，21表示管理员创建且只有医生和管理员可以访问，10表示医生创建且只有患者和医生可以访问!):\n");
            receiveInput(clientSocket, file_permission_char, sizeof(file_permission_char));
            int file_permission = atoi(file_permission_char);
            if(permission_int == 0){
                sendMessage(clientSocket, "You are patient, you can't create a file!\n");
                continue;
            }
            if ((file_permission != 11 && file_permission != 22 && file_permission != 21 && file_permission != 10) || (file_permission/10 != permission_int)) {
                sendMessage(clientSocket, "Invalid file permission!\n");
            } else {
                char* printWord = createFile(full_path, file_permission, content);
                sendMessage(clientSocket, printWord);
            }
        } else if (strcmp(cmd, "rmfile") == 0) {
            char* printWord = deleteFile(full_path, user.permission);
            sendMessage(clientSocket, printWord);
        } else if (strcmp(cmd, "read") == 0) {
            char content[MAX_BLCK_NUMBER_PER_FILE * BLOCK_SIZE];
            char* printWord = readFile(full_path, content, user.permission);
            if (strlen(content) > 0) {
                char printWord[1024] = "File content: ";
                strcat(printWord, content);
                strcat(printWord, "\n");
                sendMessage(clientSocket, printWord);
            } else {
                sendMessage(clientSocket, printWord);
            }
        } else if (strcmp(cmd, "write") == 0) {
            char current_content[MAX_BLCK_NUMBER_PER_FILE * BLOCK_SIZE];
            char* readStatus = readFile(full_path, current_content, user.permission);
            if (strlen(current_content) > 0) {
                sendMessage(clientSocket, current_content);
            } else {
                sendMessage(clientSocket, readStatus);
            }
            
            sendMessage(clientSocket, "Input the new content:\n");
            receiveInput(clientSocket, current_content, sizeof(current_content));

            char* writeStatus = writeFile(full_path, current_content, user.permission);
            sendMessage(clientSocket, writeStatus);
        } else if (strcmp(cmd, "cd") == 0) {
            if (strcmp(path, "-") == 0) {
                char temp[256];
                strcpy(temp, last_path);
                char* printWord = moveDir(current_path, temp, last_path, user.permission);
                sendMessage(clientSocket, printWord);
            } else {
                char* printWord = moveDir(current_path, full_path, last_path, user.permission);
                sendMessage(clientSocket, printWord);
            }
        } else if (strcmp(cmd, "createUser") == 0) {
            if (strcmp(user.permission, "2") != 0) {
                sendMessage(clientSocket, "You don't have permission to create a new user!\n");
            } else {
                char content[MAX_BLCK_NUMBER_PER_FILE * BLOCK_SIZE];
                readFile("/pwd", content, "2");

                char newUserName[256], newPassword[256], permission[2];
                sendMessage(clientSocket, "Input the new username:");
                receiveInput(clientSocket, newUserName, sizeof(newUserName));

                sendMessage(clientSocket, "Input the new password:");
                receiveInput(clientSocket, newPassword, sizeof(newPassword));

                sendMessage(clientSocket, "Permission (0=patient, 1=doctor, 2=admin):");
                receiveInput(clientSocket, permission, sizeof(permission));

                strcat(content, newUserName);
                strcat(content, "-");
                strcat(content, newPassword);
                strcat(content, "-");
                strcat(content, permission);
                strcat(content, "/");

                char* writeStatus = writeFile("/pwd", content, user.permission);
                sendMessage(clientSocket, writeStatus);
            }
        } else if (strcmp(cmd, "su") == 0 && path != NULL) {
            sendMessage(clientSocket, "Input the password:\n");
            char password[256];
            receiveInput(clientSocket, password, sizeof(password));

            if (checkUser(path, password, current_path, last_path) == 1) {
                sendMessage(clientSocket, "User changed successfully!\n");
            } else {
                sendMessage(clientSocket, "Failed to change user!\n");
            }
        } else if (strcmp(cmd, "quit") == 0) {
            running = 0;
            char* loadWord = saveFileSystem();
            printf("%s\n", loadWord);
        }else{
            sendMessage(clientSocket, "Invalid command!\n");
        }
    }
}


int main() {
    memset(blockBitmap, 0, sizeof(blockBitmap));  // 初始化位图

    int i;

    for (i = 0; i < INODE_NUMBER; i++) {
        inodeMem[i].inodeNumber = i;
        for (int j = 0; j < MAX_BLCK_NUMBER_PER_FILE; j++) {
            inodeMem[i].blockID[j] = -1;
        }
        inodeMem[i].blockNum = 0;
    }
    inodeMem[0].blockID[0] = 0;
    inodeMem[0].fileType = 0;
    inodeMem[0].blockNum = 1;
    blockBitmap[0] |= 1;
    struct DirectoryBlock *root = (struct DirectoryBlock *) &blockMem[0];
    for (i = 0; i < ENTRY_NUMBER; i++) {
        root->inodeID[i] = -1;
    }
    char* loadWord = loadFileSystem();
    printf("%s\n", loadWord);

    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrSize = sizeof(struct sockaddr_in);

    // 创建服务器套接字
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        printf("Socket creation failed\n");
        return 1;
    }

    // 设置套接字选项，允许地址重用
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 设置服务器地址结构
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    // 绑定套接字到指定的IP和端口
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        printf("Bind failed\n");
        close(serverSocket);
        return 1;
    }

    // 开始监听传入的连接
    if (listen(serverSocket, SOMAXCONN) < 0) {
        printf("Listen failed\n");
        close(serverSocket);
        return 1;
    }

    printf("等待传入的连接...\n");

    // 接受传入的客户端连接并创建线程处理
    while (1) {
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &addrSize);
        if (clientSocket < 0) {
            printf("Accept failed\n");
            close(serverSocket);
            return 1;
        }

        printf("Accept success\n");
        struct ThreadArgs *args = malloc(sizeof(struct ThreadArgs));
        args->clientSocket = clientSocket;
        args->root = root;
        pthread_t clientThread;
        pthread_create(&clientThread, NULL, userFunction, (void *)args);
        pthread_detach(clientThread);
    }
    // 关闭服务器套接字
    close(serverSocket);
    return 0;
}