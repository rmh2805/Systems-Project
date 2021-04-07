#ifndef M_USER_2_H_
#define M_USER_2_H_

int mUser2(uint32_t arg1, uint32_t arg2) {
    int32_t result;
    
    cwrites("\nM User 2 reporting initial values\n");
    swrites("M User 2 reporting initial values\r\n");
    mUser1(arg1, arg2);
    
    uid_t uid = getuid();

    if(uid == UID_ROOT) {
        // Test setting invalid GID as root
        swrites("M User 2 attempting to set invalid gid as root\r\n");
        result = setgid(0xF00D);
        if(result != E_SUCCESS) {
            swrites("M User 2 reports failure\r\n");
        } else {
            swrites("M User 2 reports success\r\n");
        }
        mUser1(arg1, arg2);
        
        // Test setting valid GID as root
        swrites("M User 2 attempting to set valid gid as root\r\n");
        result = setgid(GID_OPEN);
        if(result != E_SUCCESS) {
            swrites("M User 2 reports failure\r\n");
        } else {
            swrites("M User 2 reports success\r\n");
        }
        mUser1(arg1, arg2);
        
        // Test changing UID from root (no group reset)
        swrites("M User 2 attempting to set uid to root as root\r\n");
        result = setuid(UID_ROOT);
        if(result != E_SUCCESS) {
            swrites("M User 2 reports failure\r\n");
        } else {
            swrites("M User 2 reports success\r\n");
        }
        mUser1(arg1, arg2);
        
        // Test become user from root (group reset)
        swrites("M User 2 attempting to set uid to user as root\r\n");
        result = setuid(100);
        if(result != E_SUCCESS) {
            swrites("M User 2 reports failure\r\n");
        } else {
            swrites("M User 2 reports success\r\n");
        }
        mUser1(arg1, arg2);
    } else {
        swrites("M User 2 spawned as non-root, skipping root tests\r\n");
    }
    
    // Test become self as user
    swrites("M User 2 attempting to set uid to self as user\r\n");
    result = setuid(100);
    if(result != E_SUCCESS) {
        swrites("M User 2 reports failure\r\n");
    } else {
        swrites("M User 2 reports success\r\n");
    }
    mUser1(arg1, arg2);
    
    // Test change uid as user
    swrites("M User 2 attempting to set uid to other user as user\r\n");
    result = setuid(101);
    if(result != E_SUCCESS) {
        swrites("M User 2 reports failure\r\n");
    } else {
        swrites("M User 2 reports success\r\n");
    }
    mUser1(arg1, arg2);
    
    // Test become root as user
    swrites("M User 2 attempting to set uid to root as user\r\n");
    result = setuid(UID_ROOT);
    if(result != E_SUCCESS) {
        swrites("M User 2 reports failure\r\n");
    } else {
        swrites("M User 2 reports success\r\n");
    }
    mUser1(arg1, arg2);
    
    // Test set invalid group as user
    swrites("M User 2 attempting to set invalid gid as user\r\n");
    result = setgid(0xF00D);
    if(result != E_SUCCESS) {
        swrites("M User 2 reports failure\r\n");
    } else {
        swrites("M User 2 reports success\r\n");
    }
    mUser1(arg1, arg2);
    
    // Test set valid group as user
    swrites("M User 2 attempting to set valid gid as user\r\n");
    result = setgid(GID_OPEN);
    if(result != E_SUCCESS) {
        swrites("M User 2 reports failure\r\n");
    } else {
        swrites("M User 2 reports success\r\n");
    }
    mUser1(arg1, arg2);
    
    return (0);
}

#endif