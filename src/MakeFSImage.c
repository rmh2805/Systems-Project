#include "common.h"

// avoid complaints about stdio.h
#undef NULL

#include <stdio.h>

#define INODES_PER_BLOCK ((int32_t)(BLOCK_SIZE / sizeof(inode_t)))

uint32_t diskSize, nInodes;
char * oFileName;
char * pgmName;

void printUsage(bool_t isErr) {
    fprintf((isErr) ? stderr : stdout, "Usage: %s <output file> <disk size (kiB)> <nInodes>\n", pgmName);
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

int main(int argc, char** argv) {
        pgmName = argv[ 0 ];
    
    // Parse arguments
    if(argc != 4) {
        printUsage(true);
        return -1;
    }
    
    oFileName = argv[1];
    
    FILE * outF = fopen(oFileName, "wb");
    if(outF == (FILE *) -1) {
        fprintf(stderr, "Failed to open file %s", oFileName);
        exit(-1);
    }

    diskSize = decStr2int(argv[2]) * 1024;
    nInodes = decStr2int(argv[3]);
    
    if(diskSize == 0 || nInodes == 0 || diskSize < nInodes * sizeof(inode_t)) {
        fprintf(stderr, "Disk size (%u bytes) not enough to store %u inodes (need at least %u bytes)\n", 
                diskSize, nInodes, nInodes * sizeof(inode_t));
        return -1;
    }

    //Calculate regions on the disk
    uint32_t inodeBlocks = nInodes / INODES_PER_BLOCK;
    if(nInodes % INODES_PER_BLOCK != 0) {
        ++inodeBlocks;
    }

    uint32_t diskBlocks = diskSize/BLOCK_SIZE;
    if(diskSize % BLOCK_SIZE != 0) {
        ++diskBlocks;
    }

    //simple solution, just increment map blocks until you have enough 
    uint32_t mapBlocks = 0, dataBlocks = diskBlocks - inodeBlocks; 
    while(mapBlocks * BLOCK_SIZE * 8 < dataBlocks) {
        ++mapBlocks;
        --dataBlocks;
    }

    uint32_t excessMap = (mapBlocks * BLOCK_SIZE * 8) - dataBlocks;

    //For now, print stats out for debugging
    printf("disk blocks:  %u\n", diskBlocks);
    printf("inode blocks: %u\n", inodeBlocks);
    printf("map blocks:   %u\n", mapBlocks);
    printf("data blocks:  %u\n", dataBlocks);
    printf("excess map:   %u\n", excessMap);
    
    
    fclose(outF);
    return 0;
}
