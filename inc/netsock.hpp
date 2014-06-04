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

#ifdef _WIN32
#define SelSck(sock) (0)
#endif
#ifdef __linux__
#define SelSck(sock) (sock+1)
#endif

typedef union
{
    uint32  ipv4;
    uint8   ipc[4];
}IPv4;

#pragma pack(push)
#pragma pack(1)
typedef struct
{
    uint8       ContentType;    //==22 Handshake
    uint16      SSLVer;         //==0x0301 TLS1.0
    uint16      ContentLen;     //使用网络字节序
}SSLHead;

typedef struct
{
    uint8       HandshakeType;  //==0x01 Client Hello
    uint8       LenHi;          //长度高8位
    uint16      LenLo;          //长度低16位
}SSLHSHead;

typedef struct
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
}SSLCliHello;
#pragma pack(pop)

class IPBlock
{
    private:
        IPv4*   m_IPnet;  //IP子网
        IPv4*   m_IPmask; //子网掩码

        uint32  m_blockcnt;
    public:
        IPBlock(void);//初始化为空
        IPBlock(const char*);//使用字符串初始化
        int Init(const char*);//增加网段
        ~IPBlock(void);//回收

        uint32 GetCnt(void);

        uint32 GetIdxRandIP(int);//获得指定子网内随机IP
        uint32 GetRandIP(void);//获得一个子网内随机IP
};

#define SCKTP_NONE  0x0
#define SCKTP_UDP   0x1
#define SCKTP_TCP   0x2
#define SCKTP_THIS  0x4
#define SCKTP_COPY  0x8

class Network
{
protected:
        int m_scktype;
        int m_sock;
        struct sockaddr_in  remote;
public:
        void    Close(void);  //关闭连接
        int     SetSockBlock(bool); //设定socket的阻塞/非阻塞
        void    SetIPPort(uint32, uint16); //设定服务器的IP、端口
        void    SetIPPort(const char*, uint16);//设定服务器的IP、端口
        void    SetIP(uint32);//设定服务器的IP
        void    SetIP(const char*);//设定服务器IP
        int     GetSocket(void);//获取socket
        int     SetSocket(int);//设置socket
};

//支持服务端、客户端
class NetUDP : public Network
{
    public:
        NetUDP();
        ~NetUDP();
        NetUDP(int);
        NetUDP(NetUDP&);
        int     copy(NetUDP&);
        int     UDPBind(uint16);//port
        int     UDPSend(const char*, int, int);//发送数据 数据，长度，超时
        int     UDPRecv(char*, int, int);//接收数据 数据，长度，超时
};

//只支持客户端
class NetTCP : public Network
{
    public:
        NetTCP();
        ~NetTCP();
        NetTCP(NetTCP&);

        int     TCPConnect(int); //连接到远端。参数：超时
        int     TCPSend(const char*, int, int);//发送数据 数据，长度，超时
        int     TCPRecv(char*, int, int);//接收数据 数据，长度，超时
        int     TCPClear(int);//在指定时间内，清空socket接收缓存

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


inline int Network::GetSocket(void)
{
    return m_sock;
}
inline int Network::SetSocket(int sock)
{
    m_scktype &= ~SCKTP_THIS;
    m_scktype |= SCKTP_COPY;
    return m_sock = sock;
}
//设定ip、port
inline void Network::SetIPPort(uint32 ip, uint16 port)
{
    memset(&remote, 0, sizeof(remote));
    remote.sin_family = AF_INET;
    remote.sin_port = htons(port);
    remote.sin_addr.s_addr = ip;
}
inline void Network::SetIPPort(const char* ip, uint16 port)
{
    memset(&remote, 0, sizeof(remote));
    remote.sin_family = AF_INET;
    remote.sin_port = htons(port);
    remote.sin_addr.s_addr = inet_addr(ip);
}
inline void Network::SetIP(uint32 ip)
{
    remote.sin_addr.s_addr = ip;
}
inline void Network::SetIP(const char* ip)
{
    remote.sin_addr.s_addr = inet_addr(ip);
}
//设置socket阻塞/非阻塞
inline int Network::SetSockBlock(bool block)
{
    if (m_sock < 0) return false;

#ifdef _WIN32
    unsigned long mode = (block == true ? 0 : 1);
    return (ioctlsocket(m_sock, FIONBIO, &mode) == 0) ? true : false;
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
