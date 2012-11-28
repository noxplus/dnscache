#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

typedef signed long     int32;
typedef signed short    int16;
typedef signed char     int8;
typedef unsigned long   uint32;
typedef unsigned short  uint16;
typedef unsigned char   uint8;

//printf颜色代码
#define NONE        "\033[m"
#define RED         "\033[0;31m"
#define LRED        "\033[1;31m"
#define GREEN       "\033[0;32m"
#define LGREEN      "\033[1;32m"
#define BROWN       "\033[0;33m"
#define YELLOW      "\033[1;33m"
#define BLUE        "\033[0;34m"
#define LBLUE       "\033[1;34m"
#define PURPLE      "\033[0;35m"
#define LPURPLE     "\033[1;35m"
#define CYAN        "\033[0;36m"
#define LCYAN       "\033[1;36m"
#define LGRAY       "\033[0;37m"
#define WHITE       "\033[1;37m"

#define PRT_ERROR   0
#define PRT_NOTICE  1
#define PRT_WARNING 2
#define PRT_INFO    3
#define PRT_DEBUG   4
#define PRT_TEST    5

#define NAMEMAXLEN  64

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


typedef struct _tp_rbtree_node RBNode;
struct _tp_rbtree_node
{
    RBNode*     Parent;
    RBNode*     Left;
    RBNode*     Right;
    DNSRecode*  Value;
    uint32      Color;
};
typedef struct _tp_rbtree_root
{
    RBNode*     Root;
}RBRoot;

typedef struct _tp_local_query
{
    int     sktfd;
    struct sockaddr_in localadd;
    uint16  dlen;
    char    data[512];
}LocalQuery;

//RBTree
void* RBTreeSearch(RBRoot*, DNSRecode*);
int RBTreeInsert(RBRoot*, DNSRecode*);

//test google

//transdns
int addr2dns(char* dns, char* addr);
int dns2addr(char* addr, char* dns);
uint32 GenIndex(DNSRecode*);
int unpackQuery(char*, QueryRec**);
int unpackAnswer(char*, AnswerRec**);
int udpquery(char*, int);

//util
int Notify(char*, int, int, char*, ...);
