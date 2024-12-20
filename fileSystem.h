#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#ifndef FILESYSTEM_H
#define FILESYSTEM_H
#define BLOCK_SIZE 512  // 定义每个块的大小
#define MAX_FILENAME_LENGTH 12  // 文件名最大长度
#define ENTRY_NUMBER 32  // 每个目录块的最大条目数
#define BLOCK_NUMBER 128  // 最大块数
#define INODE_NUMBER 128  // 最大i节点数
#define MAX_BLCK_NUMBER_PER_FILE 8  // 每个文件最大块数
#define MAX_FILE 512  // 最大文件数


// i节点结构体
struct Inode {
    int inodeNumber;  // i节点编号
    int blockID[MAX_BLCK_NUMBER_PER_FILE];  // 所属块编号
    int fileType;  // 文件类型：0 表示目录，1 表示普通文件
    int blockNum;
    int permission; // 权限
    int fileNum; // 文件数
};

struct FileLock{
    pthread_mutex_t lock;
    char filePath[1024];
    int lockNum;  
};
// 目录块结构体
struct DirectoryBlock {
    char fileName[ENTRY_NUMBER][MAX_FILENAME_LENGTH];  // 文件名列表
    int inodeID[ENTRY_NUMBER];  // 对应的i节点编号
};

// 文件块结构体
struct FileBlock {
    char content[BLOCK_SIZE];  // 文件内容
};

// 用户结构体
struct User {
    char username[256];
    char password[256];
    char permission[2];
};

// 全局变量
struct Inode inodeMem[INODE_NUMBER];  // i节点数组，存储所有文件的iNode
struct FileBlock blockMem[BLOCK_NUMBER];  // 块数组
char blockBitmap[BLOCK_NUMBER/8];  // 块的位图
struct FileLock fileLock[MAX_FILE];  // 文件锁



// 将文件系统信息保存到硬盘
char* saveFileSystem();

// 从硬盘读取文件系统信息
char* loadFileSystem();

// 创建文件函数（修改了块分配和iNode的大小最依据文件的实际需求）
char* createFile(char *path, int file_permission, char* content);

// 删除文件函数
char* deleteFile(char *path, char *permission);

// 读取文件函数
char* readFile(char *path, char *content_back, char *permission);

// 写入文件函数
char* writeFile(char *path, char *content, char *permission, int flag);

// 移动目录
char* moveDir(char *current_path, char *full_path, char *last_path, char *permission);

// 删除目录
char* deleteDirectory(char *path, char *permission);

// 创建目录
char* createDirectory(char *path, char *permission);

// 列出文件
void listFiles(char *path, int clientSocket);

void listFiles_main(char *path);

// 进入目录
_Bool goToDirectory(char *path_origin, char *permission);


int checkUser(char *input_username, char *input_password, char *current_path, char *last_path, struct User *user);

void sendMessage(int clientSocket, const char* message);

void receiveInput(int clientSocket, char* buffer, size_t size);

int checkUserName(char *input_username);

char* linkPath(char *path, char *path_origin, char *permission);

void checkDoctor(int UserPermission, char *doctor_name);

void checkPatient(int UserPermission, char *patient_name);

void listGroup(char* group);

char* delUser(char *User_name);

// 修改文件名
char* renameFile(char *oldPath, char *newName, char *permission);

// 修改目录名
char* renameDirectory(char *oldPath, char *newName, char *permission);

// 复制文件
char* copyFile(char *srcPath, char *destPath, char *permission, int clientSocket);

// 移动文件
char* moveFile(char *srcPath, char *destPath, char *permission, int clientSocket);

// 文件权限查找
char* findFilePermission(char *path);

#endif