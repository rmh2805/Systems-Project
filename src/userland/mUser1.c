#ifndef M_USER_1_H_
#define M_USER_1_H_

int mUser1(uint32_t arg1, uint32_t arg2) {
    char buf[128];
    gid_t gid = getgid();
    uid_t uid = getuid();
    
    sprint(buf, "M User %d reports gid %d\n", arg1, (uint32_t)gid);
    cwrites(buf);
    sprint(buf, "M User %d reports uid %d\n", arg1, (uint32_t)uid);
    cwrites(buf);

    exit(0);
    return (42);
}

#endif
