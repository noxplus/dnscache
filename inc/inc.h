#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <errno.h>

typedef signed long     int32;
typedef signed short    int16;
typedef signed char     int8;
typedef unsigned long   uint32;
typedef unsigned short  uint16;
typedef unsigned char   uint8;

#define PACKED __attribute__((packed))

typedef union _tp_IPv4
{
    uint32  ipv4;
    uint8   ipc[4];
}IPv4;


typedef struct _in_SSL_HEAD
{
    uint8   ContentType;    //==22 Handshake
    uint16  SSLVer;         //==0x0301 TLS1.0
    uint16  ContentLen;     //使用网络字节序
}PACKED SSLHead;
typedef struct _in_HandShake_HEAD
{
    uint8   HandshakeType;  //==0x01 Client Hello
    uint8   LenHi;          //长度高8位
    uint16  LenLo;          //长度低16位
}PACKED HSHead;
typedef struct _tp_SSL_CLI_HELLO
{
    SSLHead ssl;
    HSHead  hands;
    uint16  SSLVer;         //==0x0301 TLS1.0
    uint8   rand[32];       //32字节随机数。前4字节是时间
    uint8   sidLen;         //0
    uint16  CipherSuitesLen;    //强制使用长度为1
    uint16  CipherSuites;   //这里简化为长度为1的数组。
    uint8   CompMethLen;    //1
    uint8   CompMeth;       //==0 (null)
    //extension
}PACKED SSLCliHello;

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
    char    name[64];
    uint16  type;
    uint16  class;
}Query;
typedef struct _tp_dns_arecd
{
    uint8   namelen;
    char    name[64];
    uint16  type;
    uint16  class;
    uint32  ttl;
    uint16  Addlen;
    IPv4    ipadd;
}Answer;

typedef struct _tp_dns_recode
{
    uint32 index;   //
    uint32 type;    //
    int32 ttl;      //dns time to live
    union _un_name
    {
        char cname[16];
        int32 iname[4];
    }uname;
    IPv4 ip;
}DNS_Recode, *pDNS_Recode;


typedef struct _tp_rbtree_node RBNode;
struct _tp_rbtree_node
{
    RBNode*     Parent;
    RBNode*     Left;
    RBNode*     Right;
    uint32      Key;
    uint32      Color;
};
typedef struct _tp_rbtree_root
{
    RBNode*     Root;
}RBRoot;
