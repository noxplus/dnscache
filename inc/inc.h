#include <stdio.h>

typedef signed int      int32;
typedef signed short    int16;
typedef signed char     int8;
typedef unsigned int    uint32;
typedef unsigned short  uint16;
typedef unsigned char   uint8;

typedef _tp_dns_recode
{
    int32 index;    //
    int32 type;     //
    int32 ttl;      //dns time to live
    union _un_name
    {
        char cname[16];
        int32 iname[4];
    }uname;
    int32 ipv4;
}DNS_Recode, *pDNS_Recode;
