#include "util.h"

uint32 random32(void)
{
    uint32 uiret = 0;
#ifdef __linux__
    int fd = open("/dev/urandom", O_RDONLY);
    read(fd, &uiret, sizeof(uiret));
    close(fd);
#endif
#ifdef _WIN32
    rand_s(&uiret);
#endif

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
    int hour, min, sec, msec;

    if (level < PRT_ERROR || level > PRT_TEST)
    {
        return -1;
    }

    if (level == PRT_ERROR || level == PRT_WARNING) fp = stderr;

#ifdef __linux__
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);
    tv.tv_sec -= tz.tz_minuteswest*60;
    sec = tv.tv_sec % 60;
    min = (tv.tv_sec / 60 ) % 60;
    hour = (tv.tv_sec / 3600 ) % 24;
    msec = tv.tv_usec / 1000;
#endif
#ifdef _WIN32
    SYSTEMTIME now;
    GetLocalTime(&now);
    hour = now.wHour;
    min = now.wMinute;
    sec = now.wSecond;
    msec = now.wMilliseconds;
#endif

    va_start(argptr, fmt);
    cnt = vsnprintf(strbuf, sizeof(strbuf), fmt, argptr);
    va_end(argptr);

    fprintf(fp, "[%02d:%02d:%02d.%03d]%s[%s] %s\033[m\n", hour, min, sec, msec, lvlclr[level], lvlstr[level], strbuf);
    return cnt;
}
