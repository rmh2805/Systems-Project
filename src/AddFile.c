#include "common.h"

// avoid complaints about stdio.h
#undef NULL

#include <stdio.h>

#define INODES_PER_BLOCK ((int32_t)(BLOCK_SIZE / sizeof(inode_t)))

#define BLOCKBUF_SIZE BLOCK_SIZE + 1

// Files
FILE* baseFile;
FILE* newFile;
FILE* outFile;

// String variables
char* callPath;
char* outFileName;
char blockBuf[BLOCKBUF_SIZE];

// inode variables
bool_t initMeta = false;
inode_t metaNode;

void printUsage() {
    printf("*Usage*: %s <Base FS File> <File to insert> <Output File>\n", callPath);
}

void cleanup() {
    if(baseFile != NULL) fclose(baseFile);
    if(newFile  != NULL) fclose(newFile);
    if(outFile  != NULL) fclose(outFile);
}


int loadBlock(FILE* file, uint32_t idx);
int writeBuf();

int fCpy(FILE* dst, FILE* src) {
    
    fseek(src, 0, SEEK_END);
    long sz = ftell(src);
    
    fseek(src, 0, SEEK_SET);
    for(int i = 0; i < sz; i++) {
        fputc(fgetc(src), dst);
    }
    
    return E_SUCCESS;
}

int loadBlock(FILE* file, uint32_t idx) {
    int ret;
    long sz, offset = idx * BLOCK_SIZE;
    
    //Get the file size
    ret = fseek(file, 0, SEEK_END);
    if(ret != 0) {
        fprintf(stderr, "*ERROR* loadBlock failed to seek EOF\n");
        return E_FAILURE;
    }
    sz = ftell(file);
    
    // Check that this block is in the file
    if(offset >= sz) {
        return E_FAILURE;
    }
    
    // Go to the start of the block
    ret = fseek(file, offset, SEEK_SET);
    if(ret != 0) {
        fprintf(stderr, "*ERROR* loadBlock failed to seek index %u\n", idx);
        return E_FAILURE;
    }
    
    // Read from the file into the buffer
    int i = 0;
    for(; i < BLOCK_SIZE; i++) {
        ret = fgetc(file);
        if(ret == EOF) {
            break;
        }
        blockBuf[i] = (char) ret;
    }
    for(; i < BLOCK_SIZE; i++) {
        blockBuf[i] = 0;
    }
    
    // Return success
    return E_SUCCESS;
}

int writeBuf() {
    int ret;
    
    // Read from the file into the buffer
    for(int i = 0; i < BLOCK_SIZE; i++) {
        ret = fputc(blockBuf[i], outFile);
        if(ret == EOF) {
            fprintf(stderr, "*ERROR* writeBlock failed to write to file\n");
            return E_FAILURE;
        }
    }
    
    // Return success
    return E_SUCCESS;
}

int writeBlock(uint32_t idx) {
    int ret;
    
    FILE* tmp = tmpfile();
    if(tmp == NULL) {
        fprintf(stderr, "*ERROR* in writeBlock: failed to create a temp file for block insertion\n");
        return E_FAILURE;
    }
    
    // Copy the current file to the temp file
    ret = fCpy(tmp, outFile);
    if(ret != E_SUCCESS) {
        fclose(tmp);
        fprintf(stderr, "*ERROR* in writeBlock: copy from base file to temp file");
        return E_FAILURE;
    }
    
    // Open a new, blank output file
    fclose(outFile);
    outFile = fopen(outFileName, "w+b");
    if(outFile == NULL) {
        fclose(tmp);
        fprintf(stderr, "*ERROR* in writeBlock: reopen outFile");
        return E_FAILURE;
    }
    
    // Copy blocks up to idx to output file
    fseek(tmp, 0, SEEK_SET);
    int blockNr;
    for(blockNr = 0; blockNr < idx; blockNr++) {
        for(int i = 0; i < BLOCK_SIZE; i++) {
            fputc(fgetc(tmp), outFile);
        }
    }
    
    // Copy the modified block
    writeBuf();
    blockNr++;
    
    // Copy the remaining blocks
    for(; loadBlock(tmp, blockNr) == E_SUCCESS; blockNr++) {
        writeBuf();
    }
    
    
    fclose(tmp);
    return E_SUCCESS;
}

int loadInode(FILE* file, inode_id_t id, inode_t * inode) {
    int ret;
    
    // Load the block containing the inode
    ret = loadBlock(file, (id.idx / sizeof(inode_t)));
    if(ret < 0) {
        fprintf(stderr, "*ERROR* loadInode failed to read the block containing inode %u.%u\n", id.devID, id.idx);
        return E_FAILURE;
    }
    
    // Copy from the loaded block into the return pointer
    int offset = sizeof(inode_t) * (id.idx % sizeof(inode_t));
    *inode = *((inode_t *)(blockBuf + offset));
    
    return E_SUCCESS;
}

int writeInode(FILE* baseFile, inode_t inode) {
    int ret;
    inode_id_t id = inode.id;
    
    // Load the block containing the inode
    int blockNr = (id.idx / INODES_PER_BLOCK);
    
    printf("%d\n", blockNr);
    ret = loadBlock(baseFile, blockNr);
    if(ret < 0) {
        fprintf(stderr, "*ERROR* writeInode failed to read the block containing inode %u.%u\n", id.devID, id.idx);
        return E_FAILURE;
    }
    
    // Copy from the loaded block into the return pointer
    int offset = sizeof(inode_t) * (id.idx % INODES_PER_BLOCK);
    for(int i = 0; i < sizeof(inode_t); i++) {
        blockBuf[i + offset] = ((char *)(&inode))[i];
    }
    
    // Write the updated block out to file
    ret = writeBlock(blockNr);
    if(ret < 0) {
        fprintf(stderr, "*ERROR* writeInode failed to write the block containing inode %u.%u\n", id.devID, id.idx);
        return E_FAILURE;
    }
    
    return E_SUCCESS;
}

int allocBlock(uint32_t * idx) {
    int ret, mapBlock, mapByte, mapBit;
    
    if(!initMeta) {
        fprintf(stderr, "*ERROR* in allocBlock: meta node uninitialized\n");
        return E_FAILURE;
    }
    
    int mapBase = metaNode.nRefs / INODES_PER_BLOCK;
    if(metaNode.nRefs % INODES_PER_BLOCK != 0) {
        mapBase += 1;
    }
    
    // Start idx pointing at the first data block
    *idx = mapBase + metaNode.nBlocks;
    
    bool_t found = false;
    for(mapBlock = 0; mapBlock < metaNode.nBlocks && !found; mapBlock++) {
        ret = loadBlock(outFile, mapBlock + mapBase);
        if(ret < 0) {
            fprintf(stderr, "*ERROR* in allocBlock: unable to read map block %d\n", mapBlock);
            return E_FAILURE;
        }
        
        for(mapByte = 0; mapByte < BLOCK_SIZE && !found; mapByte++) {
            char ch = blockBuf[mapByte];
            
            for(mapBit = 0; mapBit < 8 && !found; mapBit++) {
                *idx += 1;                      // Increment the index
                
                char mask = 0x80 >> mapBit;
                
                if((ch & mask) ^ mask) {        // (if the target bit is not set)
                    blockBuf[mapByte] |= mask;  // Set the map bit
                    found = true;               // Mark that we have found a free block
                }
            }
        }
    }
    
    // Fail out if no free block was found
    if(!found) {
        fprintf(stderr, "*ERROR* in allocBlock: find a free block\n");
        return E_FAILURE;
    }
    
    // Write the updated map to the file
    ret = writeBlock(mapBlock + mapBase);
    
    return E_SUCCESS;
}

int getFreeNode(FILE* file, uint32_t * idx) {
    inode_t node;
    int ret;
    
    if(!initMeta) {
        fprintf(stderr, "*ERROR* in getFreeNode: meta node uninitialized\n");
        return E_FAILURE;
    }
    
    for(*idx = 0; *idx < metaNode.nRefs; *idx += 1) {
        ret = loadInode(file, (inode_id_t) {metaNode.id.devID, *idx}, &node);
        if(ret != E_SUCCESS) {
            fprintf(stderr, "*ERROR* in getFreeNode: Failed to grab inode %u\n", *idx);
            return E_FAILURE;
        }
        
        if(node.id.devID == 0 && node.id.idx == 0) {
            return E_SUCCESS;
        }
    }
    
    fprintf(stderr, "*ERROR* in getFreeNode: Unable to find a free inode in provided FS image\n");
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
    
    outFile = fopen(argv[3], "w+b");    // Open the new file for read and write
    if(outFile == NULL) {
        cleanup();
        fprintf(stderr, "*ERROR* Failed to open the output image file\n");
        return E_FAILURE;
    }
    outFileName = argv[3];
    
    // Copy contents of file to output file
    int ret;
    ret = fCpy(outFile, baseFile);
    if(ret != E_SUCCESS) {
        cleanup();
        fprintf(stderr, "*ERROR* Failed to copy base file to output\n");
        return E_FAILURE;
    }
    
    // Get the meta node
    ret = loadInode(outFile, (inode_id_t) {0, 0}, &metaNode);
    if(ret != E_SUCCESS) {
        cleanup();
        fprintf(stderr, "*ERROR* Failed to load meta node from file\n");
        return E_FAILURE;
    }
    initMeta = true;
    
    // Allocate a new inode
    int idx = 0;
    ret = getFreeNode(outFile, &idx);
    if(ret != 0) {
        cleanup();
        fprintf(stderr, "*ERROR* Failed to allocate new inode in file\n");
        return E_FAILURE;
    }
    
    inode_t node;
    for(int i = 0; i < sizeof(node); i++) {
        ((char*)(&node))[i] = 0;
    }
    
    node.id.devID = metaNode.id.devID;
    node.id.idx = idx;
    
    node.nodeType = INODE_FILE_TYPE;
    node.nRefs = 1;
    
    
    // Copy new file's contents to output file
    fseek(newFile, 0, SEEK_END);
    long fSz = ftell(newFile);
    fseek(newFile, 0, SEEK_SET);
    
    for(int blockNr = 0; blockNr < NUM_DIRECT_POINTERS * 4 && blockNr * BLOCK_SIZE < fSz; blockNr++) { // Only load up to direct ptr capacity
        // Allocate the next data block
        int fIdx = 0;
        ret = allocBlock(&fIdx);
        if(ret != E_SUCCESS) {
            cleanup();
            fprintf(stderr, "*ERROR* Failed to allocate new data block\n");
            return E_FAILURE;
        }
        
        // Load the next data block in from file
        ret = loadBlock(newFile, blockNr);
        if(ret != E_SUCCESS) {
            cleanup();
            fprintf(stderr, "*ERROR* Unexpected EOF in new file\n");
            return E_FAILURE;
        }
        
        // Write the data block out to file
        ret = writeBlock(fIdx);
        if(ret != E_SUCCESS) {
            cleanup();
            fprintf(stderr, "*ERROR* Failed to write new block to file\n");
            return E_FAILURE;
        }
        
        // Update the inode
        node.nBlocks += 1;
        node.direct_pointers[blockNr/4].blocks[blockNr%4] = fIdx;
    }
    node.nBytes += fSz; // Update the node's size
    
    // Write the new inode to disk
    ret = writeInode(outFile, node);
    if(ret != E_SUCCESS) {
        cleanup();
        fprintf(stderr, "*ERROR* Failed to write new inode to disk\n");
        return E_FAILURE;
    }
    
    // Insert a new entry in the root directory

    cleanup();
    return E_SUCCESS;
}
