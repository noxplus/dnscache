#include "inc.h"

uint32 random32(void)
{
    uint32 uiret = 0;
    int fd = open("/dev/urandom", O_RDONLY);
    read(fd, &uiret, sizeof(uiret));
    close(fd);

    return uiret;
}

//参数：
//      通知级别 error, notice, warning, info, debug, test
int Notify(int level, char *fmt, ...)
{
    const char* lvlstr[6] = {"E", "N", "W","I", "D", "T"};
    //error, notice, warning, info, debug, test
    const char* lvlclr[6] = {RED, PURPLE, CYAN, BROWN, GREEN, LGRAY};
    va_list argptr;
    int cnt;
    char strbuf[1024];
    FILE * fp = stdout;

    if (level < PRT_ERROR || level > PRT_TEST)
    {
        return -1;
    }

    if (level == PRT_ERROR || level == PRT_WARNING) fp = stderr;

    va_start(argptr, fmt);
    cnt = vsnprintf(strbuf, sizeof(strbuf), fmt, argptr);
    va_end(argptr);

    fprintf(fp, "%s[%s] %s\033[m\n", lvlclr[level], lvlstr[level], strbuf);
    return cnt;
}