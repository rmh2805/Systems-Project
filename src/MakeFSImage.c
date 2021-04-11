#include "common.h"

// avoid complaints about stdio.h
#undef NULL

#include <stdio.h>

size_t diskSize, nInodes;
char * oFileName;
char * pgmName;

void printUsage(bool_t isErr) {
    fprintf((isErr) ? stderr : stdout, "Usage: %s <output file> <disk size> <nInodes>\n", pgmName);
}

int decStr2int(char * str) {
    int val = 0, sign = 1;
    
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
    
    diskSize = decStr2int(argv[2]);
    nInodes = decStr2int(argv[3]);
    
    if(diskSize == 0 || nInodes == 0 || diskSize < nInodes * sizeof(inode_t)) {
        fprintf(stderr, "Disk size (%lu bytes) not enough to store %lu inodes (need at least %lu bytes)\n", diskSize, nInodes, nInodes * sizeof(inode_t));
        return -1;
    }
    
    
    return 0;
}
