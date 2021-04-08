#ifndef M_USER_4_H_
#define M_USER_4_H_

int mUser4Kill(uint32_t arg1, uint32_t arg2);
int mUser4UserTgt(uint32_t arg1, uint32_t arg2);
int mUser4UserKill(uint32_t arg1, uint32_t arg2);

int mUser4(uint32_t arg1, uint32_t arg2) {
    char buf[64];
    uid_t uid = getuid();
    uint32_t childNr = arg2;
    
    if(uid != UID_ROOT) {
        sprint(buf, "M User %d.%d: **ERROR** Spawned as non-root.\r\n", arg1, childNr);
        cwrites(buf);
        swrites(buf);
        exit(-1);
    }
    
    mUser1(arg1, arg2);
    
    // Spawn the first target
    pid_t tgt, whom = tgt = spawn(userS, PRIO_STD, '\0', 1);
    if(whom < 0) {
        sprint(buf, "M User %d.%d: **ERROR** failed to spawn target.\r\n", arg1, childNr);
        cwrites(buf);
        swrites(buf);
        exit(-1);
    } else {
        sprint(buf, "M User %d.%d: spawned target (pid %d)\r\n", arg1, childNr, whom);
        cwrites(buf);
        swrites(buf);
    }
    
    // Test killing other's process as user
    swrites("\r\nAttempting to kill other's process as user\r\n");
    whom = spawn(mUser4UserKill, PRIO_STD, tgt, ++arg2);
    if(whom < 0) {
        sprint(buf, "M User %d.%d: failed to spawn killer.\r\n", arg1, childNr);
        cwrites(buf);
        swrites(buf);
        exit(-1);
    } else {
        sprint(buf, "M User %d.%d: spawned user killer (pid %d)\r\n", arg1, childNr, whom);
        cwrites(buf);
        swrites(buf);
        wait(NULL);
    }
    
    // Test killing own process as root
    swrites("\r\nAttempting to kill own process as root\r\n");
    whom = spawn(mUser4Kill, PRIO_STD, tgt, ++arg2);
    if(whom < 0) {
        sprint(buf, "M User %d.%d: failed to spawn killer.\r\n", arg1, childNr);
        cwrites(buf);
        swrites(buf);
        exit(-1);
    } else {
        sprint(buf, "M User %d.%d: spawned root killer (pid %d)\r\n", arg1, childNr, whom);
        cwrites(buf);
        swrites(buf);
        wait(NULL);
    }
    
    wait(NULL);
    swrites("\r\nPress enter to continue: ");
    readLn(CHAN_SIO, buf, 64, false);
    swrites("\r\n\r\n");
    
    // Spawn the user target
    whom = tgt = spawn(mUser4UserTgt, PRIO_STD, '\0', 1);
    if(whom < 0) {
        sprint(buf, "M User %d.%d: **ERROR** failed to spawn target.\r\n", arg1, childNr);
        cwrites(buf);
        swrites(buf);
        exit(-1);
    } else {
        sprint(buf, "M User %d.%d: spawned target (pid %d)\r\n", arg1, childNr, whom);
        cwrites(buf);
        swrites(buf);
    }
    
    // Test killing other's process as user
    swrites("\r\nAttempting to kill own process as user\r\n");
    whom = spawn(mUser4UserKill, PRIO_STD, tgt, ++arg2);
    if(whom < 0) {
        sprint(buf, "M User %d.%d: failed to spawn killer.\r\n", arg1, childNr);
        cwrites(buf);
        swrites(buf);
        exit(-1);
    } else {
        sprint(buf, "M User %d.%d: spawned user killer (pid %d)\r\n", arg1, childNr, whom);
        cwrites(buf);
        swrites(buf);
        wait(NULL);
    }
    
    wait(NULL);
    swrites("\r\nPress enter to continue: ");
    readLn(CHAN_SIO, buf, 64, false);
    swrites("\r\n\r\n");
    
    // Spawn second user target
    whom = tgt = spawn(mUser4UserTgt, PRIO_STD, '\0', 1);
    if(whom < 0) {
        sprint(buf, "M User %d.%d: **ERROR** failed to spawn target.\r\n", arg1, childNr);
        cwrites(buf);
        swrites(buf);
        exit(-1);
    } else {
        sprint(buf, "M User %d.%d: spawned target (pid %d)\r\n", arg1, childNr, whom);
        cwrites(buf);
        swrites(buf);
    }
    
    // Test killing other's process as user
    swrites("\r\nAttempting to kill other's process as root\r\n");
    whom = spawn(mUser4Kill, PRIO_STD, tgt, ++arg2);
    if(whom < 0) {
        sprint(buf, "M User %d.%d: failed to spawn killer.\r\n", arg1, childNr);
        cwrites(buf);
        swrites(buf);
        exit(-1);
    } else {
        sprint(buf, "M User %d.%d: spawned user killer (pid %d)\r\n", arg1, childNr, whom);
        cwrites(buf);
        swrites(buf);
        wait(NULL);
    }
    
    wait(NULL);
    swrites("\r\nPress enter to continue: ");
    readLn(CHAN_SIO, buf, 64, false);
    swrites("\r\n\r\n");
    
    return (0);
}

int mUser4UserTgt(uint32_t arg1, uint32_t arg2) {
    char buf[64];
    int32_t result = setuid(100);
    
    sleep(100);
    if(result < 0) {
        sprint(buf, "M User 4 User tgt: failed to change UID.\r\n");
        cwrites(buf);
        return result;
    }
    
    return userS(arg1, arg2);
}

int mUser4UserKill(uint32_t arg1, uint32_t arg2) {
    char buf[64];
    int32_t result = setuid(100);
    
    sleep(100);
    if(result < 0) {
        sprint(buf, "M User 4.%d: failed to change UID.\r\n", arg2);
        cwrites(buf);
        return result;
    }
    
    return mUser4Kill(arg1, arg2);
}

int mUser4Kill(uint32_t arg1, uint32_t arg2) {
    char buf[64];
    pid_t tgt = arg1;
    
    
    sleep(100);
    mUser1(4, arg2);
    sprint(buf, "M User 4.%d attempting to kill process %d\r\n", arg2, tgt);
    swrites(buf);
    
    int32_t result = kill(tgt);
    sprint(buf, "M User 4.%d: Kill order on tgt (%d) %s (result %d)\r\n", 
            arg2, tgt, (result < 0) ? "failed" : "succeded", result);
    swrites(buf);
    
    return E_SUCCESS;
}

#endif
