#ifndef _DNSUTIL_H_
#define _DNSUTIL_H_

#include "netsock.hpp"

#define DNSNAMEMAXLEN  64
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
    uint32 index;   //
    uint32 type;    //
    int32 ttl;      //dns time to live
    union _un_name
    {
        char cname[DNSNAMEMAXLEN];
        int32 iname[DNSNAMEMAXLEN/sizeof(int32)];
    }uname;
    IPv4 ip;
}DNSRecode, *pDNSRecode;

class dnsutil
{
private:
    char*   m_Qname;//前2字节表长度//host byte ordef
    char*   m_AIP;//前2字节表长度//host byte order
    char*   m_srvbuf;//前2字节表长度//host byte order
    char*   m_clibuf;//前2字节表长度//host byte ordef
    uint32  buf_alloc;//构造/析够时，是否malloc/free
    DNSHead m_head;
public:

    //根据m_dnshead, m_Qname，组织查询报文到m_clibuf
    int packQuery();

    //根据m_dnshead, m_Qname, m_AIP, 组织回复报文到m_srvbuf
    int packAnswer();

    //解析m_srvbuf到m_dnshead, m_Qname
    int unpackQuery();

    //解析m_clibuf到m_AIP
    int unpackAnswer();
};
int addr2dns(char* dns, char* addr);
int dns2addr(char* addr, char* dns);

#endif
