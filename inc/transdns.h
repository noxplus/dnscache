#ifndef _TRANDSDNS_H_
#define _TRANDSDNS_H_

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
    uint8   namelen;
    char    name[NAMEMAXLEN];
    uint16  type;
    uint16  class;
}QueryRec;
typedef struct _tp_dns_arecd
{
    uint8   namelen;
    char    name[NAMEMAXLEN];
    uint16  type;
    uint16  class;
    uint32  ttl;
    uint16  Addlen;
    IPv4    ipadd;
}AnswerRec;

typedef struct _tp_dns_recode
{
    uint32 index;   //
    uint32 type;    //
    int32 ttl;      //dns time to live
    union _un_name
    {
        char cname[NAMEMAXLEN];
        int32 iname[NAMEMAXLEN/sizeof(int32)];
    }uname;
    IPv4 ip;
}DNSRecode, *pDNSRecode;

typedef struct _tp_local_query
{
    int     sktfd;
    struct sockaddr_in localadd;
    uint16  dlen;
    char    data[512];
}LocalQuery;

//transdns
int addr2dns(char* dns, char* addr);
int dns2addr(char* addr, char* dns);
uint32 GenIndex(DNSRecode*);
int unpackQuery(char*, QueryRec**);
int unpackAnswer(char*, AnswerRec**);
int udpquery(char*, int);

#endif
