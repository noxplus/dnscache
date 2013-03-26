#ifndef _DNSUTIL_H_
#define _DNSUTIL_H_

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
    uint32  index;  //cname的hash
    uint32  ttl;    //记录过期时间
    //union
    //{
    //    char cname[DNSNAMEMAXLEN];
    //    int32 iname[DNSNAMEMAXLEN/sizeof(int32)];
    //}uname;
    IPv4 ip;
}DNSRecord;

int addr2dns(char* dns, char* addr);
int dns2addr(char* addr, char* dns);

class dnsutil
{
private:
    char*   m_Qname;    //前2字节表长度//host byte ordef
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

class LocalCache
{
protected:
    int m_type;
    std::Map<int, DNSRecord>  cache;
public:
};

//从配置文件载入的缓存数据
class StaticCache : public : LocalCache
{};
//动态获取的缓存数据
class DynamicCache : public : LocalCache
{};

#endif
