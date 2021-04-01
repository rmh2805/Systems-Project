#ifndef M_USER_2_H_
#define M_USER_2_H_

int mUser2(uint32_t arg1, uint32_t arg2) {
    if(setuid(1) != E_SUCCESS) {
        cprints("M User 2 reports failure to change UID");
    }
    
    if(setgid(1) != E_SUCCESS) {
        cprints("M User 2 reports failure to change GID");
    }
    
    mUser1(arg1, arg2);

    exit(0);
    return (42);
}

#endif
