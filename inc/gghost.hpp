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

typedef std::list<ggRec>            ggList;
typedef std::list<ggRec>::iterator  ggListIter;

class ggTest : public SSLTest
{
    ggList  iplist;
};

//test google
unsigned int initTest(void);

int tconn(unsigned int);

int tssl(int);

#endif
