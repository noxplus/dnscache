#ifndef _TESTGG_H_
#define _TESTGG_H_

#include <string>
#include <list>
#include "netsock.hpp"


typedef struct type_ggHostCFG
{
    uint32         ConnTimeout;//默认1000ms
    uint32         SSLTimeout;//默认1000ms
    uint32         HostIPCnt; //默认：0～cnt-1存地址，cnt存新入的。共cnt+1
    const char*    BakFile;
    const char*    IPBlocks;
    uint32         ChkInter;
    uint32         TestInter;
}ggHostCFG;


//获取的记录
class ggRec
{
    private:
    IPv4    ipaddr; //
    uint32  timeout;
    std::string  cert;

    public:
    ggRec(uint32 ip, uint32 time = (uint32)-1)
    {
        ipaddr.ipv4 = ip;
        timeout = time;
    }
    ~ggRec(void){}
    bool operator>(const ggRec&);
    bool operator<(const ggRec&);
    bool operator==(const ggRec&);
    void print(FILE* fw = stdout);
    void SetIPAddr(uint32);
    void SetTimeout(uint32);
    uint32 GetIPAddr(void);
    uint32 GetTimeout(void);
    void tostr(char*, int);
};

typedef std::list<ggRec>            ggList;
typedef std::list<ggRec>::iterator  ggListIter;

class ggTest : public SSLTest
{
    private:
    ggList      m_list;
    ggHostCFG   m_cfg;
    int         m_IPBlockCnt;
    IPv4*       m_ipHead;
    IPv4*       m_ipMask;
    uint32      m_next_test;
    uint32      m_next_check;

    public:
    ggTest(void);
    ~ggTest(void);
    void InitCfg(void);
    void ParseArg(int, char**);
    void InitTest(void);
    void Load2Mem(void);
    void Save2File(void);
    void CheckFunc(void);
    void TestFunc(void);
    void LoopFunc(void);
};

#endif
