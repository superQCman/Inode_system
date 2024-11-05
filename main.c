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
    char *username, *password, *permission;
    char current_path[256] = "/";
    char last_path[256] = "/";
    int login = 0;
    if (flag == 0){
        struct Inode *pointer = inodeMem; // 指向当前目录的i节点
        struct DirectoryBlock *block;
        block = (struct DirectoryBlock *) &blockMem[pointer->blockID[0]];
        block->inodeID[0] = 0;
        strcpy(block->fileName[0], ".");
        block->inodeID[1] = 0;
        strcpy(block->fileName[1], "..");
        createFile("/pwd", "2");
        username = "admin";

    }else{
        
        // char *saveptr1, *saveptr2;
        // char *USER = strtok_r(content, "/", &saveptr1);
        char input_username[256], input_password[256];
        printf("username:");   
        fgets(input_username, sizeof(input_username), stdin);
        input_username[strcspn(input_username, "\n")] = '\0';
        
        printf("password:");
        fgets(input_password, sizeof(input_password), stdin);
        input_password[strcspn(input_password, "\n")] = '\0';
        while(checkUser(input_username, input_password, current_path, last_path) == 0){
            printf("用户名或密码错误，请重新输入！\n");
            printf("username:");   
            fgets(input_username, sizeof(input_username), stdin);
            input_username[strcspn(input_username, "\n")] = '\0';
            
            printf("password:");
            fgets(input_password, sizeof(input_password), stdin);
            input_password[strcspn(input_password, "\n")] = '\0';
            
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
        printf("- %s: %s$ ", user.username,current_path);
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = '\0';  // 去掉行进符

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

        if (strcmp(cmd, "mkdir") == 0 ) {
            createDirectory(full_path, user.permission);
        } else if (strcmp(cmd, "rmdir") == 0) {
            deleteDirectory(full_path, user.permission);
        } else if (strcmp(cmd, "ls") == 0 ) {
            listFiles(full_path); 
        } else if (strcmp(cmd, "mkfile") == 0 ) {
            createFile(full_path, user.permission);
        } else if (strcmp(cmd, "rmfile") == 0 ) {
            deleteFile(full_path,user.permission); 
        } else if (strcmp(cmd, "read") == 0 ) {
            char content[MAX_BLCK_NUMBER_PER_FILE*BLOCK_SIZE];
            readFile(full_path,content,user.permission); 
            if(strcmp(content,"")!=0)printf("File content: %s\n", content);
        } else if (strcmp(cmd, "write") == 0 ) {
            char read_content[MAX_BLCK_NUMBER_PER_FILE*BLOCK_SIZE];
            char content[BLOCK_SIZE * MAX_BLCK_NUMBER_PER_FILE];
            char temp_path[256];
            strcpy(temp_path, full_path);
            readFile(temp_path, read_content, user.permission);
            printf("File content: %s\n", read_content);
            printf("Input the new content:\n");
            fgets(content, sizeof(content), stdin);
            content[strcspn(content, "\n")] = '\0';
            writeFile(full_path, content, user.permission); 
        } else if (strcmp(cmd, "cd") == 0) {
            if (strcmp(path, "-") == 0) {
                char temp[256];
                strcpy(temp, last_path);
                moveDir(current_path, temp, last_path, user.permission);
            }else moveDir(current_path, full_path, last_path, user.permission);
        } else if (strcmp(cmd, "createUser") == 0 ) {
            if (strcmp(user.permission, "2") != 0){
                printf("You don't have permission to create a new user!\n");
                continue;
            }
            char content[MAX_BLCK_NUMBER_PER_FILE*BLOCK_SIZE];
            readFile("/pwd",content,"2");
            char newUserName[256], newPassword[256];
            printf("Input the new username:");
            fgets(newUserName, sizeof(newUserName), stdin);
            newUserName[strcspn(newUserName, "\n")] = '\0';

            printf("Input the new password:");
            fgets(newPassword, sizeof(newPassword), stdin);
            newPassword[strcspn(newPassword, "\n")] = '\0';

            printf("permission: \nIf you are patient, please input 0, if you are doctor, please input 1, if you are administrators, please input 2\n");
            char permission[2];
            fgets(permission, sizeof(permission), stdin);
            permission[strcspn(permission, "\n")] = '\0';

            char delimiter_1[2] = "-";
            char delimiter_2[2] = "/";
            strcat(newUserName, delimiter_1);
            strcat(newUserName, newPassword);
            strcat(newUserName, delimiter_1);
            strcat(newUserName, permission);
            strcat(newUserName, delimiter_2);

            strcat(content, newUserName);
            printf("content: %s\n", content);
            writeFile("/pwd", content, user.permission);

        } else if (strcmp(cmd, "su") == 0  && strcmp(path, "")!=0) {
            printf("Input the password:\n");
            char password[256];
            fgets(password, sizeof(password), stdin);
            password[strcspn(password, "\n")] = '\0';
            if(checkUser(path, password, current_path, last_path) == 1){
                printf("change user successfully!\n");
            }else{
                printf("change user failed!\n");
            }
        } else if (strcmp(cmd, "quit") == 0) {
            running = 0;
        } else {
            printf("Please input a valid command\n");
        }
    }
    saveFileSystem();
    return 0;
}
