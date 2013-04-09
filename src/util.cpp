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

const char* err2str(int err_no)
{
    if (err_no < ERR_no || ERR_no > ERR_end) return "no such err code";
    static const char errstr[][32] =
    {
        "normal error", "net socket error", "net bind error", 
        "net connect error","net send error", "net recv error",
        "net select error", "net connect timeout", "net send timeout",
        "net recv timeout", "net select timeout", 
        "space malloc fail", "dns query records too many",
        "dns answer records too many",
        "error end"
    };

    return errstr[err_no - ERR_no];
}

char toc(unsigned char c)
{
    if (c >= 0x20 && c <= 0x7e) return c;
    return '.';
}

int HexDump(char* buf, uint32 len)
{
    uint32 index = 0;
    unsigned char* l = (unsigned char*)buf;
    Notify(PRT_NOTICE, "HexDump len = %d", len);
    fprintf(stderr, "      00 01 02 03 04 05 06 07  ");
    fprintf(stderr, "08 09 0A 0B 0C 0D 0E 0F   123456789ABCDEF\n");
    fprintf(stderr, "-----------------------------------------------------\n");
    while (len - index >= 16)
    {
        fprintf(stderr, "%04lx  ", index);
        fprintf(stderr, "%02x %02x %02x %02x %02x %02x %02x %02x  %02x %02x %02x %02x %02x %02x %02x %02x   ",
                l[0], l[1], l[2], l[3], l[4], l[5], l[6], l[7],
                l[8], l[9], l[10], l[11], l[12], l[13], l[14], l[15]);
        fprintf(stderr, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
                toc(l[0]), toc(l[1]), toc(l[2]), toc(l[3]), toc(l[4]),
                toc(l[5]), toc(l[6]), toc(l[7]), toc(l[8]), toc(l[9]),
                toc(l[10]), toc(l[11]), toc(l[12]), toc(l[13]), toc(l[14]),
                toc(l[15]));
        l += 16;
        index += 16;
    }
    if (index != len)
    {
        fprintf(stderr, "%04lx  ", index);
        for(int i = 0; i < 16; i++)
        {
            if (i+index < len) fprintf(stderr, "%02x ", l[i]);
            else fprintf(stderr, "   ");
            if (i == 7) fprintf(stderr, " ");
        }
        fprintf(stderr, "  ");
        for(int i = 0; i < 16; i++)
        {
            if (i+index < len) fprintf(stderr, "%c", toc(l[i]));
            else fprintf(stderr, " ");
        }
        fprintf(stderr, "\n");
    }
    Notify(PRT_NOTICE, "HexDump ~fin~");
    return len;
}
