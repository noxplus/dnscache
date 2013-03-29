#ifndef _UTIL_H_
#define _UTIL_H_


#ifdef __linux__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <pthread.h>
#endif
#ifdef _WIN32
#define _CRT_RAND_S
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

typedef signed long     int32;
typedef signed short    int16;
typedef signed char     int8;
typedef unsigned int    uint;
typedef unsigned long   uint32;
typedef unsigned short  uint16;
typedef unsigned char   uint8;

//颜色代码
#ifdef __linux__
#define NONE        "\033[m"
#define RED         "\033[0;31m"
#define LRED        "\033[1;31m"
#define GREEN       "\033[0;32m"
#define LGREEN      "\033[1;32m"
#define BROWN       "\033[0;33m"
#define YELLOW      "\033[1;33m"
#define BLUE        "\033[0;34m"
#define LBLUE       "\033[1;34m"
#define PURPLE      "\033[0;35m"
#define LPURPLE     "\033[1;35m"
#define CYAN        "\033[0;36m"
#define LCYAN       "\033[1;36m"
#define LGRAY       "\033[0;37m"
#define WHITE       "\033[1;37m"
#endif
#ifdef _WIN32
#define LIGHT       FOREGROUND_INTENSITY
#define RED         FOREGROUND_RED
#define GREEN       FOREGROUND_GREEN
#define BLUE        FOREGROUND_BLUE
#define LRED        (RED | LIGHT)
#define LGREEN      (GREEN | LIGHT)
#define BROWN       (RED | GREEN)
#define YELLOW      (BROWN | LIGHT)
#define LBLUE       (BLUE | LIGHT)
#define PURPLE      (RED | BLUE)
#define LPURPLE     (PURPLE | LIGHT)
#define CYAN        (GREEN | BLUE)
#define LCYAN       (CYAN | LIGHT)
#define LGRAY       (RED | GREEN | BLUE)
#define WHITE       (LGRAY | LIGHT)
#endif

#define PRT_ERROR   0
#define PRT_NOTICE  1
#define PRT_WARNING 2
#define PRT_INFO    3
#define PRT_DEBUG   4
#define PRT_TEST    5

#ifdef __GNUC__
#define PACKED __attribute__((packed))
#endif
#ifdef _MSC_VER
//
#endif

typedef enum _err_no
{
    ERR_no = 0x77000000,
    ERR_net_sock_error,
    ERR_net_bind_error,
    ERR_net_connect_error,
    ERR_net_send_error,
    ERR_net_recv_error,
    ERR_net_select_error,
    ERR_net_connect_timeout,
    ERR_net_send_timeout,
    ERR_net_recv_timeout,
    ERR_net_select_timeout,
    ERR_buf_malloc_fail,
    ERR_dns_query_toomany,
    ERR_dns_answer_toomany,
    ERR_end
}ErrNo;

//util

uint32 random32(void);

uint32 GetTimeMs(void);

void SleepMS(uint32);

const char* err2str(int);

int Notify(int, const char*, ...);

int HexDump(char*, uint32);
#endif
