#ifndef M_USER_3_H_
#define M_USER_3_H_

int mUser3(uint32_t arg1, uint32_t arg2) {
    int32_t result = setgid(GID_OPEN);
    if (result < 0) {
        cwrites("M User 3 reporting failure to change gid\n");
        swrites("M User 3 reporting failure to change gid\n");
        return (-1);
    }

    cwrites("\nM User 3 reporting initial values\n");
    swrites("M User 3 reporting initial values\n");

    

    mUser1(arg1, arg2);
    uid_t uid = getuid();
    pid_t whom;

    if(uid == UID_ROOT) {
        // Test inhereting group membership as group

        cwrites("\nM User 3 testing group inheretence as root\n");
        swrites("M User 3 testing group inheretence as root\n");
        
        whom = spawn( mUser1, PRIO_HIGHEST, arg1, ++arg2 );
        if( whom < 0 ) {
            cwrites( "M user 3, spawn() failed\n" );
        }

        
        // Test inheriting changed user
        
        result = setuid(100);
        if (result < 0) {
            cwrites("M User 3 reporting failure to change uid\n");
            swrites("M User 3 reporting failure to change uid\n");
            return (-1);
        }

        cwrites("\nM User 3 testing changed user inheretence\n");
        swrites("M User 3 testing changed user inheretence\n");
        
        whom = spawn( mUser1, PRIO_HIGHEST, arg1, ++arg2 );
        if( whom < 0 ) {
            cwrites( "M user 3, spawn() failed\n" );
        }
    }

    result = setgid(GID_OPEN);
    if (result < 0) {
        cwrites("M User 3 reporting failure to change gid\n");
        swrites("M User 3 reporting failure to change gid\n");
        return (-1);
    }
    cwrites("\nM User 3 testing group inheretence as user\n");
    swrites("M User 3 testing group inheretence as user\n");
    
    whom = spawn( mUser1, PRIO_HIGHEST, arg1, ++arg2 );
    if( whom < 0 ) {
        cwrites( "M user 3, spawn() failed\n" );
    }

    return (E_SUCCESS);

}

#endif
