#ifndef CHMOD_H_
#define CHMOD_H_

#include "common.h"
int chmod(uint32_t arg1, uint32_t arg2) {
    char * permStr = (char*) arg1;
    char * path = (char *) arg2;

    bool_t doUser = false, doGroup = false, doAll = false;
    bool_t doRead = false, doWrite = false;

    bool_t doRem = false;

    strLower(permStr, permStr);

    // Set cohort flags
    int i = 0;
    for(; permStr[i] == 'u' || permStr[i] == 'g' || permStr[i] == 'o'; i++) {
        switch(permStr[i]) {
            case 'u':
                doUser = true;
                break;
            case 'g':
                doGroup = true;
                break;
            case 'o':
                doAll = true;
                break;
        }
    }
    if(!doUser && !doGroup && !doAll) {
        doUser = true;
        doGroup = true;
        doAll = true;
    }

    // Set doRem properly
    if(permStr[i] == '+') {
        doRem = false;
    } else if (permStr[i] == '-') {
        doRem = true;
    } else {
        cwrites("*ERROR* in chmod: missing type specifier (+ or -)\n");
        return E_BAD_PARAM;
    }
    i++;

    // Set type markers
    for(; permStr[i] == 'r' || permStr[i] == 'w'; i++) {
        if(permStr[i] == 'r') {
            doRead = true;
        } else {
            doWrite = true;
        }
    }

    // Get the base permissions
    inode_t node;
    int ret = getinode(path, &node) ;
    if(ret < 0) {
        cwrites("*ERROR* in chmod: Failed to get inode (+ or -)\n");
        return E_BAD_PARAM;
    }

    // Build the mask
    uint32_t mask = 0;
    if(doUser  && doRead)  mask += 0x01;
    if(doUser  && doWrite) mask += 0x02;
    if(doGroup && doRead)  mask += 0x04;
    if(doGroup && doWrite) mask += 0x08;
    if(doAll   && doRead)  mask += 0x10;
    if(doAll   && doWrite) mask += 0x20;

    // Get the original permissions
    uint32_t perms = node.permissions;
    if(doRem) {
        perms &= (~mask);
    } else {
        perms |= mask;
    }

    // Set the new permissions
    ret = fSetPerm(path, perms);
    if(ret < 0) {
        cwrites("*ERROR* in chmod: Failed to set permissions\n");
        return E_FAILURE;
    }

    return E_SUCCESS;
}

#endif