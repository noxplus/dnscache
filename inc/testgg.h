#ifndef _TESTGG_H_
#define _TESTGG_H_

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

//test google
unsigned int initTest(int Cnt, char** list);

int tconn(unsigned int);

int tssl(int);

bool SetSocketBlock(int, bool);

#endif
