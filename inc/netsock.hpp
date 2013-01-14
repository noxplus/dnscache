#ifndef _NETSOCK_H_
#define _NETSOCK_H_

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

#ifdef __GNUC__
#define PACKED __attribute__((packed))
#endif

typedef enum _err_no
{
    ERR_no = 77000000,
    ERR_sock_error,
    ERR_conn_error,
    ERR_send_error,
    ERR_recv_error,
    ERR_conn_timeout,
    ERR_send_timeout,
    ERR_recv_timeout
}ErrNo;

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


class NetIP
{
    private:
        IPv4*   IPnet;  //IP子网
        IPv4*   IPmask; //子网掩码
    public:
        NetIP(void);//初始化为空
        NetIP(const char*);//使用字符串初始化
        ~NetIP(void);//回收

        int AddSubNet(const char*);//增加网段
        uint32 GetRandIP(void);//获得一个子网内随机IP
};

class NetTCP
{
    private:
        int                 m_sock; //使用的socket
        struct sockaddr_in  remote; //远端地址信息
        struct sockaddr_in  local;  //本地地址信息

    public:
        NetTCP();
        ~NetTCP();

        int     TCPConnect(int); //连接到远端。参数：超时
        void    TCPClose(void);  //关闭连接
        int     TCPSend(char*, int, int);//发送数据 数据，长度，超时
        int     TCPRecv(char*, int, int);//接收数据 数据，长度，超时

        int     SetSockBlock(bool); //设定socket的阻塞/非阻塞
        void    SetIPPort(uint32, uint16); //设定服务器的IP、端口
        void    SetIPPort(const char*, uint16);//设定服务器的IP、端口
        void    SetIP(uint32);//设定服务器的IP
        void    SetIP(const char*);//设定服务器IP
};

class SSLTest : public NetTCP
{
private:
    SSLCliHello     m_hello; //ssh报文
    int             m_connect_timeout; //连接超时
    int             m_SSL_send_timeout; //发送超时
    int             m_SSL_recv_timeout; //接收超时 

public:
    SSLTest();
    ~SSLTest();

    void SetTimeout(int, int, int);
    int RunTest(void);
    int RunTest(const char*);
    int RunTest(uint32);
};

//设定ip、port
inline void NetTCP::SetIPPort(uint32 ip, uint16 port)
{
    memset(&remote, 0, sizeof(remote));
    remote.sin_family = AF_INET;
    remote.sin_port = htons(port);
    remote.sin_addr.s_addr = ip;
}
inline void NetTCP::SetIPPort(const char* ip, uint16 port)
{
    memset(&remote, 0, sizeof(remote));
    remote.sin_family = AF_INET;
    remote.sin_port = htons(port);
    remote.sin_addr.s_addr = inet_addr(ip);
}
inline void NetTCP::SetIP(uint32 ip)
{
    remote.sin_addr.s_addr = ip;
}
inline void NetTCP::SetIP(const char* ip)
{
    remote.sin_addr.s_addr = inet_addr(ip);
}
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
}

inline int SSLTest::RunTest(uint32 ip)
{
    SetIP(ip);
    return RunTest();
}
inline int SSLTest::RunTest(const char *ip)
{
    SetIP(ip);
    return RunTest();
}

#endif
