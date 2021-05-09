#ifndef TEST_FS_5_H_
#define TEST_FS_5_H_

#include "common.h"

int testFS5(uint32_t arg1, uint32_t arg2) {
    char buf[128];
    int ret;
    inode_t node;

    sprint(buf, "Test FS %d.%d: Attempting to grab FS root node\r\n", arg1, arg2);
    swrites(buf);

    ret = getinode("/", &node);
    if(ret < 0) {
        sprint(buf, "Test FS %d.%d: Failed to grab root node, (status %d)\r\n", arg1, arg2, ret);
        swrites(buf);
        return E_FAILURE;
    }

    sprint(buf, "Test FS %d.%d: Grabbed root node, printing stats\r\n", arg1, arg2);
    swrites(buf);
    sprint(buf, "\t     Root ID: %d.%d\r\n", node.id.devID, node.id.idx);
    swrites(buf);
    sprint(buf, "\tRoot Entries: %d\r\n", node.nBytes);
    swrites(buf);

    return 0;
}

#endif
