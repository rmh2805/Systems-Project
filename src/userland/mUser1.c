#ifndef M_USER_1_H_
#define M_USER_1_H_

int mUser1(uint32_t arg1, uint32_t arg2) {
    char buf[128];
    gid_t gid = getgid();
    uid_t uid = getuid();
    
    sprint(buf, "M User %d.%d reports uid %d\r\nM User %d.%d reports gid %0d\r\n", 
            arg1, arg2, (uint32_t)uid, arg1, arg2, (uint32_t) gid);
    swrites(buf);
    
    return (0);
}

#endif
