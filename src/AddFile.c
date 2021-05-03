#include "common.h"

// avoid complaints about stdio.h
#undef NULL

#include <stdio.h>

#define INODES_PER_BLOCK ((int32_t)(BLOCK_SIZE / sizeof(inode_t)))

#define FILE_BUF_SIZE BLOCK_SIZE * 64  // Max size of fs image that this can modify

// Files
FILE* baseFile;
FILE* newFile;
FILE* outFile;

// String variables
char* callPath;

bool_t fileRead = false;
char fileBuf[FILE_BUF_SIZE];
long baseLen = FILE_BUF_SIZE;

// inode variables
bool_t initMeta = false;
inode_t metaNode;

void printUsage() {
    printf("*Usage*: %s <Base FS File> <File to insert> <Output File>\n", callPath);
}

long loadFile(FILE* src, long loadPt) {
    long idx;
    int ch;
    for(idx = loadPt; (ch = fgetc(src)) != EOF && idx < baseLen; idx++) {
        fileBuf[idx] = ch;
    }
    
    return idx - loadPt;
}

long writeFile(FILE* dst) {
    for(int i = 0; i < baseLen; i++) {
        fputc(fileBuf[i], dst);
    }
    return E_SUCCESS;
}

long loadBlock(FILE* src, long dstBlock, long srcBlock) {
    if(!fileRead) {
        fprintf(stderr, "*ERROR* in loadBlock: file buffer not initialized");
        return E_FAILURE;
    }
    
    // Ensure that we're loading from a location in the file
    fseek(src, 0, SEEK_END);
    long srcSize = ftell(src);
    
    long srcOffset = srcBlock * BLOCK_SIZE;
    if(srcOffset > srcSize) {
        return E_FAILURE;
    }
    
    // Ensure that the load point is within the base length 
    long dstOffset = dstBlock * BLOCK_SIZE;
    if(dstOffset + BLOCK_SIZE > baseLen) {
        fprintf(stderr, "*ERROR* in loadBlock: Block %ld ends outside of the file's limits", dstBlock);
        return E_FAILURE;
    }
    
    // Read the block into the file buffer
    fseek(src, srcOffset, SEEK_SET);
    
    long idx = 0;
    int ch;
    for(; idx < BLOCK_SIZE && (ch = fgetc(src)) != EOF; idx++) {
        fileBuf[dstOffset + idx] = ch;
    }
    
    // Return the number of bytes read
    return idx;
}

void cleanup() {
    if(baseFile != NULL) fclose(baseFile);
    if(newFile  != NULL) fclose(newFile);
    if(outFile  != NULL) fclose(outFile);
}

int allocBlock() {
    uint32_t dataBlock;
    
    if(!initMeta) {
        fprintf(stderr, "*ERROR* in allocBlock: Meta node not initialized");
        return E_FAILURE;
    }
    
    // Calculate the block of the first data block
    int mapBase = metaNode.nRefs / INODES_PER_BLOCK;
    if(metaNode.nRefs % INODES_PER_BLOCK != 0) {
        mapBase += 1;
    }
    
    dataBlock = mapBase + metaNode.nBlocks - 1; //Set the basic data location (less one)
    
    // Iterate through the map blocks to find a free slot
    bool_t found = false;
    for(int mapBlock = 0; mapBlock < metaNode.nBlocks && !found; mapBlock++) {
        char* blockBuf = fileBuf + ((mapBase + mapBlock) * BLOCK_SIZE);
        for(int mapByte = 0; mapByte < BLOCK_SIZE && !found; mapByte++) {
            char * blockChar = blockBuf + mapByte;
            for(int mapBit = 0; mapBit < 8 && !found; mapBit++) {
                dataBlock++;
                char mask = 0x80 >> mapBit;
                
                if((*blockChar & mask) ^ mask) {
                    *blockChar |= mask;
                    found = true;
                }
            }
        }
    }
    
    if(!found) {
        fprintf(stderr, "*ERROR* in allocBlock: Unable to alloc a new block\n");
        return E_FAILURE;
    }
    
    return dataBlock;
}

int freeBlock(int dataBlock) {
    if(!initMeta) {
        fprintf(stderr, "*ERROR* in freeBlock: Meta node not initialized");
        return E_FAILURE;
    }
    
    // Calculate the block nr of the first data block
    int mapBase = metaNode.nRefs / INODES_PER_BLOCK;
    if(metaNode.nRefs % INODES_PER_BLOCK != 0) {
        mapBase += 1;
    }
    
    dataBlock = dataBlock - mapBase - metaNode.nBlocks;
    
    // Calculate offsets
    int mapByte = dataBlock / 8;
    int mapBit = dataBlock % 8;
    
    char mask = 0xFF ^ (0x80 >> mapBit);
    fileBuf[mapBase * BLOCK_SIZE + mapByte] &= mask;
    
    return E_FAILURE;
}

int getNode(int idx, inode_t * node) {
    if(!fileRead) {
        fprintf(stderr, "*ERROR* in getNode: file buffer not initialized");
        return E_FAILURE;
    }
    
    int nodeBlock = idx / INODES_PER_BLOCK;
    idx = idx % INODES_PER_BLOCK;
    
    *node = *((inode_t*)(fileBuf + nodeBlock * BLOCK_SIZE + idx * sizeof(inode_t)));
    
    return E_SUCCESS;
}

int setNode(inode_t node) {
    int idx = node.id.idx;
    
    if(!fileRead) {
        fprintf(stderr, "*ERROR* in setNode: file buffer not initialized");
        return E_FAILURE;
    }
    
    int nodeBlock = idx / INODES_PER_BLOCK;
    int offset = idx % INODES_PER_BLOCK;
    
    for(int i = 0; i < sizeof(inode_t); i++) {
        fileBuf[nodeBlock * BLOCK_SIZE + offset + i] = ((char *)(&node))[i];
    }
}

int allocNode(uint32_t * idx) {
    if(!initMeta) {
        fprintf(stderr, "*ERROR* in allocNode: meta node not initialized\n");
        return E_FAILURE;
    }
    
    for(*idx = 2; *idx < metaNode.nRefs; *idx += 1) {
        inode_t node;
        getNode(*idx, &node);
        
        if(node.id.devID == 0 && node.id.idx == 0) {
            return E_SUCCESS;
        }
    }
    
    fprintf(stderr, "*ERROR* in allocNode: unable to allocate a new iNode\n");
    return E_FAILURE;
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
    
    outFile = fopen(argv[3], "wb");    // Open the new file for read and write
    if(outFile == NULL) {
        cleanup();
        fprintf(stderr, "*ERROR* Failed to open the output image file\n");
        return E_FAILURE;
    }
    
    // Read from the base file into the file buffer
    baseLen = loadFile(baseFile, 0);
    for(long idx = baseLen; idx < FILE_BUF_SIZE; idx++) {
        fileBuf[idx] = 0;
    }
    fileRead = true;
    
    // Read the meta node
    if(getNode(0, &metaNode) != E_SUCCESS) {
        cleanup();
        fprintf(stderr, "*ERROR* Failed to grab the meta node\n");
        return E_FAILURE;
    }
    initMeta = true;
    
    // Allocate a node for the new file
    int nodeIdx = 0;
    if(allocNode(&nodeIdx) != E_SUCCESS) {
        cleanup();
        fprintf(stderr, "*ERROR* Failed to allocate a file node\n");
        return E_FAILURE;
    }
    
    inode_t fileNode;
    for(int i = 0; i < sizeof(fileNode); i++) {
        ((char *)(&fileNode))[i] = 0;
    }
    fileNode.id.devID = metaNode.id.devID;
    fileNode.id.idx = nodeIdx;
    
    fileNode.nodeType = INODE_FILE_TYPE;
    fileNode.gid = 1;
    
    // Write the blocks of the new file to the buffer
    for(int i = 0; i < NUM_DIRECT_POINTERS * 4; i++) {
        long dBlock = allocBlock();
        if(dBlock == E_FAILURE) {
            cleanup();
            fprintf(stderr, "*ERROR* Alloc data block\n");
            return E_FAILURE;
        }
        
        long nWritten = loadBlock(newFile, dBlock, i); 
        if(nWritten < 0) {
            freeBlock(dBlock);
            break;
        }
        
        fileNode.direct_pointers[i / 4].blocks[i % 4] = dBlock;
        fileNode.nBlocks += 1;
        fileNode.nBytes += nWritten;
    }
    
    // Write the buffer to file
    writeFile(outFile);
    
    cleanup();
    return E_FAILURE;
}
