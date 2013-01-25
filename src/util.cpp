#include "util.hpp"

#ifdef _WIN32
extern errno_t rand_s(unsigned int* randomValue);
#endif

uint32 random32(void)
{
    uint32 uiret = 0;
    uint32 seeds = 0;
    uint32 times = GetTimeMs();

#ifdef __linux__
    static int fd = -1;
    if (fd == -1) fd = open("/dev/urandom", O_RDONLY);
    read(fd, &uiret, sizeof(uiret));
    //close(fd);
#endif
#ifdef _WIN32
    rand_s((uint*)&uiret);
#endif

    seeds = times & 0xff;
    seeds += seeds << 16;
    seeds += seeds << 8;

    return uiret ^ seeds;
}

//获取毫秒数
//结果总是用来相减，所以不用考虑0值
uint32 GetTimeMs(void)
{
#ifdef __linux__
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec*1000 + (tv.tv_usec+500)/1000;
#else
    return GetTickCount();
#endif
}

void SleepMS(uint32 stime)
{
#ifdef _WIN32
    Sleep(stime);
#else
    usleep(stime * 1000);
#endif
}

//参数：
//      通知级别 error, notice, warning, info, debug, test
int Notify(int level, const char *fmt, ...)
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
    int hour, min, sec, msec;

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
    hour = now.tm_hour;
    min = now.tm_min;
    sec = now.tm_sec;
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

#ifdef __linux__
    if (level == PRT_ERROR)
    {
        fprintf(fp, "[%02d:%02d:%02d.%03d]%s[%s] %s [%d:%s]\033[m\n", hour, min, sec, msec, lvlclr[level], lvlstr[level], strbuf, errno, strerror(errno));
    }
    else
    {
        fprintf(fp, "[%02d:%02d:%02d.%03d]%s[%s] %s\033[m\n", hour, min, sec, msec, lvlclr[level], lvlstr[level], strbuf);
    }
#endif
#ifdef _WIN32
    HANDLE hConsole;
    if (fp == stderr) hConsole = GetStdHandle(STD_ERROR_HANDLE);
    else hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, LGRAY);
    fprintf(fp, "[%02d:%02d:%02d.%03d]", hour, min, sec, msec);
    SetConsoleTextAttribute(hConsole, lvlclr[level]);
    if (level == PRT_ERROR)
    {
        fprintf(fp, "[%s] %s [%d]\n", lvlstr[level], strbuf, GetLastError());
    }
    else
    {
        fprintf(fp, "[%s] %s\n", lvlstr[level], strbuf);
    }
    SetConsoleTextAttribute(hConsole, LGRAY);
#endif
    return cnt;
}

//true: 占有/释放
//false: 测试是否可用
bool GetTF(bool flag)
{
    static bool lflag = true;

    if (flag == true)
    {
        lflag = !lflag;
        return !lflag;
    }

    if (lflag == false) SleepMS(10);
    return lflag;
}
