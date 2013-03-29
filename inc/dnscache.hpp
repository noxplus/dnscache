#ifndef _DNSUTIL_H_
#define _DNSUTIL_H_

#include <string>
#include <map>

#include "netsock.hpp"

#define DNSNAMEMAXLEN  128
#define DNSQUERYMAXREC 10
#define DNSANSWERMAXREC 30

typedef struct
{
    uint16  TranID;
    uint16  Flags;
    uint16  Quests;
    uint16  Ansers;
    uint16  Auths;
    uint16  Addits;
}DNSHead;

typedef struct
{
    uint8   NameLen;
    char    Name[DNSNAMEMAXLEN];
    uint16  Type;
    uint16  Class;
}QueryRec;
typedef struct
{
    uint8   NameLen;
    char    Name[DNSNAMEMAXLEN];
    uint16  Type;
    uint16  Class;
    uint32  TTL;
    uint16  Addlen;
    IPv4    IPAdd;
}AnswerRec;

typedef struct
{
    uint32  slen;   //结构体长度
    uint32  ttl;    //记录过期时间
    IPv4 ip;
}DNSRecord;

int addr2dns(char* dns, char* addr);
int dns2addr(char* addr, char* dns);

class QName
{
    int     m_len;
    char    m_name[DNSNAMEMAXLEN];
public:
    QName(void);
    QName(const char*);//字符串归一小写
    QName(const QName&);

    int reset(const char*);//字符串会归一全小写
    int reset(const QName&);
    int GetSubName(QName&);
    int Cut(void);

    bool operator<(const QName&) const;
    bool operator>(const QName&) const;
    bool operator==(const QName&) const;

    int getlen(void) const;
    int clear(void);
    int copyto(char*);
    void print(void) const;
};

#define STA_NONE    0x00
#define STA_INIT    0x01    //初始化（非none）
#define STA_SRV     0x02    //
#define STA_FND     0x04    //收到新数据
#define STA_CHK     0x08    //检测TTL
#define STA_ACK     0x10    //回复客户端

class DNSutil
{
private:
    char    m_srvbuf[1024];   //前2字节表长度//host byte order
    char    m_clibuf[1024];   //前2字节表长度//host byte ordef
    uint16  m_srvID;    //srv获取的客户端ID
    uint32  m_sta;
    uint32  m_clilen;
    uint32  m_srvlen;
    IPv4    m_dnssrv;
    DNSRecord  m_DNSRec;
    QName   m_Qname;    //
    NetUDP  m_cli;
    NetUDP  m_srv;
protected:
public:
    //根据m_Qname，组织查询报文到m_clibuf
    int packQuery(void);

    //根据m_Qname, m_DNSRec, 组织回复报文到m_srvbuf
    int packAnswer(void);

    //解析m_srvbuf到m_dnshead, m_Qname
    int unpackQuery(void);

    //解析m_clibuf到m_DNSRec
    int unpackAnswer(void);

    //作为服务端，局域网收发数据
    int SrvRecv(NetUDP&);
    int SrvSend();

    //作为客户端，从网络收发数据
    int CliSend(void);
    int CliRecv(void);

    int GetSta(void);
    int SetUse(void);
    int Release(void);

    void Search(void);
    void DNSServer(void);
    void UpdCache(void);
};

class MemCache
{
protected:
    std::map<QName, DNSRecord>  cache;
public:
    DNSRecord* search(const QName&);
    int update(const QName&, const DNSRecord&);
};

//从配置文件载入的缓存数据
class StaticCache : public MemCache
{
public:
    int init(char*);//from file
};
//动态获取的缓存数据
class DynamicCache : public MemCache
{
public:
    int save(char*);//file
    int load(char*);//file
};

#endif
