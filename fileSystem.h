#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef FILESYSTEM_H
#define FILESYSTEM_H
#define BLOCK_SIZE 512  // 定义每个块的大小
#define MAX_FILENAME_LENGTH 12  // 文件名最大长度
#define ENTRY_NUMBER 32  // 每个目录块的最大条目数
#define BLOCK_NUMBER 128  // 最大块数
#define INODE_NUMBER 128  // 最大i节点数
#define MAX_BLCK_NUMBER_PER_FILE 8  // 每个文件最大块数


// i节点结构体
struct Inode {
    int inodeNumber;  // i节点编号
    int blockID[MAX_BLCK_NUMBER_PER_FILE];  // 所属块编号
    int fileType;  // 文件类型：0 表示目录，1 表示普通文件
    int blockNum;
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

// 全局变量
struct Inode inodeMem[INODE_NUMBER];  // i节点数组，存储所有文件的iNode
struct FileBlock blockMem[BLOCK_NUMBER];  // 块数组
char blockBitmap[BLOCK_NUMBER/8];  // 块的位图

// 将文件系统信息保存到硬盘
void saveFileSystem();

// 从硬盘读取文件系统信息
void loadFileSystem();

// 创建文件函数（修改了块分配和iNode的大小最依据文件的实际需求）
void createFile(char *path);

// 删除文件函数
void deleteFile(char *path);

// 读取文件函数
void readFile(char *path, char *content_back);

// 写入文件函数
void writeFile(char *path, char *content);

// 移动目录
void moveDir(char *current_path, char *full_path, char *last_path);

// 删除目录
void deleteDirectory(char *path);

// 创建目录
void createDirectory(char *path);

// 列出文件
void listFiles(char *path);

// 进入目录
_Bool goToDirectory(char *path_origin);

#endif