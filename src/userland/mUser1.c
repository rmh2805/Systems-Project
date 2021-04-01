#ifndef M_USER_1_H_
#define M_USER_1_H_

int mUser1(uint32_t arg1, uint32_t arg2) {
    char buf[128];
    gid_t gid = getgid();
    uid_t uid = getuid();
    
    sprint(buf, "M User 1 reports gid %d\n", gid);
    cwrites(buf);
    sprint(buf, "M User 1 reports uid %d\n", uid);
    cwrites(buf);

    exit(0);
    return (42);
}

#endif