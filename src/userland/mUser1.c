#ifndef M_USER_1_H_
#define M_USER_1_H_

int mUser1(uint32_t arg1, uint32_t arg2) {
    char buf[64];
    sprint(buf, "mUser1 reports gid %08x, uid %08x", getgid(), getuid());
    cwrites(buf);

    exit(0);
    return (42);
}

#endif