#include <stdio.h>
#include <string.h>

#define TOTAL_BLOCKS 256
#define PAGES_PER_BLOCK 16
#define PAGE_SIZE 4096

typedef struct {
    char data[PAGE_SIZE];
} Page;

typedef struct {
    Page pages[PAGES_PER_BLOCK];
    int valid[PAGES_PER_BLOCK]; //标记每个page是否有效
} Block;

Block flash[TOTAL_BLOCKS];
int L2P[TOTAL_BLOCKS]; // 逻辑块号到物理块号的映射
int P2L[TOTAL_BLOCKS]; // 物理块号到逻辑块号的映射

void initFTL() { // 初始化FTL
    for (int i = 0; i < TOTAL_BLOCKS; i++) {
        L2P[i] = -1;
        P2L[i] = -1;
        for (int j = 0; j < PAGES_PER_BLOCK; j++) {
            flash[i].valid[j] = 0;
        }
    }
}

int allocateBlock() { // 分配一个新的物理块
    for (int i = 0; i < TOTAL_BLOCKS; i++) {
        if (L2P[i] == -1 && P2L[i] == -1) {
            return i;
        }
    }
    return -1;
}

int invalidPagesInBlock(int block) { // 返回一个物理块中无效页的数量
    int count = 0;
    for (int j = 0; j < PAGES_PER_BLOCK; j++) {
        if (flash[block].valid[j] == 0) {
            count++;
        }
    }
    return count;
}

void garbageCollector() {
    int targetBlock = -1;
    int maxInvalidPages = -1;

    for (int i = 0; i < TOTAL_BLOCKS; i++) {
        int currentInvalidPages = invalidPagesInBlock(i);
        if (currentInvalidPages > maxInvalidPages) {
            targetBlock = i;
            maxInvalidPages = currentInvalidPages;
        }
    }

    if (targetBlock != -1) {
        eraseBlock(P2L[targetBlock]);
    }
}

void readPage(int logicalBlock, int page, char *buffer) {
    int physicalBlock = L2P[logicalBlock];
    if (physicalBlock != -1 && flash[physicalBlock].valid[page]) {
        memcpy(buffer, flash[physicalBlock].pages[page].data, PAGE_SIZE);
    }
}

void writePage(int logicalBlock, int page, char *buffer) {
    int physicalBlock = L2P[logicalBlock];
    if (physicalBlock == -1) {
        physicalBlock = allocateBlock();
        if (physicalBlock == -1) {
            garbageCollector();
            physicalBlock = allocateBlock();
        }
        L2P[logicalBlock] = physicalBlock;
        P2L[physicalBlock] = logicalBlock;
    }

    if (flash[physicalBlock].valid[page] == 0) {
        memcpy(flash[physicalBlock].pages[page].data, buffer, PAGE_SIZE);
        flash[physicalBlock].valid[page] = 1;
    } else {
        int newPhysicalBlock = allocateBlock();
        if (newPhysicalBlock == -1) {
            garbageCollector();
            newPhysicalBlock = allocateBlock();
        }
        for (int j = 0; j < PAGES_PER_BLOCK; j++) {
            if (flash[physicalBlock].valid[j]) {
                memcpy(flash[newPhysicalBlock].pages[j].data, flash[physicalBlock].pages[j].data, PAGE_SIZE);
                flash[newPhysicalBlock].valid[j] = 1;
            }
        }
        eraseBlock(logicalBlock);
        L2P[logicalBlock] = newPhysicalBlock;
        P2L[newPhysicalBlock] = logicalBlock;

        memcpy(flash[newPhysicalBlock].pages[page].data, buffer, PAGE_SIZE);
        flash[newPhysicalBlock].valid[page] = 1;
    }
}

void eraseBlock(int logicalBlock) {
    int physicalBlock = L2P[logicalBlock];
    if (physicalBlock != -1) {
        for (int j = 0; j < PAGES_PER_BLOCK; j++) {
            memset(flash[physicalBlock].pages[j].data, 0, PAGE_SIZE);
            flash[physicalBlock].valid[j] = 0;
        }
        L2P[logicalBlock] = -1;
        P2L[physicalBlock] = -1;
    }
}

int main() {
    initFTL();

    char buffer[PAGE_SIZE] = "Hello";
    writePage(10, 5, buffer);
    char readBuffer[PAGE_SIZE];
    readPage(10, 5, readBuffer);
    printf("Read data: %s\n", readBuffer);

    return 0;
}
