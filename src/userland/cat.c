#ifndef CAT_H_
#define CAT_H_

#include "common.h"
int32_t cat(uint32_t arg1, uint32_t arg2) {
    char * path = (char *) arg1;
    char buf[128];
    char oBuf[130];
    int ret;
    int fp = fopen(path, false);

    if(fp < 0) {
        sprint(oBuf, "*ERROR* in cat: File open error (%d)\r\n", fp);
            swrites(oBuf);
            cwrites(oBuf);
        return E_FAILURE;
    }

    swrites("\r\n");
    while(true) {
        ret = fReadLn(fp, buf, 128);
        if(ret == E_EOF) {
            break;
        } else if (ret < 0) {
            sprint(oBuf, "*ERROR* in cat: File read error (%d)\r\n", ret);
            swrites(oBuf);
            cwrites(oBuf);
            return E_FAILURE;
        }

        sprint(oBuf, "%s\r\n", buf);
        swrites(oBuf);
    }

    ret = fclose(fp);
    if(ret < 0) {
        sprint(oBuf, "*ERROR* in cat: File close error (%d)\r\n", fp);
        swrites(oBuf);
        cwrites(oBuf);
        return E_FAILURE;
    }

    return E_SUCCESS;
}

#endif