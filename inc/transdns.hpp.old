#ifndef _TRANSDNS_HPP_
#define _TRANSDNS_HPP_

#include "netsock.hpp"

typedef struct
{
    uint16  TranID;
    uint16  Flags;
    uint16  Quests;
    uint16  Ansers;
    uint16  Auths;
    uint16  Addits;
}DnsHead;

typedef struct
{
    uint8   NameLen;
    char    Name[NAMEMAXLEN];
    uint16  Type;
    uint16  Class;
}DnsQueryRec;
typedef struct
{
    uint8   NameLen;
    char    Name[NAMEMAXLEN];
    uint16  Type;
    uint16  Class;
    uint32  TTL;
    uint16  Addlen;
    IPv4    IPadd;
}DnsAnswerRec;

typedef struct
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

class transdns : public NetUDP
{
    private:
        char    querydata[1024];//

    public:
        transdns(int);
}

#endif
