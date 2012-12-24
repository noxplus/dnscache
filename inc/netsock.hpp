#ifdef __linux__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <pthread.h>
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

#define NAMEMAXLEN  64



#ifdef __GNUC__
#define PACKED __attribute__((packed))
#endif

typedef union _tp_IPv4
{
    uint32  ipv4;
    uint8   ipc[4];
}IPv4;

typedef struct _tp_dns_head
{
    uint16  TranID;
    uint16  Flags;
    uint16  Quests;
    uint16  Ansers;
    uint16  Auths;
    uint16  Addits;
}DnsHead;

typedef struct _tp_dns_qrecd
{
    uint8   NameLen;
    char    Name[NAMEMAXLEN];
    uint16  Type;
    uint16  Class;
}DnsQueryRec;
typedef struct _tp_dns_arecd
{
    uint8   NameLen;
    char    Name[NAMEMAXLEN];
    uint16  Type;
    uint16  Class;
    uint32  TTL;
    uint16  Addlen;
    IPv4    IPadd;
}DnsAnswerRec;

typedef struct _tp_dns_recode
{
    uint32 Index;   //
    uint32 Type;    //
    int32 TTL;      //dns time to live
    union _un_name
    {
        char cname[NAMEMAXLEN];
        int32 iname[NAMEMAXLEN/sizeof(int32)];
    }uName;
    IPv4 ip;
}DNSRecode, *pDNSRecode;

typedef struct _tp_local_query
{
    int         sktfd;
    struct sockaddr_in localadd;
    uint16      dlen;
    char        data[512];
}LocalQuery;

typedef struct _in_SSL_HEAD
{
    uint8       ContentType;    //==22 Handshake
    uint16      SSLVer;         //==0x0301 TLS1.0
    uint16      ContentLen;     //使用网络字节序
}PACKED SSLHead;
typedef struct _in_HandShake_HEAD
{
    uint8       HandshakeType;  //==0x01 Client Hello
    uint8       LenHi;          //长度高8位
    uint16      LenLo;          //长度低16位
}PACKED SSLHSHead;
typedef struct _tp_SSL_CLI_HELLO
{
    SSLHead     ssl;
    SSLHSHead   hands;
    uint16      SSLVer;         //==0x0301 TLS1.0
    uint8       rand[32];       //32字节随机数。前4字节是时间
    uint8       sidLen;         //0
    uint16      CipherSuitesLen;    //强制使用长度为1
    uint16      CipherSuites;   //这里简化为长度为1的数组。
    uint8       CompMethLen;    //1
    uint8       CompMeth;       //==0 (null)
    //extension
}PACKED SSLCliHello;

typedef struct _tp_ip_time
{
    IPv4    ipaddr;
    uint32  timeout;
}IPTime;


class NetTCP
{
private:
    int                 m_sock;
    struct sockaddr_in  remote;
    struct sockaddr_in  local;

public:
    NetTCP();
    ~NetTCP();

    int     TCPConnect(int);
    void    TCPClose(void);
    int     TCPSend(char*, int, int);
    int     TCPRecv(char*, int, int);

    int     SetSockBlock(bool);
    void    SetIPPort(uint32, uint16);
    void    SetIPPort(const char*, uint16);
    void    SetIP(uint32);
    void    SetIP(const char*);
};

class SSLTest : public NetTCP
{
private:
    SSLCliHello     m_hello;
    int             m_connect_timeout;
    int             m_SSL_send_timeout;
    int             m_SSL_recv_timeout;

public:
    SSLTest();
    ~SSLTest();

    int RunTest(void);
};

//设定ip、port
inline void NetTCP::SetIPPort(uint32 ip, uint16 port)
{
    memset(&remote, 0, sizeof(remote));
    remote.sin_family = AF_INET;
    remote.sin_port = htons(port);
    remote.sin_addr.s_addr = ip;
};
inline void NetTCP::SetIPPort(const char* ip, uint16 port)
{
    memset(&remote, 0, sizeof(remote));
    remote.sin_family = AF_INET;
    remote.sin_port = htons(port);
    remote.sin_addr.s_addr = inet_addr(ip);
};
inline void NetTCP::SetIP(uint32 ip)
{
    remote.sin_addr.s_addr = ip;
};
inline void NetTCP::SetIP(const char* ip)
{
    remote.sin_addr.s_addr = inet_addr(ip);
};
//设置socket阻塞/非阻塞
inline int NetTCP::SetSockBlock(bool block)
{
    if (m_sock < 0) return false;

#ifdef _WIN32
    unsigned long mode = (block == true ? 0 : 1);
    return (ioctlsocket(fd, FIONBIO, &mode) == 0) ? true : false;
#endif
#ifdef __linux__
    int flags;
    if ((flags = fcntl(m_sock, F_GETFL, 0)) < 0) return false;
    flags = block == true ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
    return (fcntl(m_sock, F_SETFL, flags) == 0) ? true : false;
#endif
};


uint32 random32(void);
int Notify(int, const char*, ...);

