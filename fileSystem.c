#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fileSystem.h"


// struct Inode *current_pointer;  // 当前目录的i节点指针
// 将文件系统信息保存到硬盘
void saveFileSystem() {
    FILE *file = fopen("filesystem", "wb");
    if (file == NULL) {
        printf("Error: Unable to save filesystem data.\n");
        return;
    }
    fwrite(inodeMem, sizeof(struct Inode), INODE_NUMBER, file);
    fwrite(blockMem, sizeof(struct DirectoryBlock), BLOCK_NUMBER, file);
    fwrite(blockBitmap, sizeof(blockBitmap), 1, file);
    fclose(file);
    printf("Filesystem data saved successfully.\n");
}

// 从硬盘读取文件系统信息
void loadFileSystem() {
    FILE *file = fopen("filesystem", "rb");
    if (file == NULL) {
        printf("Error: Unable to load filesystem data.\n");
        return;
    }
    fread(inodeMem, sizeof(struct Inode), INODE_NUMBER, file);
    fread(blockMem, sizeof(struct DirectoryBlock), BLOCK_NUMBER, file);
    fread(blockBitmap, sizeof(blockBitmap), 1, file);
    fclose(file);
    printf("Filesystem data loaded successfully.\n");
}

// 前往目录函数
_Bool goToDirectory(char *path_origin) {
    char path[256];
    strcpy(path, path_origin);
    int i, flag;
    char *directory, *parent;
    const char delimiter[2] = "/";
    struct Inode *pointer = inodeMem;
    struct DirectoryBlock *block;

    // 递归访问父目录
    parent = NULL;
    directory = strtok(path, delimiter);
    if (directory == NULL) {
        printf("It is the root directory!\n");
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
                printf("The path does not exist!\n");
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
                printf("Directory changed successfully!\n");
                return 1;
            } else {
                printf("The specified path is not a directory!\n");
                return 0;
            }
        }
    }
    printf("The directory does not exist!\n");
    return 0;
}
// 创建目录函数
void createDirectory(char *path) {
    int i, j, flag;
    char *directory, *parent, *target;
    const char delimiter[2] = "/"; // 分隔符
    struct Inode *pointer = inodeMem; // 指向当前目录的i节点
    struct DirectoryBlock *block;

    // printf("Input the path:\n");
    // fgets(path, sizeof(path), stdin);
    // path[strcspn(path, "\n")] = '\0';  // 去掉行进符

    // 递归访问父目录
    parent = NULL;
    directory = strtok(path, delimiter);
    if (directory == NULL) {
        printf("It is the root directory!\n");
        return;
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
                printf("The path does not exist!\n");
                return;
            }
        }
        parent = directory;
        directory = strtok(NULL, delimiter);
    }

    // 创建目标目录
    int entry = -1, n_block = -1, n_inode = -1; // entry: 目录块中的条目号；n_block: 新目录块编号；n_inode: 新目录i节点编号
    target = parent;
    block = (struct DirectoryBlock *) &blockMem[pointer->blockID[0]]; //由于 blockMem 是一个 FileBlock 数组，直接访问时无法以 DirectoryBlock 方式来处理。但是在文件系统中，目录块和文件块都存储在相同的 blockMem 数组中。
    for (i = 0; i < ENTRY_NUMBER; i++) {
        if (strcmp(block->fileName[i], target) == 0) {
            printf("The directory already exists!\n");
            return;
        }
        if (block->inodeID[i] == -1) {
            entry = i;
            break;
        }
    }
    if (entry >= 0) {
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
            printf("The block is full!\n");
            return;
        }

        // 查找未使用的i节点
        flag = 0;
        for (i = 0; i < INODE_NUMBER; i++) {
            if (inodeMem[i].blockID[0] == -1) {
                flag = 1;
                inodeMem[i].blockID[0] = n_block;
                inodeMem[i].fileType = 0;
                n_inode = i;
                break;
            }
        }
        if (n_inode == -1) {
            printf("The inode is full!\n");
            return;
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

        printf("Directory created successfully!\n");
    }
    else {
        printf("The directory is full!\n");
    }
    return;
}

// 删除目录函数
void deleteDirectory(char *path) {
    int i, flag;
    char *directory, *parent;
    const char delimiter[2] = "/";
    struct Inode *pointer = inodeMem;
    struct DirectoryBlock *block;

    // printf("Input the path:\n");
    // fgets(path, sizeof(path), stdin);
    // path[strcspn(path, "\n")] = '\0';  // 去掉行进符

    // 递归访问父目录
    parent = NULL;
    directory = strtok(path, delimiter);
    if (directory == NULL) {
        printf("It is the root directory!\n");
        return;
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
                printf("The path does not exist!\n");
                return;
            }
        }
        parent = directory;
        directory = strtok(NULL, delimiter);
    }

    // 删除目标目录
    block = (struct DirectoryBlock *) &blockMem[pointer->blockID[0]];
    for (i = 0; i < ENTRY_NUMBER; i++) {
        if (strcmp(block->fileName[i], parent) == 0) {
            int n_inode = block->inodeID[i];
            if (inodeMem[n_inode].fileType == 0) { // 检查是否为目录文件
                struct DirectoryBlock *targetBlock = (struct DirectoryBlock *) &blockMem[inodeMem[n_inode].blockID[0]];
                // 检查目录是否为空
                for (int j = 0; j < ENTRY_NUMBER; j++) {
                    if (targetBlock->inodeID[j] != -1) {
                        printf("The directory is not empty!\n");
                        return;
                    }
                }
                // 删除目录
                block->inodeID[i] = -1;
                
                blockBitmap[inodeMem[n_inode].blockID[0] / 8] &= ~(1 << (inodeMem[n_inode].blockID[0] % 8)); 
                inodeMem[n_inode].blockID[0] = -1;
                printf("Directory deleted successfully!\n");
                return;
            } else {
                printf("The specified path is not a directory!\n");
                return;
            }
        }
    }
    printf("The directory does not exist!\n");
    return;
}

// 创建文件函数（修改了块分配和iNode的大小最依据文件的实际需求）
void createFile(char *path) {
    int i, j, flag;
    char content[BLOCK_SIZE * MAX_BLCK_NUMBER_PER_FILE];
    char *directory, *parent, *target;
    const char delimiter[2] = "/";
    struct Inode *pointer = inodeMem;
    struct DirectoryBlock *block;

    // printf("Input the path:\n");
    // fgets(path, sizeof(path), stdin);
    // path[strcspn(path, "\n")] = '\0';  // 去掉行进符
    if(strcmp(path,"/pwd") != 0) printf("Input the content:\n");
    else printf("Input the username and password(username-password/):\n");
    fgets(content, sizeof(content), stdin);
    content[strcspn(content, "\n")] = '\0';
    if(strcmp(path, "/pwd") == 0){
        if (strlen(content) == 0) {
            printf("The content is empty!\n");
            createFile(path);
            return;
        }else{
            char delimiter[2] = "/";
            strcat(content, delimiter);
        }
    }

    // 递序访问父目录
    parent = NULL;
    directory = strtok(path, delimiter);
    if (directory == NULL) {
        printf("It is the root directory!\n");
        return;
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
                printf("The path does not exist or is not a directory!\n");
                return;
            }
        }
        parent = directory;
        directory = strtok(NULL, delimiter);
    }

    // 创建目标文件
    int entry = -1, n_inode = -1;
    int N_block[MAX_BLCK_NUMBER_PER_FILE] = {-1};
    target = parent;
    block = (struct DirectoryBlock *) &blockMem[pointer->blockID[0]];
    for (i = 0; i < ENTRY_NUMBER; i++) { // 查找目录块中的空闲条目
        if (strcmp(block->fileName[i], target) == 0) {
            printf("The file already exists!\n");
            return;
        }
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
            printf("The file is too large!\n");
            return;
        }

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
            printf("The block is not enough for a file!\n");
            return;
        }

        // 查找未使用的i节点
        for (i = 0; i < INODE_NUMBER; i++) {
            if (inodeMem[i].blockID[0] == -1) {
                for (int k = 0; k < block_num; k++) {
                    inodeMem[i].blockID[k] = N_block[k];
                }
                inodeMem[i].fileType = 1;
                inodeMem[i].blockNum = block_num;
                n_inode = i;
                break;
            }
        }
        if (n_inode == -1) {
            printf("The inode is full!\n");
            return;
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
        printf("File created successfully!\n");
    } else {
        printf("The directory is full!\n");
    }
    return;
}


// 删除文件函数
void deleteFile(char *path) {
    int i, flag;
    char *directory, *parent;
    const char delimiter[2] = "/";
    struct Inode *pointer = inodeMem;
    struct DirectoryBlock *block;

    // printf("Input the path:\n");
    // fgets(path, sizeof(path), stdin);
    // path[strcspn(path, "\n")] = '\0';  // 去掉行进符

    // 递归访问父目录
    parent = NULL;
    directory = strtok(path, delimiter);
    if (directory == NULL) {
        printf("It is the root directory!\n");
        return;
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
                printf("The path does not exist!\n");
                return;
            }
        }
        parent = directory;
        directory = strtok(NULL, delimiter);
    }

    // 删除目标文件
    block = (struct DirectoryBlock *) &blockMem[pointer->blockID[0]];
    for (i = 0; i < ENTRY_NUMBER; i++) {
        if (strcmp(block->fileName[i], parent) == 0) {
            int n_inode = block->inodeID[i];
            if (inodeMem[n_inode].fileType == 1) {
                // 删除文件
                block->inodeID[i] = -1;
                for(int j = 0; j<MAX_BLCK_NUMBER_PER_FILE; j++){ //修改
                    blockBitmap[inodeMem[n_inode].blockID[j] / 8] &= ~(1 << (inodeMem[n_inode].blockID[j] % 8));
                    inodeMem[n_inode].blockID[j] = -1;
                }
                
                printf("File deleted successfully!\n");
                return;
            } else {
                printf("The specified path is not a file!\n");
                return;
            }
        }
    }
    printf("The file does not exist!\n");
    return;
}

// 读取文件函数
void readFile(char *path, char *content_back) {
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
        printf("It is the root directory!\n");
        return;
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
                printf("The path does not exist!\n");
                return;
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
                return;
            } else {
                printf("The specified path is not a file!\n");
                return;
            }
        }
    }
    printf("The file does not exist!\n");
    return;
}

// 写入文件函数（动态增加或减少块数量）
void writeFile(char *path, char *content) {
    int i, j, flag;
    
    char *directory, *parent;
    const char delimiter[2] = "/";
    struct Inode *pointer = inodeMem;
    struct DirectoryBlock *block;
    

    // printf("Input the path:\n");
    // fgets(path, sizeof(path), stdin);
    // path[strcspn(path, "\n")] = '\0';  // 去掉行进符
    
    size_t content_size = strlen(content);
    if(strcmp(path, "/pwd") == 0){
        if (strlen(content) == 0) {
            printf("The content is empty!\n");
            createFile(path);
            return;
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

            if (strcmp(full_path, "/home/root") != 0) {
                createDirectory(home);
                createDirectory(full_path);
            }

            USER = strtok_r(NULL, "/", &saveptr1);
        }
        strcpy(content, content_temp);
    }
    
    // 递归访问父目录
    parent = NULL;
    directory = strtok(path, delimiter);
    if (directory == NULL) {
        printf("It is the root directory!\n");
        return;
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
                printf("The path does not exist or is not a directory!\n");
                return;
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
                    printf("The file is too large!\n");
                    return;
                }

                // 释放多余的块
                if (new_block_num < old_block_num) {
                    for (j = new_block_num; j < old_block_num; j++) {
                        blockBitmap[inodeMem[n_inode].blockID[j] / 8] &= ~(1 << (inodeMem[n_inode].blockID[j] % 8));
                        inodeMem[n_inode].blockID[j] = -1;
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
                                    inodeMem[n_inode].blockID[j] = new_block;
                                    break;
                                }
                            }
                            if (new_block != -1) {
                                break;
                            }
                        }
                        if (new_block == -1) {
                            printf("The block is full!\n");
                            return;
                        }
                    }
                }

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

                inodeMem[n_inode].blockNum = new_block_num;
                printf("File content updated successfully!\n");
                printf("%d blocks were written!\n", inodeMem[n_inode].blockNum);
                return;
            } else {
                printf("The specified path is not a file!\n");
                return;
            }
        }
    }
    printf("The file does not exist!\n");
    return;
}

// 列出文件函数
void listFiles(char *path) {
    int i, flag;
    // char path[100];
    char *directory, *parent;
    const char delimiter[2] = "/";
    struct Inode *pointer = inodeMem;
    struct DirectoryBlock *block;

    // printf("Input the path:\n");
    // scanf("%s", path);
    printf("The directory includes following files\n");

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
            printf("The path does not exist!\n");
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
    return;
}

void moveDir(char *current_path, char *full_path, char *last_path){
    strcpy(last_path, current_path);
    if (goToDirectory(full_path) == 1){
        strcpy(current_path, full_path);
        if (current_path[strlen(current_path) - 1] != '/') {
            strcat(current_path, "/");
        }
    }
}