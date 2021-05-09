#define ULIB_H_
#include "common.h"

// avoid complaints about stdio.h
#undef NULL

#include <stdio.h>

#define INODES_PER_BLOCK ((int32_t)(BLOCK_SIZE / sizeof(inode_t)))

uint32_t diskSize, nInodes, devId;
char * oFileName;
char * pgmName;

void printUsage(bool_t isErr) {
    fprintf((isErr) ? stderr : stdout, "Usage: %s <output file> <disk size (kiB)> <nInodes> <device ID> [<dev 1 name> <dev 1 id> [...]]\n", pgmName);
}

int32_t decStr2int(char * str) {
    uint32_t val = 0, sign = 1;
    
    if(*str == '-') {
        sign = -1;
        ++str;
    }
    
    while(*str >= '0' && *str <= '9') {
        val = val * 10 + *str++ - '0';
    }
    
    return val * sign;
}

int strncpy(char * dst, char * src, int n) {
    int i;
    for(i = 0; i < n && src[i]; i++) {
        dst[i] = src[i];
    }
    for(; i < n; i++) {
        dst[i] = 0;
    }
}

int main(int argc, char** argv) {
    pgmName = argv[ 0 ];
    
    if(INODES_PER_BLOCK < 1) {
        fprintf(stderr, "ERROR: less than 1 inode per block\n");
        return -1;
    }
    
    // Parse arguments
    if(argc < 5 || argc % 2 != 1) {
        printUsage(true);
        return -1;
    }
    
    oFileName = argv[1];
    
    FILE * outF = fopen(oFileName, "wb");
    if(outF == (FILE *) -1) {
        fprintf(stderr, "Failed to open file %s\n", oFileName);
        return (-1);
    }

    diskSize = decStr2int(argv[2]) * 1024;
    nInodes = decStr2int(argv[3]);
    devId = decStr2int(argv[4]);
    
    if(diskSize == 0 || nInodes < 2 || diskSize < nInodes * sizeof(inode_t)) {
        fprintf(stderr, "Disk size (%u bytes) not enough to store %u inodes (need at least %u bytes)\n", 
                diskSize, nInodes, nInodes * sizeof(inode_t));
        return -1;
    }

    // Calculate regions on the disk
    uint32_t inodeBlocks = nInodes / INODES_PER_BLOCK;
    if(nInodes % INODES_PER_BLOCK != 0) {
        ++inodeBlocks;
    }

    uint32_t diskBlocks = diskSize/BLOCK_SIZE;
    if(diskSize % BLOCK_SIZE != 0) {
        ++diskBlocks;
    }

    // Simple solution, just increment map blocks until you have enough 
    uint32_t mapBlocks = 0, dataBlocks = diskBlocks - inodeBlocks; 
    while(mapBlocks * BLOCK_SIZE * 8 < dataBlocks) {
        ++mapBlocks;
        --dataBlocks;
    }

    uint32_t excessMap = (mapBlocks * BLOCK_SIZE * 8) - dataBlocks;

    // For now, print stats out for debugging
    printf("disk blocks:     %u\n", diskBlocks);
    printf("inode blocks:    %u\n", inodeBlocks);
    printf("map blocks:      %u\n", mapBlocks);
    printf("data blocks:     %u\n", dataBlocks);
    printf("excess map bits: %u\n", excessMap);
    
    // Create the metadata inode
    inode_t metaNode;
    
    for(size_t i = 0; i < sizeof(metaNode); i++) {
        ((char*) &metaNode)[i] = 0;
    }
    
    metaNode.id = (inode_id_t) {devId, 0};
    metaNode.nBlocks = mapBlocks;
    metaNode.nodeType = INODE_META_TYPE;
    metaNode.nBytes = diskSize;
    metaNode.nRefs = nInodes;
    
    // Create device root inode
    inode_t rootNode;
    
    for(size_t i = 0; i < sizeof(rootNode); i++) {
        ((char*) &rootNode)[i] = 0;
    }
    
    rootNode.id = (inode_id_t) {devId, 1};
    rootNode.nBlocks = 0;
    rootNode.nBytes = 1;
    rootNode.nRefs = 1;
    rootNode.nodeType = INODE_DIR_TYPE;
    rootNode.permissions = 0x3F;
    strncpy(rootNode.direct_pointers[0].dir.name, "..", 12);
    rootNode.direct_pointers[0].dir.inode = rootNode.id;

    for(int i = 1; i < NUM_DIRECT_POINTERS && 4 + (i * 2) < argc; i++) {
        strncpy(rootNode.direct_pointers[i].dir.name, argv[2 * i], 12);
        rootNode.direct_pointers[i].dir.inode = (inode_id_t){decStr2int(argv[2 * i + 1]), 1};
    }
    
    // Write out the inode array
    uint8_t blockBuf[BLOCK_SIZE];
    for(size_t i = 0; i < BLOCK_SIZE; i++) {
        blockBuf[i] = 0;
    }
    
    for(size_t i = 0; i < sizeof(metaNode); i++) {
        blockBuf[i] = ((char *) &metaNode)[i];
    }
    
    if(INODES_PER_BLOCK > 1) {
        for(size_t i = 0; i < sizeof(rootNode); i++) {
            blockBuf[sizeof(metaNode) + i] = ((char *) &rootNode)[i];
        }
    }
    
    fwrite(blockBuf, BLOCK_SIZE, 1, outF);
    
    if(INODES_PER_BLOCK == 1) {
        for(size_t i = 0; i < BLOCK_SIZE; i++) {
            blockBuf[i] = 0;
        }
    
        for(size_t i = 0; i < sizeof(rootNode); i++) {
            blockBuf[i] = ((char *) &rootNode)[i];
        }
        
        fwrite(blockBuf, BLOCK_SIZE, 1, outF);
    }
    
    for(size_t i = 0; i < BLOCK_SIZE; i++) {
        blockBuf[i] = 0;
    }
    
    for(size_t i = (INODES_PER_BLOCK == 1) ? 2 : 1; i < inodeBlocks; i++) {
        fwrite(blockBuf, BLOCK_SIZE, 1, outF);
    }
    
    // Write out the bitmap
    for(size_t i = 0; i < mapBlocks - 1; i++) {
        fwrite(blockBuf, BLOCK_SIZE, 1, outF);
    }
    
    for(size_t off = 0; off < excessMap; off++) {
        uint32_t mapIdx = BLOCK_SIZE - (off/8 + 1);
        uint32_t bit = off % 8;
        blockBuf[mapIdx] |= 1 << bit;
    }
        
    fwrite(blockBuf, BLOCK_SIZE, 1, outF);
    
    for(size_t i = 0; i < BLOCK_SIZE; i++) {
        blockBuf[i] = 0;
    }
    
    // Null out the data blocks
    for(size_t i = 0; i < dataBlocks; i++) {
        fwrite(blockBuf, BLOCK_SIZE, 1, outF);
    }
    
    fclose(outF);
    return 0;
}
