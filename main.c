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
    char* loadWord = loadFileSystem();
    printf("%s\n", loadWord);
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
    char file_permission_char[2];
    
    if (flag == 0){
        char content[BLOCK_SIZE * MAX_BLCK_NUMBER_PER_FILE];
        struct Inode *pointer = inodeMem; // 指向当前目录的i节点
        struct DirectoryBlock *block;
        block = (struct DirectoryBlock *) &blockMem[pointer->blockID[0]];
        block->inodeID[0] = 0;
        strcpy(block->fileName[0], ".");
        block->inodeID[1] = 0;
        strcpy(block->fileName[1], "..");
        char *path = "/pwd";
        char password[256];
        printf("Input the administrator's password:\n");
        fgets(password, sizeof(password), stdin);
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
            printf("The content is empty!\n");
            printf("Input the administrator's password:\n");
            fgets(password, sizeof(password), stdin);
            password[strcspn(password, "\n")] = '\0';
            char *admin = "admin-";
            strcpy(content, admin);
            strcat(content, password);
            strcat(content, "-2/");
            strcpy(user.username, "admin");
            strcpy(user.password, password);
            strcpy(user.permission, "2");
        }

        char* printWord = createFile(path, file_permission, content);
        printf("%s\n", printWord);
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
    char command[1024] = {'\0'};
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
            char* printWord = createDirectory(full_path, user.permission);
            printf("%s\n", printWord);
        } else if (strcmp(cmd, "rmdir") == 0) {
            char* printWord = deleteDirectory(full_path, user.permission);
            printf("%s\n", printWord);
        } else if (strcmp(cmd, "ls") == 0 ) {
            listFiles_main(full_path); 
        } else if (strcmp(cmd, "mkfile") == 0 ) {
            char content[BLOCK_SIZE * MAX_BLCK_NUMBER_PER_FILE];
            char password[256];
            printf("Input the content:\n");
            fgets(content, sizeof(content), stdin);
            content[strcspn(content, "\n")] = '\0';  // 去掉行进符
            int permission_int = atoi(user.permission);

            char file_permission_char[64];
            printf("Input the file permsission(文件权限11n表示医生创建且只有第n组医生（n == 0代表所有组的医生都可以访问）可以访问，22表示管理员创建且只有管理员可以访问，21n表示管理员创建且只有第n组医生和管理员可以访问，13n表示医生创建且只有患者和医生可以访问（只有第n组的医生患者可以访问）!):\n");
            fgets(file_permission_char, sizeof(file_permission_char), stdin);
            file_permission_char[strcspn(file_permission_char, "\n")] = '\0';
            int file_permission = atoi(file_permission_char);
            if(file_permission/10 != 11 && file_permission != 22 && file_permission/10 != 21 && file_permission/10 != 13){
                printf("文件权限输入错误！\n");
                printf("file_permission: %d\n",file_permission);
                continue;
            }else if((file_permission/100 != permission_int/10 && permission_int != 2) || (file_permission%10 != permission_int%10 && permission_int != 2) ){
                printf("文件权限输入错误！\n");
                printf("file_permission: %d\n",file_permission);
                printf("permission_int: %d\n",permission_int);
                continue;
            }
            
            if(permission_int == 0){
                printf("You are patient, you can't create a file!\n");
                continue;
            }
            char* printWord =  createFile(full_path, file_permission, content);  
            printf("%s\n", printWord);  
        } else if (strcmp(cmd, "rmfile") == 0 ) {
            char* printWord =  deleteFile(full_path,user.permission); 
            printf("%s\n", printWord);
        } else if (strcmp(cmd, "read") == 0 ) {
            char content[MAX_BLCK_NUMBER_PER_FILE*BLOCK_SIZE];
            char* printWord =  readFile(full_path,content,user.permission); 
            if(strcmp(content,"")!=0)printf("File content: %s\n", content);
            else printf("%s\n", printWord);
        } else if (strcmp(cmd, "write") == 0 ) {
            char read_content[MAX_BLCK_NUMBER_PER_FILE*BLOCK_SIZE];
            char content[BLOCK_SIZE * MAX_BLCK_NUMBER_PER_FILE];
            char temp_path[256];
            strcpy(temp_path, full_path);
            char* printWord =  readFile(temp_path, read_content, user.permission);
            if (strcmp(read_content, "") == 0) {
                printf("%s\n", printWord);
            }
            printf("File content: %s\n", read_content);
            printf("Input the new content:\n");
            fgets(content, sizeof(content), stdin);
            content[strcspn(content, "\n")] = '\0';
            printWord =  writeFile(full_path, content, user.permission); 
            printf("%s\n", printWord);
        } else if (strcmp(cmd, "cd") == 0) {
            if(path == NULL) {
                printf("Invalid command!\n");
                continue;
            }
            if (strcmp(path, "-") == 0) {
                char temp[256];
                strcpy(temp, last_path);
                char* printWord =  moveDir(current_path, temp, last_path, user.permission);
                printf("%s\n", printWord);
            }else{
                char* printWord =   moveDir(current_path, full_path, last_path, user.permission);
                printf("%s\n", printWord);
            } 
        } else if (strcmp(cmd, "createUser") == 0 ) {
            if (strcmp(user.permission, "2") != 0){
                printf("You don't have permission to create a new user!\n");
                continue;
            }
            char content[MAX_BLCK_NUMBER_PER_FILE*BLOCK_SIZE];
            readFile("/pwd",content,"2");
            char newUserName[256], newPassword[256], userName[256];
            printf("Input the new username:");
            fgets(newUserName, sizeof(newUserName), stdin);
            newUserName[strcspn(newUserName, "\n")] = '\0';
            
            while(checkUserName(newUserName) == 1){
                printf("The username already exists, please input another username!\n");
                printf("Input the new username:");
                fgets(newUserName, sizeof(newUserName), stdin);
                newUserName[strcspn(newUserName, "\n")] = '\0';
            }
            strcpy(userName, newUserName);

            printf("Input the new password:");
            fgets(newPassword, sizeof(newPassword), stdin);
            newPassword[strcspn(newPassword, "\n")] = '\0';
            
            char position[2];
            printf("If you are patient, enter 3, if you are doctor, enter 1, if you are administrators, enter 2\n");
            fgets(position, sizeof(position), stdin);
            position[strcspn(position, "\n")] = '\0';

            if(position[0] != '1' && position[0] != '3' && position[0] != '2'){
                printf("position input error!\n");
                continue;
            }

            char permission[3];
            strcat(permission, position);
            if(position[0] != '2'){
                char DoctorGroup[1024];
                listGroup(DoctorGroup);
                printf("DoctorGroup: %s\n", DoctorGroup);

                char group[2];
                int c;
                while ((c = getchar()) != '\n' && c != EOF); // clear input buffer
                printf("Input the group you want to join:\n");
                fgets(group, sizeof(group), stdin);
                group[strcspn(group, "\n")] = '\0';

                strcat(permission, group);
            }
            

            // printf("permission: \nIf you are patient, please input 3n(n is your group), if you are doctor, please input 1n(n is your group), if you are administrators, please input 2\n");
            
            // fgets(permission, sizeof(permission), stdin);
            // permission[strcspn(permission, "\n")] = '\0';
            int permission_int = atoi(permission);
            char doctor_name[1024];
            if(permission_int/10 == 3){
                checkDoctor(permission_int, doctor_name);
                if(strcmp(doctor_name, "admin") == 0){
                    printf("There is no doctor in this group!\n");
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
            char* printWord = writeFile("/pwd", content, user.permission);
            printf("%s\n", printWord);
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
                    printf("%s\n", printWord);
                    printWord = linkPath( doctor_path, patient_path, user.permission);
                    printf("%s\n", printWord);
                    patient = strtok_r(NULL, delimiter_0, &saveptr1);
                }
            }
            if(permission_int/10 == 3){
                char delimiter_0[2] = " ";
                char *saveptr1;
                char patient_path[1024] = "/home/";
                strcat(patient_path, userName);
                char *doctor = strtok_r(doctor_name, delimiter_0, &saveptr1);
                while(doctor != NULL){
                    char full_path[1024] = "/home/";
                    strcat(full_path, doctor);
                    strcat(full_path, "/");
                    strcat(full_path, userName);
                    char doctor_path[1024];
                    strcpy(doctor_path,full_path);
                    char* printWord = createDirectory(full_path, user.permission);
                    printf("%s\n", printWord);
                    printWord = linkPath( doctor_path, patient_path, user.permission);
                    printf("%s\n", printWord);
                    doctor = strtok_r(NULL, delimiter_0, &saveptr1);
                }
            }

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
        } else if (strcmp(cmd, "ln") == 0 && strcmp(path, "")!=0) {
            printf("Input the path of the file you want to link:\n");
            char link_path[256];
            fgets(link_path, sizeof(link_path), stdin);
            link_path[strcspn(link_path, "\n")] = '\0';
            char* printWord =  linkPath( full_path, link_path, user.permission);
            printf("%s\n", printWord);
        }else if (strcmp(cmd, "quit") == 0) {
            running = 0;
        } else {
            printf("Please input a valid command\n");
        }
    }
    loadWord = saveFileSystem();
    printf("%s\n", loadWord);
    return 0;
}
