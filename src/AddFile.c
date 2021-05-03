#include "common.h"

// avoid complaints about stdio.h
#undef NULL

#include <stdio.h>

#define INODES_PER_BLOCK ((int32_t)(BLOCK_SIZE / sizeof(inode_t)))

#define BLOCKBUF_SIZE BLOCK_SIZE + 1

char* callPath;
FILE* baseFile;
FILE* newFile;
FILE* outFile;
char blockBuf[BLOCKBUF_SIZE];

void printUsage() {
    printf("*Usage*: %s <Base FS File> <File to insert> <Output File>\n", callPath);
}

void cleanup() {
    if(baseFile != NULL) fclose(baseFile);
    if(newFile  != NULL) fclose(newFile);
    if(outFile  != NULL) fclose(outFile);
}

int loadBlock(FILE* file, uint32_t idx) {
    
}

int main(int argc, char** argv) {
    callPath = argv[0];
    
    if(argc != 4) {
        printUsage();
        return E_FAILURE;
    }

    if(strcmp(argv[1], argv[2]) == 0 || strcmp(argv[1], argv[3]) == 0 || strcmp(argv[2], argv[3]) == 0) {
        fprintf(stderr, "*ERROR* All three files must be different\n");
        return E_FAILURE;
    } 
    
    // Open all of the file streams
    baseFile = fopen(argv[1], "rb");    // Open the basic FS image to read bytes
    if(baseFile == NULL) {
        cleanup();
        fprintf(stderr, "*ERROR* Failed to open the base image file\n");
        return E_FAILURE;
    }
    
    newFile = fopen(argv[2], "rb");     // Open the new file to read bytes
    if(newFile == NULL) {
        cleanup();
        fprintf(stderr, "*ERROR* Failed to open the new file\n");
        return E_FAILURE;
    }
    
    outFile = fopen(argv[3], "w+b");    // Open the new file for read and write
    if(outFile == NULL) {
        cleanup();
        fprintf(stderr, "*ERROR* Failed to open the output image file\n");
        return E_FAILURE;
    }
    
    // Copy contents of file to output filei
    while(fgets(blockBuf, BLOCKBUF_SIZE, baseFile) != NULL) {
        if(fputs(blockBuf, outFile) < 0) {
            cleanup();
            fprintf(stderr, "*ERROR* Failed to copy across a block\n");
            return E_FAILURE;
        }
    }

    cleanup();
    return E_SUCCESS;
}
