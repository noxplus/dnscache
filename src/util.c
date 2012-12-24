#include "util.h"

#ifdef _WIN32
extern errno_t rand_s(unsigned int* randomValue);
#endif

uint32 random32(void)
{
    uint32 uiret = 0;
#ifdef __linux__
    int fd = open("/dev/urandom", O_RDONLY);
    read(fd, &uiret, sizeof(uiret));
    close(fd);
#endif
#ifdef _WIN32
    rand_s((uint*)&uiret);
#endif

    return uiret;
}

//参数：
//      通知级别 error, notice, warning, info, debug, test
int Notify(int level, char *fmt, ...)
{
    const char* lvlstr[6] = {"E", "N", "W","I", "D", "T"};
    //error, notice, warning, info, debug, test
#ifdef __linux__
    const char* lvlclr[6] = {RED, PURPLE, CYAN, BROWN, GREEN, LGRAY};
#endif
#ifdef _WIN32
    const WORD lvlclr[6] = {RED, PURPLE, CYAN, BROWN, GREEN, LGRAY};
#endif
    va_list argptr;
    int cnt;
    char strbuf[1024];
    FILE * fp = stdout;
    int year, mon, day, hour, min, sec, msec;

    if (level < PRT_ERROR || level > PRT_TEST)
    {
        return -1;
    }

    if (level == PRT_ERROR || level == PRT_WARNING) fp = stderr;

#ifdef __linux__
    struct timeval tv;
    struct tm now;
    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &now);
    year = now.tm_year + 1900;
    mon = now.tm_mon + 1;
    day = now.tm_mday;
    hour = now.tm_hour;
    min = now.tm_min;
    sec = now.tm_sec;
    msec = tv.tv_usec / 1000;
#endif
#ifdef _WIN32
    SYSTEMTIME now;
    GetLocalTime(&now);
    year = now.wYear;
    mon = now.wMonth;
    day = now.wDay;
    hour = now.wHour;
    min = now.wMinute;
    sec = now.wSecond;
    msec = now.wMilliseconds;
#endif

    va_start(argptr, fmt);
    cnt = vsnprintf(strbuf, sizeof(strbuf), fmt, argptr);
    va_end(argptr);

#ifdef __linux__
    fprintf(fp, "[%02d:%02d:%02d.%03d]%s[%s] %s\033[m\n", hour, min, sec, msec, lvlclr[level], lvlstr[level], strbuf);
#endif
#ifdef _WIN32
    HANDLE hConsole;
    if (fp == stderr) hConsole = GetStdHandle(STD_ERROR_HANDLE);
    else hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, WHITE);
    fprintf(fp, "[%02d:%02d:%02d.%03d]", hour, min, sec, msec);
    SetConsoleTextAttribute(hConsole, lvlclr[level]);
    fprintf(fp, "[%s] %s\n", lvlstr[level], strbuf);
    SetConsoleTextAttribute(hConsole, WHITE);
#endif
    return cnt;
}
