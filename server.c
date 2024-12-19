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
#include <pthread.h>

#define PORT 8000

struct ThreadArgs {
    int clientSocket;
    struct DirectoryBlock *root;
};


void *userFunction(void* arg) {
    struct User user;  // 用户数组，存储所有用户的信息
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
    char file_permission_char[64];

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
        strcpy(user.username, "admin");
        strcpy(user.password, password);
        strcpy(user.permission, "2");
        int file_permission = 22;
        while (strlen(content) == 0) {
            sendMessage(clientSocket, "The content is empty!\nInput the administrator's password:\n");
            receiveInput(clientSocket, password, sizeof(password));
            password[strcspn(password, "\n")] = '\0';
            strcpy(content, admin);
            strcat(content, password);
            strcat(content, "-2/");
        }
        char path_tmp[512];
        strcpy(path_tmp, path);
        char* printWord = createFile(path, file_permission, content);
        if(strcmp(printWord, "File created successfully!\n")==0){
            strcpy(fileLock[0].filePath, path_tmp);
            fileLock[0].lockNum = 1;
        }
        sendMessage(clientSocket, printWord);
        username = "admin";
    } else {
        char input_username[256], input_password[256];
        
        // Authenticate user
        sendMessage(clientSocket, "username:");
        receiveInput(clientSocket, input_username, sizeof(input_username));
        sendMessage(clientSocket, "password:");
        receiveInput(clientSocket, input_password, sizeof(input_password));

        while (checkUser(input_username, input_password, current_path, last_path, &user) == 0) {
            sendMessage(clientSocket, "Invalid username or password, please try again!\nusername:");
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
                               "createUser <username> - Create a new user\n"
                               "su <username> - Change user\n"
                               "ln <path> - Link a file\n"
                               "delUser <username> - Delete a user\n"
                               "quit - Exit\n"
                               "cpfile <srcPath> - Copy a file\n"
                               "renameFile <oldPath> - Rename a file\n"
                               "renameDirectory <oldPath> - Rename a directory\n"
                               "Input your command:\n");
    while (running) {
        // saveFileSystem();
        // Prompt user for command input
        snprintf(command_word, sizeof(command_word), "- %s: %s$ ", user.username, current_path);
        sendMessage(clientSocket, command_word);
        
        // Receive command from client
        receiveInput(clientSocket, command, sizeof(command));
        // loadFileSystem();
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
        char *space = strchr(full_path, ' ');
        if (space != NULL) {
            *space = '\0';
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

            sendMessage(clientSocket, "Input the file permission (File permission 11n means created by doctor and only doctors of group n (n == 0 means all groups) can access, 22 means created by admin and only admin can access, 21n means created by admin and only doctors of group n and admin can access, 13n means created by doctor and only patients and doctors of group n can access):\n");
            receiveInput(clientSocket, file_permission_char, sizeof(file_permission_char));
            int file_permission = atoi(file_permission_char);
            if(permission_int/10 == 3){
                sendMessage(clientSocket, "You are patient, you can't create a file!\n");
                continue;
            }
            if (file_permission/10 != 11 && file_permission != 22 && file_permission/10 != 21 && file_permission/10 != 13) {
                sendMessage(clientSocket, "Invalid file permission!\n");
                printf("file_permission: %d\n",file_permission);
                continue;
            }else if((file_permission/100 != permission_int/10 && permission_int != 2) || (file_permission%10 != permission_int%10 && permission_int != 2) ){
                sendMessage(clientSocket, "Invalid file permission!\n");
                printf("file_permission: %d\n",file_permission);
                printf("permission_int: %d\n",permission_int);
                continue;
            } else {
                char path_tmp[1024];
                strcpy(path_tmp, full_path);
                char* printWord = createFile(full_path, file_permission, content);
                if(strcmp(printWord, "File created successfully!\n")==0){
                    for(i = 0; i < MAX_FILE; i++){
                        if(fileLock[i].lockNum == 0){
                            strcpy(fileLock[i].filePath, path_tmp);
                            fileLock[i].lockNum = 1;
                            break;
                        }
                    }
                    
                }
                sendMessage(clientSocket, printWord);
            }
        } else if (strcmp(cmd, "rmfile") == 0) {
            int lockNum = -1;
            for (i = 0; i < MAX_FILE; i++) {
                if (fileLock[i].lockNum && strcmp(fileLock[i].filePath, full_path) == 0) {
                    lockNum = i;
                    break;
                }
            }
            if(lockNum == -1){
                sendMessage(clientSocket, "The fileLock is not exist!\n");
                continue;
            }
            if (pthread_mutex_trylock(&fileLock[lockNum].lock) != 0) {
                sendMessage(clientSocket, "The file is being modified by other user, please try again later!\n");
                continue;
            }
            char* printWord = deleteFile(full_path, user.permission);
            if(strcmp(printWord, "File deleted successfully!\n")==0){
                fileLock[lockNum].lockNum = 0;
            }
            pthread_mutex_unlock(&fileLock[lockNum].lock);
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
            int lockNum = -1;
            for (i = 0; i < MAX_FILE; i++) {
                if (fileLock[i].lockNum && strcmp(fileLock[i].filePath, full_path) == 0) {
                    lockNum = i;
                    break;
                }
            }
            if(lockNum == -1){
                sendMessage(clientSocket, "The fileLock is not exist!\n");
                continue;
            }
            if (pthread_mutex_trylock(&fileLock[lockNum].lock) != 0) {
                sendMessage(clientSocket, "The file is being modified by other user, please try again later!\n");
                continue;
            }
            char current_content[MAX_BLCK_NUMBER_PER_FILE * BLOCK_SIZE];
            int permission_int = atoi(user.permission);
            if(permission_int/10 == 3 && permission_int != 2){
                sendMessage(clientSocket, "You are patient, you can't write a file!\n");
                continue;
            }
            char temp_path[1024];
            strcpy(temp_path, full_path);
            char* readStatus = readFile(full_path, current_content, user.permission);
            strcpy(full_path, temp_path);
            if (strcmp(readStatus, "File content: ") == 0) {
                sendMessage(clientSocket, readStatus);
                sendMessage(clientSocket, current_content);
            } else {
                sendMessage(clientSocket, readStatus);
                pthread_mutex_unlock(&fileLock[lockNum].lock);
                continue;
            }
            
            sendMessage(clientSocket, "\nInput the new content:\n");
            receiveInput(clientSocket, current_content, sizeof(current_content));

            char* writeStatus = writeFile(full_path, current_content, user.permission, 0);
            sendMessage(clientSocket, writeStatus);
            pthread_mutex_unlock(&fileLock[lockNum].lock);
        } else if (strcmp(cmd, "cd") == 0) {
            if (path == NULL){
                sendMessage(clientSocket, "Invalid command!\n");
                continue;
            }
            if (strcmp(path, "-") == 0) {
                char temp[256];
                strcpy(temp, last_path);
                char* printWord = moveDir(current_path, temp, last_path, user.permission);
                sendMessage(clientSocket, printWord);
            } else {
                char* printWord = moveDir(current_path, full_path, last_path, user.permission);
                sendMessage(clientSocket, printWord);
            }
        } else if (strcmp(cmd, "createUser") == 0 && path != NULL) {
            if (strcmp(user.permission, "2") != 0) {
                sendMessage(clientSocket, "You don't have permission to create a new user!\n");
            } else {
                char content[MAX_BLCK_NUMBER_PER_FILE * BLOCK_SIZE];
                readFile("/pwd", content, "2");

                char newUserName[256], newPassword[256], userName[256]="admin";
                strcpy(newUserName, path);
                sendMessage(clientSocket, "Input the new password:");
                receiveInput(clientSocket, newPassword, sizeof(newPassword));
                char permission[3]="";
                while(checkUserName(userName) == 1){
                    strcpy(permission, "");
                    strcpy(userName, newUserName);
                    char position[2];
                    sendMessage(clientSocket, "If you are patient, enter 3, if you are doctor, enter 1, if you are administrators, enter 2\n");
                    receiveInput(clientSocket, position, sizeof(position));
                    printf("position: %s\n", position);
                    if(position[0] != '1' && position[0] != '3' && position[0] != '2'){
                        sendMessage(clientSocket, "position input error!\n");
                        continue;
                    }

                    strcat(permission, position);
                    if(position[0] != '2'){
                        char DoctorGroup[1024];
                        listGroup(DoctorGroup);
                        // printf("DoctorGroup: %s\n", DoctorGroup);
                        char message[1024];
                        sprintf(message, "DoctorGroup: %s\n", DoctorGroup);
                        sendMessage(clientSocket, message);

                        char group[2];
                        int c;
                        // while ((c = getchar()) != '\n' && c != EOF); // clear input buffer
                        sendMessage(clientSocket, "Input the group you want to join:\n");
                        receiveInput(clientSocket, group, sizeof(group));

                        strcat(permission, group);
                    }
                    strcat(userName, "_");
                    strcat(userName, permission);
                    if(checkUserName(userName) == 1){
                        sendMessage(clientSocket, "The username already exists, please input another username:");
                        receiveInput(clientSocket, newUserName, sizeof(newUserName));
                    }else{
                        strcpy(newUserName, userName);
                    }
                }
                // while(checkUserName(userName) == 1){
                //     sendMessage(clientSocket, "The username already exists, please input another username:");
                //     receiveInput(clientSocket, newUserName, sizeof(newUserName));
                //     strcpy(userName, newUserName);
                // }
                
                int permission_int = atoi(permission);
                char doctor_name[1024];
                if(permission_int/10 == 3){
                    checkDoctor(permission_int, doctor_name);
                    if(strcmp(doctor_name, "admin") == 0){
                        // printf("There is no doctor in this group!\n");
                        sendMessage(clientSocket, "There is no doctor in this group!\n");
                        continue;
                    }
                }
                char patient_name[1024];
                int updata = 0;
                if(permission_int/10 == 1){
                    checkPatient(permission_int,patient_name);
                    if(strcmp(patient_name,"admin") != 0){
                        updata = 1;
                    }
                }
                char delimiter_1[2] = "-";
                char delimiter_2[2] = "/";
                strcat(newUserName, delimiter_1);
                strcat(newUserName, newPassword);
                strcat(newUserName, delimiter_1);
                strcat(newUserName, permission);
                strcat(newUserName, delimiter_2);

                strcat(content, newUserName);
                printf("root 1:\n");
                listFiles_main("/");
                char* writeStatus = writeFile("/pwd", content, user.permission, 0);
                sendMessage(clientSocket, writeStatus);
                if(permission_int/10 == 1 && updata == 1){
                    char delimiter_0[2] = " ";
                    char *saveptr1;
                    // char doctor_path[1024] = "/home/";
                    // strcat(doctor_path, userName);
                    char *patient = strtok_r(patient_name, delimiter_0, &saveptr1);
                    while(patient != NULL){
                        char full_path[1024] = "/home/";
                        strcat(full_path, userName);
                        strcat(full_path, "/");
                        strcat(full_path, patient);
                        char doctor_path[1024];
                        strcpy(doctor_path,full_path);
                        char patient_path[1024] = "/home/";
                        strcat(patient_path, patient);
                        char* printWord = createDirectory(full_path, user.permission);
                        // printf("%s\n", printWord);
                        sendMessage(clientSocket, printWord);
                        printf("test root 7:\n");
                        listFiles_main("/");
                        printWord = linkPath( doctor_path, patient_path, user.permission);
                        printf("test root 8:\n");
                        listFiles_main("/");
                        // printf("%s\n", printWord);
                        sendMessage(clientSocket, printWord);
                        patient = strtok_r(NULL, delimiter_0, &saveptr1);
                    }
                }
                if(permission_int/10 == 3){
                    char delimiter_0[2] = " ";
                    char *saveptr1;
                    char *doctor = strtok_r(doctor_name, delimiter_0, &saveptr1);
                    while(doctor != NULL){
                        char patient_path[1024] = "/home/";
                        strcat(patient_path, userName);
                        char full_path[1024] = "/home/";
                        strcat(full_path, doctor);
                        strcat(full_path, "/");
                        strcat(full_path, userName);
                        char doctor_path[1024];
                        strcpy(doctor_path,full_path);
                        char* printWord = createDirectory(full_path, user.permission);
                        // printf("%s\n", printWord);
                        sendMessage(clientSocket, printWord);
                        printWord = linkPath( doctor_path, patient_path, user.permission);
                        // printf("%s\n", printWord);
                        sendMessage(clientSocket, printWord);
                        doctor = strtok_r(NULL, delimiter_0, &saveptr1);
                    }
                }
            }
        } else if(strcmp(cmd, "delUser")==0 && path != NULL){
            if (strcmp(user.permission, "2") != 0) {
                sendMessage(clientSocket, "You don't have permission to delete a new user!\n");
            }else{
                char content[MAX_BLCK_NUMBER_PER_FILE * BLOCK_SIZE];
                readFile("/pwd", content, "2");

                char delUserName[256], newPassword[256], userName[256]="admin";
                strcpy(delUserName, path);
                char home[256] = "/home/";
                strcat(home, delUserName);
                strcpy(userName, delUserName);
                char* saveptr0;
                char *P= strtok_r(delUserName, "_",&saveptr0);
                char permission[256];
                while(P != NULL){
                    strcpy(permission, P);
                    P = strtok_r(NULL, "_",&saveptr0);
                }
                // permission = strtok_r(NULL, "_",&saveptr0);
                if(permission == NULL){
                    sendMessage(clientSocket, "The user does not exist!\n");
                    continue;
                }
                int permission_int = atoi(permission);
                if(permission_int/10 == 3){
                    char doctor_name[1024];
                    checkDoctor(permission_int, doctor_name);
                    if(strcmp(doctor_name, "admin") != 0){
                        char delimiter_0[2] = " ";
                        char *saveptr1;

                        char *doctor = strtok_r(doctor_name, delimiter_0, &saveptr1);
                        while(doctor != NULL){
                            char Full_path[1024] = "/home/";
                            strcat(Full_path, doctor);
                            strcat(Full_path, "/");
                            strcat(Full_path, userName);
                            char* printWord = deleteDirectory(Full_path, user.permission);
                            // printf("%s\n", printWord);
                            sendMessage(clientSocket, printWord);
                            doctor = strtok_r(NULL, delimiter_0, &saveptr1);
                        }
                    }
                }
                // char* printWord = delUser(userName);
                // sendMessage(clientSocket, printWord);
                char* delword = deleteDirectory(home, user.permission);
                listFiles_main("/home");
                if(strcmp(delword, "Directory deleted successfully!\n")==0){
                    char* printWord = delUser(userName);
                    sendMessage(clientSocket, printWord);
                }else{
                    sendMessage(clientSocket, "The user does not exist!\n");
                }
                // listFiles_main("/home");
            }
        }else if (strcmp(cmd, "su") == 0 && path != NULL) {
            sendMessage(clientSocket, "Input the password:\n");
            char password[256];
            receiveInput(clientSocket, password, sizeof(password));

            if (checkUser(path, password, current_path, last_path, &user) == 1) {
                sendMessage(clientSocket, "User changed successfully!\n");
            } else {
                sendMessage(clientSocket, "Failed to change user!\n");
            }
        } else if (strcmp(cmd, "ln") == 0 && strcmp(path, "")!=0) {
            sendMessage(clientSocket, "Input the path of the file you want to link:\n");
            char link_path[256];
            receiveInput(clientSocket, link_path, sizeof(link_path));
            char* printWord = linkPath(full_path, link_path, user.permission);
            sendMessage(clientSocket, printWord);
        }else if (strcmp(cmd, "group") == 0) {
            char DoctorGroup[1024];
            listGroup(DoctorGroup);
            // printf("DoctorGroup: %s\n", DoctorGroup);
            char message[1024];
            sprintf(message, "DoctorGroup: %s\n", DoctorGroup);
            sendMessage(clientSocket, message);
        }else if (strcmp(cmd, "quit") == 0) {
            running = 0;
            char* loadWord = saveFileSystem();
            printf("%s\n", loadWord);
        }else if (strcmp(cmd, "renameFile") == 0 && path != NULL) {
            // 先加锁
            int lockNum = -1;
            for (i = 0; i < MAX_FILE; i++) {
                if (fileLock[i].lockNum && strcmp(fileLock[i].filePath, full_path) == 0) {
                    lockNum = i;
                    break;
                }
            }
            if(lockNum == -1){
                sendMessage(clientSocket, "The fileLock is not exist!\n");
                continue;
            }
            if (pthread_mutex_trylock(&fileLock[lockNum].lock) != 0) {
                sendMessage(clientSocket, "The file is being modified by other user, please try again later!\n");
                continue;
            }
            // 让用户输入新的文件名
            sendMessage(clientSocket, "Input the new file name:\n");
            char newName[256];
            receiveInput(clientSocket, newName, sizeof(newName));
            // 存放旧的文件名
            char oldName[256];
            strcpy(oldName, full_path);
            char* printWord = renameFile(full_path, newName, user.permission);
            strcpy(full_path, oldName);
            // 如果修改成功，修改文件锁的文件名
            if(strcmp(printWord, "File renamed successfully!\n") == 0){
                char newFullPath[1024];
                strcpy(newFullPath, full_path);
                char *lastSlash = strrchr(newFullPath, '/');
                if (lastSlash != NULL) {
                    *(lastSlash + 1) = '\0'; // 保留最后一个斜杠
                }
                strcat(newFullPath, newName);
                strcpy(fileLock[lockNum].filePath, newFullPath);
            }
            sendMessage(clientSocket, printWord);
            pthread_mutex_unlock(&fileLock[lockNum].lock);
        }else if (strcmp(cmd, "renameDirectory") == 0 && path != NULL) {
            // 让用户输入新的文件名
            sendMessage(clientSocket, "Input the new directory name:\n");
            char newName[256];
            receiveInput(clientSocket, newName, sizeof(newName));
            char* printWord = renameDirectory(full_path, newName, user.permission);
            sendMessage(clientSocket, printWord);
        }else if (strcmp(cmd, "cpfile") == 0 && path != NULL) {
            // 先加锁
            int lockNum = -1;
            for (i = 0; i < MAX_FILE; i++) {
                if (fileLock[i].lockNum && strcmp(fileLock[i].filePath, full_path) == 0) {
                    lockNum = i;
                    break;
                }
            }
            if(lockNum == -1){
                sendMessage(clientSocket, "The fileLock is not exist!\n");
                continue;
            }
            if (pthread_mutex_trylock(&fileLock[lockNum].lock) != 0) {
                sendMessage(clientSocket, "The file is being modified by other user, please try again later!\n");
                continue;
            }
            // 让用户输入要复制到的路径
            sendMessage(clientSocket, "Input the path of the file you want to copy:\n");
            char copy_path[256];
            receiveInput(clientSocket, copy_path, sizeof(copy_path));
            char copy_path_tmp[256];
            strcpy(copy_path_tmp, copy_path);
            char* printWord = copyFile(full_path, copy_path_tmp, user.permission, clientSocket);
            // 如果复制成功，添加文件锁
            if(strcmp(printWord, "File created successfully!\n")==0){
                for(i = 0; i < MAX_FILE; i++){
                    if(fileLock[i].lockNum == 0){
                        strcpy(fileLock[i].filePath, copy_path);
                        fileLock[i].lockNum = 1;
                        break;
                    }
                }
            }
            sendMessage(clientSocket, printWord);
            pthread_mutex_unlock(&fileLock[lockNum].lock);
        }else if (strcmp(cmd, "mvfile") == 0 && path != NULL) {
            // 先加锁
            int lockNum = -1;
            for (i = 0; i < MAX_FILE; i++) {
                if (fileLock[i].lockNum && strcmp(fileLock[i].filePath, full_path) == 0) {
                    lockNum = i;
                    break;
                }
            }
            if(lockNum == -1){
                sendMessage(clientSocket, "The fileLock is not exist!\n");
                continue;
            }
            if (pthread_mutex_trylock(&fileLock[lockNum].lock) != 0) {
                sendMessage(clientSocket, "The file is being modified by other user, please try again later!\n");
                continue;
            }
            sendMessage(clientSocket, "Input the path of the file you want to move:\n");
            char move_path[256];
            receiveInput(clientSocket, move_path, sizeof(move_path));
            char move_path_tmp[256];
            strcpy(move_path_tmp,move_path);
            char* printWord = moveFile(full_path, move_path_tmp, user.permission);
            // 如果复制成功，添加文件锁
            if(strcmp(printWord, "File created successfully!\n")==0){
                fileLock[lockNum].lockNum = 0;
                for(i = 0; i < MAX_FILE; i++){
                    if(fileLock[i].lockNum == 0){
                        strcpy(fileLock[i].filePath, move_path);
                        fileLock[i].lockNum = 1;
                        break;
                    }
                }
            }
            sendMessage(clientSocket, printWord);
            pthread_mutex_unlock(&fileLock[lockNum].lock);
        }
        else if (strcmp(cmd, "fp") == 0 && path != NULL) {
            char* permission = findFilePermission(full_path);
            sendMessage(clientSocket, permission);
        }
        else{
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
        inodeMem[i].fileType = -1;
        inodeMem[i].permission = -1;
        inodeMem[i].fileNum = 0;
    }
    inodeMem[0].blockID[0] = 0;
    inodeMem[0].fileType = 0;
    inodeMem[0].blockNum = 1;
    inodeMem[0].permission = 2;
    inodeMem[0].fileNum = 1;
    blockBitmap[0] |= 1;
    struct DirectoryBlock *root = (struct DirectoryBlock *) &blockMem[0];
    for (i = 0; i < ENTRY_NUMBER; i++) {
        root->inodeID[i] = -1;
    }

    for(i = 0; i < MAX_FILE; i++){
        pthread_mutex_init(&fileLock[i].lock, NULL);
        fileLock[i].filePath[0] = '\0';
        fileLock[i].lockNum = 0;
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