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


// 创建互斥锁
pthread_mutex_t filesystemLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t inodeLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t blockLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t bitmapLock = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t filesystemLock_r = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t inodeLock_r = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t blockLock_r = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t bitmapLock_r = PTHREAD_MUTEX_INITIALIZER;

int filesystemLock_count = 0;
int inodeLock_count = 0;
int blockLock_count = 0;
int bitmapLock_count = 0;
// struct Inode *current_pointer;  // 当前目录的i节点指针
void sendMessage(int clientSocket, const char* message) {
    // Send "OUTPUT" marker and the message to the client
    send(clientSocket, "OUTPUT", strlen("OUTPUT"), 0);
    usleep(1000);
    char buffer[1024];
    recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    send(clientSocket, message, strlen(message), 0);
    recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    printf("output: %s\n", message);
    printf("--------------------------send over--------------------------\n");
}

void receiveInput(int clientSocket, char* buffer, size_t size) {
    // Send "INPUT" marker to indicate client should send input
    memset(buffer, 0, size);
    send(clientSocket, "INPUT", strlen("INPUT"), 0);
    recv(clientSocket, buffer, size, 0);
    buffer[strcspn(buffer, "\n")] = '\0';  // Remove newline if exists
    printf("Input: %s\n", buffer);
    printf("--------------------------receive over--------------------------\n");
}
// 将文件系统信息保存到硬盘
char* saveFileSystem() {
    pthread_mutex_lock(&filesystemLock);  // 加锁

    FILE *file = fopen("filesystem", "wb");
    if (file == NULL) {
        // printf("Error: Unable to save filesystem data.\n");
        return "Error: Unable to save filesystem data.\n";
    }
    fwrite(inodeMem, sizeof(struct Inode), INODE_NUMBER, file);
    fwrite(blockMem, sizeof(struct DirectoryBlock), BLOCK_NUMBER, file);
    fwrite(blockBitmap, sizeof(blockBitmap), 1, file);
    fwrite(fileLock, sizeof(struct FileLock), MAX_FILE, file);
    fclose(file);
    // printf("Filesystem data saved successfully.\n");

    pthread_mutex_unlock(&filesystemLock);  // 解锁
    return "Filesystem data saved successfully.\n";
}

// 从硬盘读取文件系统信息
char* loadFileSystem() {
    pthread_mutex_lock(&filesystemLock_r);  // 加锁

    if(filesystemLock_count == 0){
        pthread_mutex_lock(&filesystemLock);
    }
    filesystemLock_count ++;
    pthread_mutex_unlock(&filesystemLock_r);  // 解锁
    FILE *file = fopen("filesystem", "rb");
    if (file == NULL) {
        // printf("Error: Unable to load filesystem data.\n");
        filesystemLock_count --;
        if(filesystemLock_count == 0){
            pthread_mutex_unlock(&filesystemLock);
        }
        return "Error: Unable to load filesystem data.\n";
    }
    fread(inodeMem, sizeof(struct Inode), INODE_NUMBER, file);
    fread(blockMem, sizeof(struct DirectoryBlock), BLOCK_NUMBER, file);
    fread(blockBitmap, sizeof(blockBitmap), 1, file);
    fread(fileLock, sizeof(struct FileLock), MAX_FILE, file);
    fclose(file);
    // printf("Filesystem data loaded successfully.\n");

    filesystemLock_count --;
    if(filesystemLock_count == 0){
        pthread_mutex_unlock(&filesystemLock);
    }
    return "Filesystem data loaded successfully.\n";
}

// 前往目录函数
_Bool goToDirectory(char *path_origin, char *permission) {
    char path[256];
    strcpy(path, path_origin);
    int i, flag;
    char *directory, *parent;
    const char delimiter[2] = "/";
    pthread_mutex_lock(&inodeLock_r);  // 加锁
    pthread_mutex_lock(&blockLock_r);  // 加锁
    if(inodeLock_count == 0){
        pthread_mutex_lock(&inodeLock);
    }
    inodeLock_count ++;
    if(blockLock_count == 0){
        pthread_mutex_lock(&blockLock);
    }
    blockLock_count ++;
    pthread_mutex_unlock(&inodeLock_r);  // 解锁
    pthread_mutex_unlock(&blockLock_r);  // 解锁
    struct Inode *pointer = inodeMem;
    struct DirectoryBlock *block;

    // 递归访问父目录
    parent = NULL;
    directory = strtok(path, delimiter);
    if (directory == NULL) {
        printf("Server: It is the root directory!\n");
        inodeLock_count --;
        blockLock_count --;
        if(inodeLock_count == 0){
            pthread_mutex_unlock(&inodeLock);
        }
        if(blockLock_count == 0){
            pthread_mutex_unlock(&blockLock);
        }
        // current_pointer = &inodeMem[0];
        return 1;
    }
    while (directory != NULL) {
        if (parent != NULL) {
            block = (struct DirectoryBlock *) &blockMem[pointer->blockID[0]];
            flag = 0;
            for (i = 0; i < ENTRY_NUMBER; i++) { 
                if (strcmp(block->fileName[i], parent) == 0) {
                    flag = 1;
                    pointer = &inodeMem[block->inodeID[i]];
                    break;
                }
            }
            if (flag == 0 || pointer->fileType == 1) {
                // 如果父目录不存在或者不是目录文件
                printf("Server: The path does not exist!\n");
                inodeLock_count --;
                blockLock_count --;
                if(inodeLock_count == 0){
                    pthread_mutex_unlock(&inodeLock);
                }
                if(blockLock_count == 0){
                    pthread_mutex_unlock(&blockLock);
                }
                return 0;
            }
        }
        parent = directory;
        directory = strtok(NULL, delimiter);
    }

    // 进入目标目录
    block = (struct DirectoryBlock *) &blockMem[pointer->blockID[0]];
    for (i = 0; i < ENTRY_NUMBER; i++) {
        if (strcmp(block->fileName[i], parent) == 0) {
            int n_inode = block->inodeID[i];
            if (inodeMem[n_inode].fileType == 0) {
                // current_pointer = &inodeMem[n_inode];
                printf("Server: Directory changed successfully!\n");
                inodeLock_count --;
                blockLock_count --;
                if(inodeLock_count == 0){
                    pthread_mutex_unlock(&inodeLock);
                }
                if(blockLock_count == 0){
                    pthread_mutex_unlock(&blockLock);
                }
                return 1;
            } else {
                printf("Server: The specified path is not a directory!\n");
                inodeLock_count --;
                blockLock_count --;
                if(inodeLock_count == 0){
                    pthread_mutex_unlock(&inodeLock);
                }
                if(blockLock_count == 0){
                    pthread_mutex_unlock(&blockLock);
                }
                return 0;
            }
        }
    }
    printf("Server: The directory does not exist!\n");

    
    return 0;
}
// 创建目录函数
char* createDirectory(char *path, char *permission) {
    int permission_int = atoi(permission);
    if (permission_int/10 == 3 && permission_int != 2){
        // printf("You are patient, you can't create a directory!\n");

        return "You are patient, you can't create a directory!\n";
    }
    int i, j, flag;
    char *directory, *parent, *target;
    const char delimiter[2] = "/"; // 分隔符
    struct Inode *pointer = inodeMem; // 指向当前目录的i节点
    struct DirectoryBlock *block;
    
    
    // 递归访问父目录
    parent = NULL;
    directory = strtok(path, delimiter);
    if (directory == NULL) {
        // printf("It is the root directory!\n");

        return "It is the root directory!\n";
    }
    while (directory != NULL) { // 逐级访问目录
        if (parent != NULL) { // 除了根目录外，逐级访问目录
            block = (struct DirectoryBlock *) &blockMem[pointer->blockID[0]]; // 由于 blockMem 是一个 FileBlock 数组，直接访问时无法以 DirectoryBlock 方式来处理。但是在文件系统中，目录块和文件块都存储在相同的 blockMem 数组中。此时pointer指向当前目录的i节点
            flag = 0;
            for (i = 0; i < ENTRY_NUMBER; i++) { 
                if (strcmp(block->fileName[i], parent) == 0) { // 查找父目录
                    flag = 1;
                    pointer = &inodeMem[block->inodeID[i]]; // 更新当前目录的i节点
                    break;
                }
            }
            if (flag == 0 || pointer->fileType == 1) {
                // 如果父目录不存在或者不是目录文件
                // printf("The path does not exist!\n");
                return "The path does not exist!\n";
            }
        }
        parent = directory;
        directory = strtok(NULL, delimiter);
    }
    
    
    {
        pthread_mutex_lock(&blockLock);
        // 创建目标目录
        int entry = -1, n_block = -1, n_inode = -1; // entry: 目录块中的条目号；n_block: 新目录块编号；n_inode: 新目录i节点编号
        target = parent;
        block = (struct DirectoryBlock *) &blockMem[pointer->blockID[0]]; //由于 blockMem 是一个 FileBlock 数组，直接访问时无法以 DirectoryBlock 方式来处理。但是在文件系统中，目录块和文件块都存储在相同的 blockMem 数组中。
        for (i = 0; i < ENTRY_NUMBER; i++) {
            if (strcmp(block->fileName[i], target) == 0) {
                // printf("The directory already exists!\n");
                pthread_mutex_unlock(&blockLock);  // 解锁
                return "The directory already exists!\n";
            }
        }
        for(i = 0; i < ENTRY_NUMBER; i++){
            if (block->inodeID[i] == -1) {
                entry = i;
                break;
            }
        }
        
        
        if (entry >= 0) {
            pthread_mutex_lock(&bitmapLock);
            // 查找未使用的块
            for (i = 0; i < BLOCK_NUMBER/8; i++) {
                for (j = 0; j < 8; j++) {
                    if ((blockBitmap[i] & (1 << j)) == 0) {
                        n_block = i * 8 + j;
                        break;
                    }
                }
                if (n_block != -1) {
                    break;
                }
            }
            if (n_block == -1) {
                // printf("The block is full!\n");
                pthread_mutex_unlock(&blockLock);  // 解锁
                pthread_mutex_unlock(&bitmapLock);  // 解锁
                return "The block is full!\n";
            }
            
            
            // 查找未使用的i节点
            flag = 0;
            for (i = 0; i < INODE_NUMBER; i++) {
                if (inodeMem[i].blockID[0] == -1) {
                    pthread_mutex_lock(&inodeLock);  // 加锁
                    flag = 1;
                    inodeMem[i].blockID[0] = n_block;
                    inodeMem[i].fileType = 0;
                    inodeMem[i].blockNum = 1;
                    inodeMem[i].permission = permission_int;
                    inodeMem[i].fileNum++;
                    n_inode = i;
                    pthread_mutex_unlock(&inodeLock);  // 解锁
                    break;
                }
            }
            
            
            if (n_inode == -1) {
                // printf("The inode is full!\n");
                pthread_mutex_unlock(&blockLock);  // 解锁
                pthread_mutex_unlock(&bitmapLock);  // 解锁
                return "The inode is full!\n";
            }

            // 初始化新目录文件
            block->inodeID[entry] = n_inode;
            strcpy(block->fileName[entry], target);
            blockBitmap[n_block/8] |= 1 << (n_block % 8);

            block = (struct DirectoryBlock *) &blockMem[n_block];
            for (i = 0; i < ENTRY_NUMBER; i++) {
                block->inodeID[i] = -1;
            }

            //创建当前目录和上级目录
            block->inodeID[0] = n_inode;
            strcpy(block->fileName[0], ".");
            
            
            block->inodeID[1] = pointer->inodeNumber;
            strcpy(block->fileName[1], "..");
            
            
            pthread_mutex_unlock(&bitmapLock);  // 解锁
            pthread_mutex_unlock(&blockLock);  // 解锁
            saveFileSystem();
            return "Directory created successfully!\n";
            // printf("Directory created successfully!\n");
            
        }
        else {
            pthread_mutex_unlock(&blockLock);  // 解锁
            pthread_mutex_unlock(&bitmapLock);  // 解锁
            return "The directory is full!\n";
            // printf("The directory is full!\n");
        }
        
    }
}

// 链接目录函数
char* linkPath(char *path, char *path_origin, char *permission) {
    int permission_int = atoi(permission);
    if (permission_int/10 == 3 && permission_int != 2){
        return "You are patient, you can't link a directory!\n";
    }
    int i, j, flag;
    char *directory, *parent, *target, *directory_origin, *parent_origin;
    const char delimiter[2] = "/"; // 分隔符
    struct Inode *pointer = inodeMem; // 指向当前目录的i节点
    struct Inode *pointer_origin = inodeMem; 
    struct DirectoryBlock *block, *block_origin;
    char *saveptr1, *saveptr2;

    // 递归访问父目录
    parent = NULL;
    parent_origin = NULL;
    directory = strtok_r(path, delimiter, &saveptr1);
    directory_origin = strtok_r(path_origin, delimiter, &saveptr2);
    if (directory == NULL) {
        return "It is the root directory!\n";
    }
    // 找到链接父目录
    while (directory != NULL) { // 逐级访问目录
        if (parent != NULL) { // 除了根目录外，逐级访问目录
            block = (struct DirectoryBlock *) &blockMem[pointer->blockID[0]]; // 由于 blockMem 是一个 FileBlock 数组，直接访问时无法以 DirectoryBlock 方式来处理。但是在文件系统中，目录块和文件块都存储在相同的 blockMem 数组中。此时pointer指向当前目录的i节点
            flag = 0;
            for (i = 0; i < ENTRY_NUMBER; i++) { 
                if (strcmp(block->fileName[i], parent) == 0) { // 查找父目录
                    flag = 1;
                    pointer = &inodeMem[block->inodeID[i]]; // 更新当前目录的i节点
                    break;
                }
            }
            if (flag == 0 || pointer->fileType == 1) {
                // 如果父目录不存在或者不是目录文件
                // printf("The path does not exist!\n");
                return "The path does not exist!\n";
            }
        }
        parent = directory;
        directory = strtok_r(NULL, delimiter, &saveptr1);
    }

    while(directory_origin != NULL){
        if (parent_origin != NULL) { // 除了根目录外，逐级访问目录
            block_origin = (struct DirectoryBlock *) &blockMem[pointer_origin->blockID[0]]; // 由于 blockMem 是一个 FileBlock 数组，直接访问时无法以 DirectoryBlock 方式来处理。但是在文件系统中，目录块和文件块都存储在相同的 blockMem 数组中。此时pointer指向当前目录的i节点
            flag = 0;
            for (i = 0; i < ENTRY_NUMBER; i++) { 
                if (strcmp(block_origin->fileName[i], parent_origin) == 0) { // 查找父目录
                    flag = 1;
                    pointer_origin = &inodeMem[block_origin->inodeID[i]]; // 更新当前目录的i节点
                    break;
                }
            }
            if (flag == 0 || pointer_origin->fileType == 1) {
                // 如果父目录不存在或者不是目录文件
                // printf("The path does not exist!\n");
                return "The path does not exist!\n";
            }
        }
        parent_origin = directory_origin;
        directory_origin = strtok_r(NULL, delimiter, &saveptr2);
    }

    block_origin = (struct DirectoryBlock *) &blockMem[pointer_origin->blockID[0]];
    int n_inode =-1;
    int flag_origin = 0;
    for (i = 0; i < ENTRY_NUMBER; i++) {
        if (strcmp(block_origin->fileName[i], parent_origin) == 0) {
            flag_origin = 1;
            int n_inode_origin = block_origin->inodeID[i];
            int entry = -1;
            target = parent;
            block = (struct DirectoryBlock *) &blockMem[pointer->blockID[0]]; //由于 blockMem 是一个 FileBlock 数组，直接访问时无法以 DirectoryBlock 方式来处理。但是在文件系统中，目录块和文件块都存储在相同的 blockMem 数组中。
            for (int k = 0; k < ENTRY_NUMBER; k++) {
                if (strcmp(block->fileName[k], target) == 0) {
                    n_inode = block->inodeID[k];
                    int blockNum = inodeMem[n_inode].blockNum;
                    // 检查目录是否为空
                    struct DirectoryBlock *target_block = (struct DirectoryBlock*) &blockMem[inodeMem[n_inode].blockID[0]];
                    for(int z = 2; z < ENTRY_NUMBER && inodeMem[n_inode].fileType == 0; z++){
                        if (target_block->inodeID[z] != -1) {
                            return "The directory is not empty!\n";
                        }
                    }
                    for(j = 0; j < blockNum; j++){
                        {
                            pthread_mutex_lock(&bitmapLock);  // 加锁
                            blockBitmap[inodeMem[n_inode].blockID[j] / 8] &= ~(1 << (inodeMem[n_inode].blockID[j] % 8)); 
                            pthread_mutex_unlock(&bitmapLock);  // 解锁
                        }

                        {
                            pthread_mutex_lock(&inodeLock);  // 加锁
                            inodeMem[n_inode].blockID[j] = -1;
                            inodeMem[n_inode].fileType = -1;
                            inodeMem[n_inode].blockNum = 0;
                            inodeMem[n_inode].permission = -1;
                            inodeMem[n_inode].fileNum = 0;
                            inodeMem[n_inode_origin].fileNum++;
                            pthread_mutex_unlock(&inodeLock);  // 解锁
                        }
                    }
                    // printf("The directory already exists!\n");
                    entry = k;
                    break;
                }
            }
            if(entry>=0){
                block->inodeID[entry] = n_inode_origin;
                break;
            }else{
                return "The path does not exist!\n";
            }
        }
    }
    if(flag_origin == 0){
        return "The path does not exist!\n";
    }else{
        saveFileSystem();
        return "Path linked successfully!\n";
    }
}

// 删除目录函数
char* deleteDirectory(char *path, char *permission) {
    int permission_int = atoi(permission);
    if(permission_int/10 == 3 && permission_int != 2){
        // printf("You are patient, you can't delete a directory!\n");
        
        return "You are patient, you can't delete a directory!\n";
    }
    int i, flag;
    char *directory, *parent;
    const char delimiter[2] = "/";
    struct Inode *pointer = inodeMem;
    struct DirectoryBlock *block;

    // 递归访问父目录
    parent = NULL;
    directory = strtok(path, delimiter);
    if (directory == NULL) {
        // printf("It is the root directory!\n");
        
        return "It is the root directory!\n";
    }
    while (directory != NULL) {
        if (parent != NULL) {
            block = (struct DirectoryBlock *) &blockMem[pointer->blockID[0]];
            flag = 0;
            for (i = 0; i < ENTRY_NUMBER; i++) { 
                if (strcmp(block->fileName[i], parent) == 0) {
                    flag = 1;
                    pointer = &inodeMem[block->inodeID[i]];
                    break;
                }
            }
            if (flag == 0 || pointer->fileType == 1) {
                // 如果父目录不存在或者不是目录文件
                // printf("The path does not exist!\n");
                
                return "The path does not exist!\n";
            }
        }
        parent = directory;
        directory = strtok(NULL, delimiter);
    }
    {
        pthread_mutex_lock(&blockLock);  // 加锁
        // 删除目标目录
        block = (struct DirectoryBlock *) &blockMem[pointer->blockID[0]];
        for (i = 0; i < ENTRY_NUMBER; i++) {
            if (strcmp(block->fileName[i], parent) == 0) {
                int n_inode = block->inodeID[i];
                if (inodeMem[n_inode].permission == 2 && permission_int != 2) { // 管理员创建的目录只有管理员可以删除
                    // printf("You don't have permission to delete the directory!\n");
                    pthread_mutex_unlock(&blockLock);  // 解锁
                    return "You are not administrator. You don't have permission to delete the directory!\n";
                }else if(inodeMem[n_inode].permission != permission_int && inodeMem[n_inode].permission != 2){
                    pthread_mutex_unlock(&blockLock);  // 解锁
                    return "You don't have permission to delete the directory!\n";
                }
                if (inodeMem[n_inode].fileType == 0) { // 检查是否为目录文件
                    struct DirectoryBlock *targetBlock = (struct DirectoryBlock *) &blockMem[inodeMem[n_inode].blockID[0]];
                    if(inodeMem[n_inode].fileNum == 1){
                        // 检查目录是否为空
                        for (int j = 2; j < ENTRY_NUMBER; j++) {
                            if (targetBlock->inodeID[j] != -1) {
                                // printf("The directory is not empty!\n");
                                pthread_mutex_unlock(&blockLock);  // 解锁
                                return "The directory is not empty!\n";
                            }
                        }
                        // 删除目录
                        block->inodeID[i] = -1;
                        memset(block->fileName[i], '\0', sizeof(block->fileName[i]));
                        {
                            pthread_mutex_lock(&bitmapLock);  // 加锁
                            blockBitmap[inodeMem[n_inode].blockID[0] / 8] &= ~(1 << (inodeMem[n_inode].blockID[0] % 8)); 
                            pthread_mutex_unlock(&bitmapLock);  // 解锋
                        }

                        {
                            pthread_mutex_lock(&inodeLock);  // 加锁
                            inodeMem[n_inode].blockID[0] = -1;
                            inodeMem[n_inode].fileType = -1;
                            inodeMem[n_inode].blockNum = 0;
                            inodeMem[n_inode].permission = -1;
                            inodeMem[n_inode].fileNum = 0;
                            pthread_mutex_unlock(&inodeLock);  // 解锁
                        }
                    }else{
                        // 删除目录
                        block->inodeID[i] = -1;
                        memset(block->fileName[i], '\0', sizeof(block->fileName[i]));
                        inodeMem[n_inode].fileNum--;
                    }
                    // printf("Directory deleted successfully!\n");
                    pthread_mutex_unlock(&blockLock);  // 解锁
                    saveFileSystem();
                    return "Directory deleted successfully!\n";
                } else {
                    // printf("The specified path is not a directory!\n");
                    pthread_mutex_unlock(&blockLock);  // 解锁
                    return "The specified path is not a directory!\n";
                }
            }
        }
        pthread_mutex_unlock(&blockLock);  // 解锁
    }
    // printf("The directory does not exist!\n");
    
    return "The directory does not exist!\n";
}

// 创建文件函数（修改了块分配和iNode的大小最依据文件的实际需求）
char* createFile(char *path, int file_permission, char* content) {
    int i, j, flag;
    char *directory, *parent, *target;
    const char delimiter[2] = "/";
    struct Inode *pointer = inodeMem;
    struct DirectoryBlock *block;

    // 递序访问父目录
    parent = NULL;
    directory = strtok(path, delimiter);
    if (directory == NULL) {
        // printf("It is the root directory!\n");
        return "It is the root directory!\n";
    }
    while (directory != NULL) {
        if (parent != NULL) {
            block = (struct DirectoryBlock *) &blockMem[pointer->blockID[0]];
            flag = 0;
            for (i = 0; i < ENTRY_NUMBER; i++) { 
                if (strcmp(block->fileName[i], parent) == 0) {
                    flag = 1;
                    pointer = &inodeMem[block->inodeID[i]];
                    break;
                }
            }
            if (flag == 0 || pointer->fileType != 0) {
                // 如果父目录不存在或者不是目录文件
                // printf("The path does not exist or is not a directory!\n");
                return "The path does not exist or is not a directory!\n";
            }
        }
        parent = directory;
        directory = strtok(NULL, delimiter);
    }
    {
        pthread_mutex_lock(&blockLock);  // 加锁
        // 创建目标文件
        int entry = -1, n_inode = -1;
        int N_block[MAX_BLCK_NUMBER_PER_FILE] = {-1};
        target = parent;
        block = (struct DirectoryBlock *) &blockMem[pointer->blockID[0]];
        for (i = 0; i < ENTRY_NUMBER; i++) { // 查找目录块中的空闲条目
            if (strcmp(block->fileName[i], target) == 0) {
                // printf("The file already exists!\n");
                pthread_mutex_unlock(&blockLock);  // 解锁
                return "The file already exists!\n";
            }
        }
        for(i = 0; i < ENTRY_NUMBER; i++){
            if (block->inodeID[i] == -1) {
                entry = i;
                break;
            }
        }
        if (entry >= 0) {
            // 查找未使用的块
            int block_pointer = 0;
            int block_num = strlen(content) / BLOCK_SIZE + ((strlen(content) % BLOCK_SIZE != 0) ? 1 : 0);
            if (block_num > MAX_BLCK_NUMBER_PER_FILE) {
                // printf("The file is too large!\n");
                pthread_mutex_unlock(&blockLock);  // 解锁
                return "The file is too large!\n";
            }
            {
                pthread_mutex_lock(&bitmapLock);  // 加锁
                for (i = 0; i < BLOCK_NUMBER / 8; i++) {
                    for (j = 0; j < 8; j++) {
                        if ((blockBitmap[i] & (1 << j)) == 0) {
                            N_block[block_pointer] = i * 8 + j;
                            block_pointer++;
                            if (block_pointer == block_num) {
                                break;
                            }
                        }
                    }
                    if (block_pointer == block_num) {
                        break;
                    }
                }
                if (block_pointer != block_num) {
                    // printf("The block is not enough for a file!\n");
                    pthread_mutex_unlock(&bitmapLock);  // 解锁
                    pthread_mutex_unlock(&blockLock);  // 解锁
                    return "The block is not enough for a file!\n";
                }

                // 查找未使用的i节点
                for (i = 0; i < INODE_NUMBER; i++) {
                    if (inodeMem[i].blockID[0] == -1) {
                        pthread_mutex_lock(&inodeLock);  // 加锁
                        for (int k = 0; k < block_num; k++) {
                            inodeMem[i].blockID[k] = N_block[k];
                        }
                        inodeMem[i].fileType = 1;
                        inodeMem[i].blockNum = block_num;
                        inodeMem[i].permission = file_permission;
                        inodeMem[i].fileNum++;
                        n_inode = i;
                        pthread_mutex_unlock(&inodeLock);  // 解锁
                        break;
                    }
                }
                if (n_inode == -1) {
                    // printf("The inode is full!\n");
                    pthread_mutex_unlock(&bitmapLock);  // 解锁
                    pthread_mutex_unlock(&blockLock);  // 解锁
                    return "The inode is full!\n";
                }

                // 初始化新文件
                block->inodeID[entry] = n_inode;
                strcpy(block->fileName[entry], target);
                
                for (int i = 0; i < block_num; i++) {
                    blockBitmap[N_block[i] / 8] |= 1 << (N_block[i] % 8);
                    struct FileBlock *fileBlock = (struct FileBlock *) &blockMem[N_block[i]];
                    strncpy(fileBlock->content, content + i * BLOCK_SIZE, BLOCK_SIZE);
                    fileBlock->content[BLOCK_SIZE - 1] = '\0';
                }
                pthread_mutex_unlock(&bitmapLock);  // 解锁
            }
            // printf("File created successfully!\n");
            pthread_mutex_unlock(&blockLock);  // 解锁
            saveFileSystem();
            return "File created successfully!\n";
        } else {
            // printf("The directory is full!\n");
            pthread_mutex_unlock(&blockLock);  // 解锁
            return "The directory is full!\n";
        }
    }
}


// 删除文件函数
char* deleteFile(char *path, char *permission) {
    int permission_int = atoi(permission);
    if(permission_int/10 == 3 && permission_int != 2){
        // printf("You are patient, you can't delete a file!\n");
        return "You are patient, you can't delete a file!\n";
    }
    int i, flag;
    char *directory, *parent;
    const char delimiter[2] = "/";
    struct Inode *pointer = inodeMem;
    struct DirectoryBlock *block;


    // 递归访问父目录
    parent = NULL;
    directory = strtok(path, delimiter);
    if (directory == NULL) {
        // printf("It is the root directory!\n");
        return "It is the root directory!\n";
    }
    while (directory != NULL) {
        if (parent != NULL) {
            block = (struct DirectoryBlock *) &blockMem[pointer->blockID[0]];
            flag = 0;
            for (i = 0; i < ENTRY_NUMBER; i++) { 
                if (strcmp(block->fileName[i], parent) == 0) {
                    flag = 1;
                    pointer = &inodeMem[block->inodeID[i]];
                    break;
                }
            }
            if (flag == 0 || pointer->fileType == 1) {
                // 如果父目录不存在或者不是目录文件
                // printf("The path does not exist!\n");
                return "The path does not exist!\n";
            }
        }
        parent = directory;
        directory = strtok(NULL, delimiter);
    }
    {
        pthread_mutex_lock(&blockLock);  // 加锁
        // 删除目标文件
        block = (struct DirectoryBlock *) &blockMem[pointer->blockID[0]];
        for (i = 0; i < ENTRY_NUMBER; i++) {
            if (strcmp(block->fileName[i], parent) == 0) {
                int n_inode = block->inodeID[i];
                if (inodeMem[n_inode].permission/100 == 2 && permission_int/10 == 1) {
                    // printf("You don't have permission to delete the file!\n");
                    pthread_mutex_unlock(&blockLock);  // 解锁
                    return "You don't have permission to delete the file!\n";
                }
                if (inodeMem[n_inode].fileType == 1) {
                    if (permission_int/10 != inodeMem[n_inode].permission/100 || permission_int%10 != inodeMem[n_inode].permission%10){
                        // printf("You don't have permission to delete the file!\n");
                        pthread_mutex_unlock(&blockLock);  // 解锁
                        return "You don't have permission to delete the file!\n";
                    }
                    // 删除文件
                    block->inodeID[i] = -1;
                    memset(block->fileName[i], '\0', sizeof(block->fileName[i]));
                    int blockNum = inodeMem[n_inode].blockNum;
                    
                    if(inodeMem[n_inode].fileNum == 1){
                        for(int j = 0; j<blockNum; j++){ //修改
                            {
                                pthread_mutex_lock(&bitmapLock);  // 加锁
                                blockBitmap[inodeMem[n_inode].blockID[j] / 8] &= ~(1 << (inodeMem[n_inode].blockID[j] % 8)); 
                                pthread_mutex_unlock(&bitmapLock);  // 解锋
                            }

                            {
                                pthread_mutex_lock(&inodeLock);  // 加锁
                                inodeMem[n_inode].blockID[j] = -1;
                                inodeMem[n_inode].fileType = -1;
                                inodeMem[n_inode].blockNum = 0;
                                inodeMem[n_inode].fileNum = 0;
                                pthread_mutex_unlock(&inodeLock);  // 解锁
                            }
                        }
                    }else{
                        block->inodeID[i] = -1;
                        memset(block->fileName[i], '\0', sizeof(block->fileName[i]));
                        inodeMem[n_inode].fileNum--;
                    }
                    
                    // printf("File deleted successfully!\n");
                    pthread_mutex_unlock(&blockLock);  // 解锁
                    saveFileSystem();
                    return "File deleted successfully!\n";
                } else {
                    // printf("The specified path is not a file!\n");
                    pthread_mutex_unlock(&blockLock);  // 解锁
                    return "The specified path is not a file!\n";
                }
            }
        }
        pthread_mutex_unlock(&blockLock);  // 解锁
    }
    // printf("The file does not exist!\n");
    return "The file does not exist!\n";
}

// 读取文件函数
char* readFile(char *path, char *content_back, char *permission) {
    pthread_mutex_lock(&inodeLock_r);
    pthread_mutex_lock(&blockLock_r);
    if(inodeLock_count == 0){
        pthread_mutex_lock(&inodeLock);
    }
    inodeLock_count ++;
    if(blockLock_count == 0){
        pthread_mutex_lock(&blockLock);
    }
    blockLock_count ++;
    pthread_mutex_unlock(&inodeLock_r);
    pthread_mutex_unlock(&blockLock_r);

    int permission_int = atoi(permission);
    int i, flag;
    // char path[100];
    char *directory, *parent;
    const char delimiter[2] = "/";
    struct Inode *pointer = inodeMem;
    struct DirectoryBlock *block;

    // printf("Input the path:\n");
    // scanf("%s", path);

    // 递归访问父目录
    parent = NULL;
    directory = strtok(path, delimiter);
    if (directory == NULL) {
        // printf("It is the root directory!\n");
        inodeLock_count --;
        blockLock_count --;
        if(inodeLock_count == 0){
            pthread_mutex_unlock(&inodeLock);
        }
        if(blockLock_count == 0){
            pthread_mutex_unlock(&blockLock);
        }
        return "It is the root directory!\n";
    }
    while (directory != NULL) {
        if (parent != NULL) {
            block = (struct DirectoryBlock *) &blockMem[pointer->blockID[0]];
            flag = 0;
            for (i = 0; i < ENTRY_NUMBER; i++) { 
                if (strcmp(block->fileName[i], parent) == 0) {
                    flag = 1;
                    pointer = &inodeMem[block->inodeID[i]];
                    break;
                }
            }
            if (flag == 0 || pointer->fileType == 1) {
                // 如果父目录不存在或者不是目录文件
                // printf("The path does not exist!\n");
                inodeLock_count --;
                blockLock_count --;
                if(inodeLock_count == 0){
                    pthread_mutex_unlock(&inodeLock);
                }
                if(blockLock_count == 0){
                    pthread_mutex_unlock(&blockLock);
                }
                return "The path does not exist!\n";
            }
        }
        parent = directory;
        directory = strtok(NULL, delimiter);
    }

    // 读取文件
    block = (struct DirectoryBlock *) &blockMem[pointer->blockID[0]];
    for (i = 0; i < ENTRY_NUMBER; i++) {
        if (strcmp(block->fileName[i], parent) == 0) {
            int n_inode = block->inodeID[i];
            if (inodeMem[n_inode].fileType == 1) {
                if ((permission_int/10 != inodeMem[n_inode].permission/100 && (inodeMem[n_inode].permission/10)%10 != permission_int/10  || inodeMem[n_inode].permission%10 != permission_int%10 && inodeMem[n_inode].permission%10 != 0) && permission_int != inodeMem[n_inode].permission/100){
                    // printf("permission_int: %d\n", permission_int);
                    // printf("inodeMem[n_inode].permission: %d\n", inodeMem[n_inode].permission);
                    // printf("You don't have permission to read the file!\n");
                    strcpy(content_back, "");
                    printf("permission_int: %d\n", permission_int);
                    printf("inodeMem[n_inode].permission: %d\n", inodeMem[n_inode].permission);
                    inodeLock_count --;
                    blockLock_count --;
                    if(inodeLock_count == 0){
                        pthread_mutex_unlock(&inodeLock);
                    }
                    if(blockLock_count == 0){
                        pthread_mutex_unlock(&blockLock);
                    }
                    return "You don't have permission to read the file!\n";
                }
                char content [inodeMem[n_inode].blockNum*BLOCK_SIZE];
                for(int j = 0;j<inodeMem[n_inode].blockNum;j++){
                    int offset = j*BLOCK_SIZE;
                    struct FileBlock *fileBlock = (struct FileBlock *) &blockMem[inodeMem[n_inode].blockID[j]];
                    // Copy content from each block to the appropriate offset in the content array
                    strncpy(content + offset, fileBlock->content, BLOCK_SIZE);
                }
                content[inodeMem[n_inode].blockNum * BLOCK_SIZE - 1] = '\0';
                strcpy(content_back, content);
                // printf("File content: %s\n", content);
                inodeLock_count --;
                blockLock_count --;
                if(inodeLock_count == 0){
                    pthread_mutex_unlock(&inodeLock);
                }
                if(blockLock_count == 0){
                    pthread_mutex_unlock(&blockLock);
                }
                return "File content: ";
            } else {
                // printf("The specified path is not a file!\n");
                inodeLock_count --;
                blockLock_count --;
                if(inodeLock_count == 0){
                    pthread_mutex_unlock(&inodeLock);
                }
                if(blockLock_count == 0){
                    pthread_mutex_unlock(&blockLock);
                }
                return "The specified path is not a file!\n";
            }
        }
    }
    // printf("The file does not exist!\n");
    strcpy(content_back, "");
    inodeLock_count --;
    blockLock_count --;
    if(inodeLock_count == 0){
        pthread_mutex_unlock(&inodeLock);
    }
    if(blockLock_count == 0){
        pthread_mutex_unlock(&blockLock);
    }
    return "The file does not exist!\n";
}

// 写入文件函数（动态增加或减少块数量）
char* writeFile(char *path, char *content, char *permission, int update) {
    int permission_int = atoi(permission);
    if(permission_int/10 == 3 && permission_int != 2){
        // printf("You are patient, you can't write a file!\n");
        return "You are patient, you can't write a file!\n";
    }
    int i, j, flag;

    char *directory, *parent;
    const char delimiter[2] = "/";
    struct Inode *pointer = inodeMem;
    struct DirectoryBlock *block;
    

    // printf("Input the path:\n");
    // fgets(path, sizeof(path), stdin);
    // path[strcspn(path, "\n")] = '\0';  // 去掉行进符
    printf("test2\n");
    
    size_t content_size = strlen(content);
    if(strcmp(path, "/pwd") == 0 && update == 0){
        if (strlen(content) == 0) {
            // printf("The content is empty!\n");
            // createFile(path, permission);
            return "The content is empty!\n";
        }
        char content_temp[MAX_BLCK_NUMBER_PER_FILE*BLOCK_SIZE];
        strcpy(content_temp, content);
        char *saveptr1, *saveptr2;
        char *USER = strtok_r(content, "/", &saveptr1);
        while (USER != NULL) {
            char user_copy[256];
            strncpy(user_copy, USER, sizeof(user_copy) - 1);
            user_copy[sizeof(user_copy) - 1] = '\0';  // 确保字符串以空字符结尾

            char *username = strtok_r(user_copy, "-", &saveptr2);
            char home[256] = "/home/";
            char full_path[256] = "/home/";
            strcat(full_path, username);

            if (strcmp(full_path, "/home/admin") != 0) {
                createDirectory(home, permission);
                printf("test3\n");
                
                createDirectory(full_path, permission);
            }
            printf("root 11:\n");
            
            USER = strtok_r(NULL, "/", &saveptr1);
        }
        strcpy(content, content_temp);
    }
    
    // 递归访问父目录
    parent = NULL;
    directory = strtok(path, delimiter);
    if (directory == NULL) {
        // printf("It is the root directory!\n");
        return "It is the root directory!\n";
    }
    while (directory != NULL) {
        if (parent != NULL) {
            block = (struct DirectoryBlock *) &blockMem[pointer->blockID[0]];
            flag = 0;
            for (i = 0; i < ENTRY_NUMBER; i++) { 
                if (strcmp(block->fileName[i], parent) == 0) {
                    flag = 1;
                    pointer = &inodeMem[block->inodeID[i]];
                    break;
                }
            }
            if (flag == 0 || pointer->fileType != 0) {
                // 如果父目录不存在或者不是目录文件
                // printf("The path does not exist or is not a directory!\n");
                return "The path does not exist or is not a directory!\n";
            }
        }
        parent = directory;
        directory = strtok(NULL, delimiter);
    }
    
    // 写入文件
    block = (struct DirectoryBlock *) &blockMem[pointer->blockID[0]];
    for (i = 0; i < ENTRY_NUMBER; i++) {
        if (strcmp(block->fileName[i], parent) == 0) {
            int n_inode = block->inodeID[i];
            if (inodeMem[n_inode].fileType == 1) {
                int old_block_num = inodeMem[n_inode].blockNum;
                int new_block_num = content_size / BLOCK_SIZE + ((content_size % BLOCK_SIZE != 0) ? 1 : 0);

                if (new_block_num > MAX_BLCK_NUMBER_PER_FILE) {
                    // printf("The file is too large!\n");
                    return "The file is too large!\n";
                }

                // 释放多余的块
                if (new_block_num < old_block_num) {
                    for (j = new_block_num; j < old_block_num; j++) {
                        blockBitmap[inodeMem[n_inode].blockID[j] / 8] &= ~(1 << (inodeMem[n_inode].blockID[j] % 8));

                        {
                            pthread_mutex_lock(&inodeLock);  // 加锁
                            inodeMem[n_inode].blockID[j] = -1;
                            pthread_mutex_unlock(&inodeLock);  // 解锁
                        }
                    }
                }
                // 分配新的块
                if (new_block_num > old_block_num) {
                    for (j = old_block_num; j < new_block_num; j++) {
                        int new_block = -1;
                        for (int k = 0; k < BLOCK_NUMBER / 8; k++) {
                            for (int l = 0; l < 8; l++) {
                                if ((blockBitmap[k] & (1 << l)) == 0) {
                                    new_block = k * 8 + l;
                                    blockBitmap[k] |= 1 << l;
                                    {
                                        pthread_mutex_lock(&inodeLock);  // 加锁
                                        inodeMem[n_inode].blockID[j] = new_block;
                                        pthread_mutex_unlock(&inodeLock);  // 解锁
                                    }
                                    break;
                                }
                            }
                            if (new_block != -1) {
                                break;
                            }
                        }
                        if (new_block == -1) {
                            // printf("The block is full!\n");
                            return "The block is full!\n";
                        }
                    }
                }
                printf("root 12:\n");
                
                // 更新文件内容
                for (j = 0; j < new_block_num; j++) {
                    struct FileBlock *fileBlock = (struct FileBlock *) &blockMem[inodeMem[n_inode].blockID[j]];
                    int offset = j * BLOCK_SIZE;
                    int bytes_to_copy = (content_size - offset < BLOCK_SIZE) ? content_size - offset : BLOCK_SIZE;
                    strncpy(fileBlock->content, content + offset, bytes_to_copy);
                    if (bytes_to_copy < BLOCK_SIZE) {
                        fileBlock->content[bytes_to_copy] = '\0';
                    }
                }
                printf("root 13:\n");
                
                {  
                    pthread_mutex_lock(&inodeLock);  // 加锁
                    inodeMem[n_inode].blockNum = new_block_num;
                    pthread_mutex_unlock(&inodeLock);  // 解锁
                }
                // printf("File content updated successfully!\n");
                printf("Server: %d blocks were written!\n", inodeMem[n_inode].blockNum);
                saveFileSystem();
                return "File content updated successfully!\n";
            } else {
                // printf("The specified path is not a file!\n");
                return "The specified path is not a file!\n";
            }
        }
    }
    // printf("The file does not exist!\n");
    return "The file does not exist!\n";
}

// 列出文件函数
void listFiles(char *path, int clientSocket) {
    pthread_mutex_lock(&blockLock_r);  // 加锁
    pthread_mutex_lock(&inodeLock_r);  // 加锁

    if(inodeLock_count == 0){
        pthread_mutex_lock(&inodeLock);  // 加锁
    }
    inodeLock_count++;
    if(blockLock_count == 0){
        pthread_mutex_lock(&blockLock);  // 加锁
    }
    blockLock_count++;
    pthread_mutex_unlock(&inodeLock_r);  // 解锁
    pthread_mutex_unlock(&blockLock_r);  // 解锁
    int i, flag;
    // char path[100];
    char *directory, *parent;
    const char delimiter[2] = "/";
    struct Inode *pointer = inodeMem;
    struct DirectoryBlock *block;

    // printf("Input the path:\n");
    // scanf("%s", path);
    printf("Server: The directory includes following files\n");
    sendMessage(clientSocket, "The directory includes following files\n");

    directory = strtok(path, delimiter);
    while (directory != NULL) {
        block = (struct DirectoryBlock *) &blockMem[pointer->blockID[0]];
        flag = 0;
        for (i = 0; i < ENTRY_NUMBER; i++) { 
            if (strcmp(block->fileName[i], directory) == 0) {
                flag = 1;
                pointer = &inodeMem[block->inodeID[i]];
                break;
            }
        }
        if (flag == 0 || pointer->fileType == 1) {
            // 如果父目录不存在或者不是目录文件
            printf("Server: The path does not exist!\n");
            sendMessage(clientSocket, "The path does not exist!\n");
            inodeLock_count--;
            blockLock_count--;
            if(inodeLock_count == 0){
                pthread_mutex_unlock(&inodeLock);  // 解锁
            }
            if(blockLock_count == 0){
                pthread_mutex_unlock(&blockLock);  // 解锁
            }
            return;
        }
        directory = strtok(NULL, delimiter); 
    }

    block = (struct DirectoryBlock *) &blockMem[pointer->blockID[0]];
    printf("INode\tisDir\tFile Name\n");
    sendMessage(clientSocket, "INode\tisDir\tFile Name\n");
    for (i = 0; i < ENTRY_NUMBER; i++) {
        if (block->inodeID[i] != -1) {
            printf("%d\t%d\t%s\n", block->inodeID[i], 1-inodeMem[block->inodeID[i]].fileType, block->fileName[i]);
            char message[256];
            sprintf(message, "%d\t%d\t%s\n", block->inodeID[i], 1-inodeMem[block->inodeID[i]].fileType, block->fileName[i]);
            sendMessage(clientSocket, message);
        }
    }
    inodeLock_count--;
    blockLock_count--;
    if(inodeLock_count == 0){
        pthread_mutex_unlock(&inodeLock);  // 解锁
    }
    if(blockLock_count == 0){
        pthread_mutex_unlock(&blockLock);  // 解锁
    }
    return ;
}

void listFiles_main(char *path) {
    int i, flag;
    // char path[100];
    char *directory, *parent;
    const char delimiter[2] = "/";
    struct Inode *pointer = inodeMem;
    struct DirectoryBlock *block;

    // printf("Input the path:\n");
    // scanf("%s", path);
    printf("Server: The directory includes following files\n");
    // sendMessage(clientSocket, "The directory includes following files\n");

    directory = strtok(path, delimiter);
    while (directory != NULL) {
        block = (struct DirectoryBlock *) &blockMem[pointer->blockID[0]];
        flag = 0;
        for (i = 0; i < ENTRY_NUMBER; i++) { 
            if (strcmp(block->fileName[i], directory) == 0) {
                flag = 1;
                pointer = &inodeMem[block->inodeID[i]];
                break;
            }
        }
        if (flag == 0 || pointer->fileType == 1) {
            // 如果父目录不存在或者不是目录文件
            printf("Server: The path does not exist!\n");
            return;
        }
        directory = strtok(NULL, delimiter); 
    }

    block = (struct DirectoryBlock *) &blockMem[pointer->blockID[0]];
    printf("INode\tisDir\tFile Name\n");
    for (i = 0; i < ENTRY_NUMBER; i++) {
        if (block->inodeID[i] != -1) {
            printf("%d\t%d\t%s\n", block->inodeID[i], 1-inodeMem[block->inodeID[i]].fileType, block->fileName[i]);
        }
    }
    return ;
}

char* moveDir(char *current_path, char *full_path, char *last_path, char *permission){
    strcpy(last_path, current_path);
    if (goToDirectory(full_path,permission) == 1){
        strcpy(current_path, full_path);
        if (current_path[strlen(current_path) - 1] != '/') {
            strcat(current_path, "/");
        }
        return "Directory changed successfully!\n";
    }else{
        return "Directory does not exist!\n";
    }
}


int checkUser(char *input_username, char *input_password, char *current_path, char *last_path, struct User *user){
    char content[MAX_BLCK_NUMBER_PER_FILE*BLOCK_SIZE];
    readFile("/pwd",content,"2");
    char *username, *password, *permission;
    char *saveptr1, *saveptr2;
    int login = 0;
    char *USER = strtok_r(content, "/", &saveptr1);
    while(USER != NULL && login == 0){
        username = strtok_r(USER, "-", &saveptr2);
        password = strtok_r(NULL, "-", &saveptr2);
        permission = strtok_r(NULL, "-", &saveptr2);
        
        if(strcmp(username, input_username) == 0 && strcmp(password, input_password) == 0){
            strcpy(user->username, username);
            strcpy(user->password, password);
            strcpy(user->permission, permission);
            printf("Server: username: %s\npassword: %s\npermission: %s\n", user->username, user->password, user->permission);
            login = 1;
        }
        USER = strtok_r(NULL, "/", &saveptr1);
    }
    if (login == 0){
        printf("Server: Login failed!\n");
        return 0;
    }else{
        char full_path[256] = "/home/";
        if(strcmp(username, "admin") != 0){
            strcat(full_path, username);
            moveDir(current_path, full_path, last_path, user->permission);
        }
        printf("Server: Login successfully!\n");
        return 1;
    }
}

int checkUserName(char *input_username){
    char content[MAX_BLCK_NUMBER_PER_FILE*BLOCK_SIZE];
    readFile("/pwd",content,"2");
    char *username, *password, *permission;
    char *saveptr1, *saveptr2;
    int login = 0;
    char *USER = strtok_r(content, "/", &saveptr1);
    while(USER != NULL && login == 0){
        username = strtok_r(USER, "-", &saveptr2);
        password = strtok_r(NULL, "-", &saveptr2);
        permission = strtok_r(NULL, "-", &saveptr2);
        
        if(strcmp(username, input_username) == 0){
            login = 1;
        }
        USER = strtok_r(NULL, "/", &saveptr1);
    }
    return login;
}

void checkDoctor(int UserPermission, char *doctor_name) {
    int group = UserPermission % 10;
    char content[MAX_BLCK_NUMBER_PER_FILE * BLOCK_SIZE];
    readFile("/pwd", content, "2");
    char *username, *password, *permission;
    char *saveptr1, *saveptr2;
    char *USER = strtok_r(content, "/", &saveptr1);

    doctor_name[0] = '\0'; // Initialize doctor_name for strcat
    int flag = 0;

    while (USER != NULL) {
        username = strtok_r(USER, "-", &saveptr2);
        password = strtok_r(NULL, "-", &saveptr2);
        permission = strtok_r(NULL, "-", &saveptr2);

        if (atoi(permission) / 10 == 1 && atoi(permission) % 10 == group) {
            flag++;
            strcat(doctor_name, username);
            strcat(doctor_name, " ");
        }
        USER = strtok_r(NULL, "/", &saveptr1);
    }

    if (flag == 0) {
        strcpy(doctor_name, "admin");
    }
}

char* delUser(char *User_name) {
    char content[MAX_BLCK_NUMBER_PER_FILE * BLOCK_SIZE];
    char *txt=readFile("/pwd", content, "2");
    char content_back[MAX_BLCK_NUMBER_PER_FILE * BLOCK_SIZE];
    content_back[0] = '\0';
    char *username, *password, *permission;
    char *saveptr1, *saveptr2;
    char *USER = strtok_r(content, "/", &saveptr1);
    int flag = 0;

    while (USER != NULL) {
        username = strtok_r(USER, "-", &saveptr2);
        password = strtok_r(NULL, "-", &saveptr2);
        permission = strtok_r(NULL, "-", &saveptr2);

        if(strcmp(username, User_name) != 0){
            strcat(content_back, username);
            strcat(content_back, "-");
            strcat(content_back, password);
            strcat(content_back, "-");
            strcat(content_back, permission);
            strcat(content_back, "/");
        }else{
            flag = 1;
        }
        USER = strtok_r(NULL, "/", &saveptr1);
    }
    writeFile("/pwd", content_back, "2",1);
    if(flag == 0){
        return "The user does not exist!\n";
    }else{
        return "The user has been deleted!\n";
    }
}

void listGroup(char* group){
    char content[MAX_BLCK_NUMBER_PER_FILE*BLOCK_SIZE];
    readFile("/pwd",content,"2");
    char *username, *password, *permission;
    char *saveptr1, *saveptr2;
    char *USER = strtok_r(content, "/", &saveptr1);
    group[0] = '\0';
    while(USER != NULL){
        username = strtok_r(USER, "-", &saveptr2);
        password = strtok_r(NULL, "-", &saveptr2);
        permission = strtok_r(NULL, "-", &saveptr2);
        int permission_int = atoi(permission);
        if(permission_int/10 == 1){
            strcat(group, username);
            strcat(group, ": ");
            strcat(group, &permission[1]);
            strcat(group, " ");
        }
        USER = strtok_r(NULL, "/", &saveptr1);
    }
}

void checkPatient(int UserPermission, char *patient_name) {
    int group = UserPermission % 10;
    char content[MAX_BLCK_NUMBER_PER_FILE * BLOCK_SIZE];
    readFile("/pwd", content, "2");
    char *username, *password, *permission;
    char *saveptr1, *saveptr2;
    char *USER = strtok_r(content, "/", &saveptr1);

    patient_name[0] = '\0'; // Initialize patient_name for strcat
    int flag = 0;

    while (USER != NULL) {
        username = strtok_r(USER, "-", &saveptr2);
        password = strtok_r(NULL, "-", &saveptr2);
        permission = strtok_r(NULL, "-", &saveptr2);

        if (atoi(permission) / 10 == 3 && atoi(permission) % 10 == group) {
            flag++;
            strcat(patient_name, username);
            strcat(patient_name, " ");
        }
        USER = strtok_r(NULL, "/", &saveptr1);
    }

    if (flag == 0) {
        strcpy(patient_name, "admin");
    }
}

// 修改文件名函数
char* renameFile(char *oldPath, char *newName, char *permission) {
    int permission_int = atoi(permission);
    if (permission_int / 10 == 3 && permission_int != 2) {
        return "You are patient, you can't rename a file!\n";
    }
    int i, flag;
    char *directory, *parent;
    const char delimiter[2] = "/";
    struct Inode *pointer = inodeMem;
    struct DirectoryBlock *block;

    // 递归访问父目录
    parent = NULL;
    directory = strtok(oldPath, delimiter);
    if (directory == NULL) {
        return "It is the root directory!\n";
    }
    while (directory != NULL) {
        if (parent != NULL) {
            block = (struct DirectoryBlock *) &blockMem[pointer->blockID[0]];
            flag = 0;
            for (i = 0; i < ENTRY_NUMBER; i++) {
                if (strcmp(block->fileName[i], parent) == 0) {
                    flag = 1;
                    pointer = &inodeMem[block->inodeID[i]];
                    break;
                }
            }
            if (flag == 0 || pointer->fileType == 1) {
                return "The path does not exist!\n";
            }
        }
        parent = directory;
        directory = strtok(NULL, delimiter);
    }

    // 修改文件名
    block = (struct DirectoryBlock *) &blockMem[pointer->blockID[0]];
    for (i = 0; i < ENTRY_NUMBER; i++) {
        if (strcmp(block->fileName[i], parent) == 0) {
            int n_inode = block->inodeID[i];
            if (inodeMem[n_inode].fileType == 1) {
                if (permission_int / 10 != inodeMem[n_inode].permission / 100 || permission_int % 10 != inodeMem[n_inode].permission % 10) {
                    return "You don't have permission to rename the file!\n";
                }
                strcpy(block->fileName[i], newName);
                saveFileSystem();
                return "File renamed successfully!\n";
            } else {
                return "The specified path is not a file!\n";
            }
        }
    }
    return "The file does not exist!\n";
}

// 修改目录名函数
char* renameDirectory(char *oldPath, char *newName, char *permission) {
    int permission_int = atoi(permission);
    if (permission_int / 10 == 3 && permission_int != 2) {
        return "You are patient, you can't rename a directory!\n";
    }
    int i, flag;
    char *directory, *parent;
    const char delimiter[2] = "/";
    struct Inode *pointer = inodeMem;
    struct DirectoryBlock *block;

    // 递归访问父目录
    parent = NULL;
    directory = strtok(oldPath, delimiter);
    if (directory == NULL) {
        return "It is the root directory!\n";
    }
    while (directory != NULL) {
        if (parent != NULL) {
            block = (struct DirectoryBlock *) &blockMem[pointer->blockID[0]];
            flag = 0;
            for (i = 0; i < ENTRY_NUMBER; i++) {
                if (strcmp(block->fileName[i], parent) == 0) {
                    flag = 1;
                    pointer = &inodeMem[block->inodeID[i]];
                    break;
                }
            }
            if (flag == 0 || pointer->fileType == 1) {
                return "The path does not exist!\n";
            }
        }
        parent = directory;
        directory = strtok(NULL, delimiter);
    }

    // 修改目录名
    block = (struct DirectoryBlock *) &blockMem[pointer->blockID[0]];
    for (i = 0; i < ENTRY_NUMBER; i++) {
        if (strcmp(block->fileName[i], parent) == 0) {
            int n_inode = block->inodeID[i];
            if (inodeMem[n_inode].fileType == 0) {
                if (permission_int / 10 != inodeMem[n_inode].permission / 100 || permission_int % 10 != inodeMem[n_inode].permission % 10) {
                    return "You don't have permission to rename the directory!\n";
                }
                strcpy(block->fileName[i], newName);
                saveFileSystem();
                return "Directory renamed successfully!\n";
            } else {
                return "The specified path is not a directory!\n";
            }
        }
    }
    return "The directory does not exist!\n";
}