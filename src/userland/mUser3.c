#ifndef M_USER_3_H_
#define M_USER_3_H_

int mUser3(uint32_t arg1, uint32_t arg2) {
    int32_t result = setgid(GID_OPEN);
    if (result < 0) {
        swrites("M User 3 reporting failure to change gid\r\n");
        return (-1);
    }

    swrites("M User 3 reporting initial values\r\n");

    

    mUser1(arg1, arg2);
    uid_t uid = getuid();
    pid_t whom;

    if(uid == UID_ROOT) {
        // Test inhereting group membership as group

        swrites("M User 3 testing group inheretence as root\r\n");
        
        whom = spawn( mUser1, PRIO_HIGHEST, arg1, ++arg2 );
        if( whom < 0 ) {
            cwrites( "M user 3, spawn() failed\n" );
            swrites( "M user 3, spawn() failed\r\n" );
        } else {
            wait(NULL);
        }

        
        // Test inheriting changed user
        
        result = setuid(100);
        if (result < 0) {
            swrites("M User 3 reporting failure to change uid\r\n");
            return (-1);
        }

        swrites("M User 3 testing changed user inheretence\r\n");
        
        whom = spawn( mUser1, PRIO_HIGHEST, arg1, ++arg2 );
        if( whom < 0 ) {
            cwrites( "M user 3, spawn() failed\n" );
            swrites( "M user 3, spawn() failed\r\n" );
        } else {
            wait(NULL);
        }
    }

    result = setgid(GID_OPEN);
    if (result < 0) {
        swrites("M User 3 reporting failure to change gid\r\n");
        return (-1);
    }
    swrites("M User 3 testing group inheretence as user\r\n");
    
    whom = spawn( mUser1, PRIO_HIGHEST, arg1, ++arg2 );
    if( whom < 0 ) {
        cwrites( "M user 3, spawn() failed\n" );
        swrites( "M user 3, spawn() failed\r\n" );
    } else {
        wait(NULL);
    }

    return (E_SUCCESS);

}

#endif
