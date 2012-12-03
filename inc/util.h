#ifndef _UTIL_H_
#define _UTIL_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

typedef signed long     int32;
typedef signed short    int16;
typedef signed char     int8;
typedef unsigned long   uint32;
typedef unsigned short  uint16;
typedef unsigned char   uint8;

//printf��ɫ����
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

#define PRT_ERROR   0
#define PRT_NOTICE  1
#define PRT_WARNING 2
#define PRT_INFO    3
#define PRT_DEBUG   4
#define PRT_TEST    5

#define NAMEMAXLEN  64

#define PACKED __attribute__((packed))

typedef union _tp_IPv4
{
    uint32  ipv4;
    uint8   ipc[4];
}IPv4;

//util

uint32 random32(void);

int Notify(int, char*, ...);

#endif