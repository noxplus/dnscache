#ifndef _TESTGG_H_
#define _TESTGG_H_

#include <string>
#include <list>
#include "netsock.hpp"

class ggRec
{
    private:
    IPv4    ipaddr;
    uint32  timeout;
    std::string  cert;

    public:
    ggRec(uint32 ip = 0L, uint32 time = (uint32)-1)
    {
        ipaddr.ipv4 = ip;
        timeout = time;
    }
    ~ggRec(void){}
    bool operator>(const ggRec&);
    bool operator<(const ggRec&);
    bool operator==(const ggRec&);
};

class ggHostCFG
{
    private:
    int32          Connect_Timeout;//默认1000ms
    int32          SSL_Timeout;//默认1000ms
    int32          HostIPCnt; //默认：0～cnt-1存地址，cnt存新入的。共cnt+1
    const char*    BakFile;
    const char*    IPBlocks;
    int32          Time_to_Check;
    int32          Time_Sleepms;

    public:
    ggHostCFG(void);
    ~ggHostCFG(void){}
    void ReadCfg(const char*);
    void ParseArg(int, char**);
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
    time_t      m_checkTime;

    public:
    ggTest(void);
    ~ggTest(void){}
    void initTest(int, char**);
    void load2mem(void);
    void save2file(void);
    void checkall(void);
    void looopfunc(void);
};

//test google
unsigned int initTest(void);

int tconn(unsigned int);

int tssl(int);

#endif
