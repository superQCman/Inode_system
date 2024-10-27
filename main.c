#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fileSystem.h"

int main() {
    // 初始化
    int i;
    memset(blockBitmap, 0, sizeof(blockBitmap));  // 初始化位图
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
    loadFileSystem();
    int flag = 0;
    for (i = 0; i < ENTRY_NUMBER; i++) {
        if(strcmp(root->fileName[i], "pwd") == 0){
            flag = 1;
            break;
        }
    }
    char *username, *password;
    char current_path[256] = "/";
    char last_path[256] = "/";
    int login = 0;
    if (flag == 0){
        createFile("/pwd");
        username = "root";
    }else{
        char content[MAX_BLCK_NUMBER_PER_FILE*BLOCK_SIZE];
        readFile("/pwd",content);
        char *saveptr1, *saveptr2;
        char *USER = strtok_r(content, "/", &saveptr1);
        char input_username[256], input_password[256];
        printf("username:");   
        fgets(input_username, sizeof(input_username), stdin);
        input_username[strcspn(input_username, "\n")] = '\0';
        
        printf("password:");
        fgets(input_password, sizeof(input_password), stdin);
        input_password[strcspn(input_password, "\n")] = '\0';
        
        while(USER != NULL && login == 0){
            username = strtok_r(USER, "-", &saveptr2);
            password = strtok_r(NULL, "-", &saveptr2);
            // printf("username: %s\npassword: %s\n", username, password);
            
            if(strcmp(username, input_username) == 0 && strcmp(password, input_password) == 0){
                login = 1;
                // printf("Login successfully!\n");
            }
            USER = strtok_r(NULL, "/", &saveptr1);
        }
        if (login == 0){
            printf("Login failed!\n");
            return 0;
        }else{
            char full_path[256] = "/home/";
            if(strcmp(username, "root") != 0){
                strcat(full_path, username);
                moveDir(current_path, full_path, last_path);
            }
            printf("Login successfully!\n");
        }
    }
    int running = 1;
    char command[256] = {'\0'};
    char *cmd, *path;
    

    printf("\n##################################################\n"
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
    while (running == 1) {
        printf("%s$ ", current_path);
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = '\0';  // 去掉行进符

        cmd = strtok(command, " ");
        path = strtok(NULL, "");

        if (cmd == NULL) {
            continue;
        }

        char *new_path, full_path[256]={'\0'};
        strcpy(full_path, current_path);
        if (path != NULL){
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

        if (strcmp(cmd, "mkdir") == 0 ) {
            createDirectory(full_path);
        } else if (strcmp(cmd, "rmdir") == 0) {
            deleteDirectory(full_path);
        } else if (strcmp(cmd, "ls") == 0 ) {
            listFiles(full_path);  // 暂时省略列出文件功能，可以依照前面的调整方法修改
        } else if (strcmp(cmd, "mkfile") == 0 ) {
            createFile(full_path);
        } else if (strcmp(cmd, "rmfile") == 0 ) {
            deleteFile(full_path);  // 暂时省略删除文件功能，可以依照前面的调整方法修改
        } else if (strcmp(cmd, "read") == 0 ) {
            char content[MAX_BLCK_NUMBER_PER_FILE*BLOCK_SIZE];
            readFile(full_path,content);  // 暂时省略读取文件功能，可以依照前面的调整方法修改
            printf("File content: %s\n", content);
        } else if (strcmp(cmd, "write") == 0 ) {
            char read_content[MAX_BLCK_NUMBER_PER_FILE*BLOCK_SIZE];
            char content[BLOCK_SIZE * MAX_BLCK_NUMBER_PER_FILE];
            char temp_path[256];
            strcpy(temp_path, full_path);
            readFile(temp_path, read_content);
            printf("File content: %s\n", read_content);
            printf("Input the new content:\n");
            fgets(content, sizeof(content), stdin);
            content[strcspn(content, "\n")] = '\0';
            writeFile(full_path, content); 
        } else if (strcmp(cmd, "cd") == 0) {
            if (strcmp(path, "-") == 0) {
                char temp[256];
                strcpy(temp, last_path);
                moveDir(current_path, temp, last_path);
            }else moveDir(current_path, full_path, last_path);
        } else if (strcmp(cmd, "createUser") == 0 ) {
            char content[MAX_BLCK_NUMBER_PER_FILE*BLOCK_SIZE];
            readFile("/pwd",content);
            char newUserName[256], newPassword[256];
            printf("Input the new username:");
            fgets(newUserName, sizeof(newUserName), stdin);
            newUserName[strcspn(newUserName, "\n")] = '\0';
            printf("Input the new password:");
            fgets(newPassword, sizeof(newPassword), stdin);
            newPassword[strcspn(newPassword, "\n")] = '\0';
            char delimiter_1[2] = "-";
            char delimiter_2[2] = "/";
            strcat(newUserName, delimiter_1);
            strcat(newUserName, newPassword);
            strcat(newUserName, delimiter_2);
            strcat(content, newUserName);
            printf("content: %s\n", content);
            writeFile("/pwd", content);

        } else if (strcmp(cmd, "quit") == 0) {
            running = 0;
        } else {
            printf("Please input a valid command\n");
        }
    }
    saveFileSystem();
    return 0;
}
