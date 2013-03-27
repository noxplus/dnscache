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

    bool operator<(const QName&) const;
    bool operator>(const QName&) const;
    bool operator==(const QName&) const;

    int getlen(void) const;
    int clear(void);
    int copyto(char*);
    void print(void) const;
};

class dnsutil
{
private:
    QName*  m_Qname;    //
    char*   m_srvbuf;   //前2字节表长度//host byte order
    char*   m_clibuf;   //前2字节表长度//host byte ordef
    DNSRecord*  m_DNSRec;
    uint32  buf_alloc;//构造/析够时，是否malloc/free
    DNSHead m_head;
public:

    //根据m_Qname，组织查询报文到m_clibuf
    int packQuery(void);

    //根据m_Qname, m_DNSRec, 组织回复报文到m_srvbuf
    int packAnswer(void);

    //解析m_srvbuf到m_dnshead, m_Qname
    int unpackQuery(void);

    //解析m_clibuf到m_DNSRec
    int unpackAnswer(void);
};

class MemCache
{
protected:
    std::map<QName, DNSRecord>  cache;
public:
    void* search(const QName&);
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

class Server
{
    StaticCache srv_add;//查询网址的dns服务器
    StaticCache srv_dom;//查询域名的dns服务器
    StaticCache host_add;//存储网址的IP地址
    StaticCache host_dom;//存储域名的IP地址
    DynamicCache dnscache;//存储查询的缓存数据
};

#endif
