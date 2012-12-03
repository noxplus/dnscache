#ifndef _TESTGG_H_
#define _TESTGG_H_

typedef struct _in_SSL_HEAD
{
    uint8   ContentType;    //==22 Handshake
    uint16  SSLVer;         //==0x0301 TLS1.0
    uint16  ContentLen;     //ʹ�������ֽ���
}PACKED SSLHead;
typedef struct _in_HandShake_HEAD
{
    uint8   HandshakeType;  //==0x01 Client Hello
    uint8   LenHi;          //���ȸ�8λ
    uint16  LenLo;          //���ȵ�16λ
}PACKED HSHead;
typedef struct _tp_SSL_CLI_HELLO
{
    SSLHead ssl;
    HSHead  hands;
    uint16  SSLVer;         //==0x0301 TLS1.0
    uint8   rand[32];       //32�ֽ��������ǰ4�ֽ���ʱ��
    uint8   sidLen;         //0
    uint16  CipherSuitesLen;    //ǿ��ʹ�ó���Ϊ1
    uint16  CipherSuites;   //�����Ϊ����Ϊ1�����顣
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
