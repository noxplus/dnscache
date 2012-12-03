#include "util.h"
#include <sys/time.h>

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
    struct timeval tv;
    struct timezone tz;
    int hour, minute, second;

    if (level < PRT_ERROR || level > PRT_TEST)
    {
        return -1;
    }

    if (level == PRT_ERROR || level == PRT_WARNING) fp = stderr;

    gettimeofday(&tv, &tz);
    tv.tv_sec -= tz.tz_minuteswest*60;
    second = tv.tv_sec % 60;
    minute = (tv.tv_sec / 60 ) % 60;
    hour = (tv.tv_sec / 3600 ) % 24;

    va_start(argptr, fmt);
    cnt = vsnprintf(strbuf, sizeof(strbuf), fmt, argptr);
    va_end(argptr);

    fprintf(fp, "[%02d:%02d:%02d.%03ld]%s[%s] %s\033[m\n", hour, minute, second, tv.tv_usec/1000, lvlclr[level], lvlstr[level], strbuf);
    return cnt;
}
