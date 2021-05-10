#ifndef LS_H_
#define LS_H_

#include "common.h"

void getEntry(char * oBuf, inode_t node, const char * name) {
    sprint(oBuf, "    %c%c%c%c%c%c%c", 
            (node.nodeType == INODE_DIR_TYPE) ? 'd' : '-',
            (node.permissions & 0x20) ? 'w' : '-',
            (node.permissions & 0x10) ? 'r' : '-',
            (node.permissions & 0x08) ? 'w' : '-',
            (node.permissions & 0x04) ? 'r' : '-',
            (node.permissions & 0x02) ? 'w' : '-',
            (node.permissions & 0x01) ? 'r' : '-');

    sprint(oBuf + strlen(oBuf), "    %04x", node.uid);
    sprint(oBuf + strlen(oBuf), "  %04x", node.gid);
    
    sprint(oBuf + strlen(oBuf), "    %s", name);
    
    if(node.nodeType == INODE_DIR_TYPE) strcat(oBuf, "/");
    strcat(oBuf, "\r\n");
}

int32_t ls(uint32_t arg1, uint32_t arg2) {
    const int oBufSz = 128;

    char * path = (char *) arg1;
    char oBuf[oBufSz];
    char nBuf[MAX_FILENAME_SIZE + 1];
    inode_t node, child;
    int ret;

    // Read in the current entry
    ret = getinode(path, &node);
    if(ret < 0) {
        sprint(oBuf, "*ERROR* in ls: failed to get inode at path \"%s\"\r\n", path);
        cwrites(oBuf);
        return E_FAILURE;
    }

    // Check that this is a directory
    if(node.nodeType != INODE_DIR_TYPE) {
        sprint(oBuf, "*ERROR* in ls: inode at path \"%s\" is not a directory\r\n", path);
        cwrites(oBuf);
        return E_FAILURE;
    }

    // Print the current directory
    swrites("\r\n");
    getEntry(oBuf, node, ".");
    swrites(oBuf);

    // Print all child directories
    for(int i = 0; i < node.nBytes; i++) {
        // Get the child's name
        ret = dirname(path, nBuf, i);
        if(ret < 0) {
            sprint(oBuf, "*ERROR* in ls: failed to get inode %d at path \"%s\"\r\n", i, path);
            cwrites(oBuf);
            return E_FAILURE;
        }

        // Genearte the path
        strcpy(oBuf, path);
        strcat(oBuf, "/");
        strcat(oBuf, nBuf);

        // Get the child
        ret = getinode(oBuf, &child);
        if(ret < 0) {
            sprint(oBuf, "*ERROR* in ls: failed to get inode at path \"%s\"\r\n", oBuf);
            cwrites(oBuf);
            return E_FAILURE;
        }

        // Print the child
        getEntry(oBuf, child, nBuf);
        swrites(oBuf);

    }

    return E_SUCCESS;
}

#endif