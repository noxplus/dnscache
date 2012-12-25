#ifndef _TESTGG_H_
#define _TESTGG_H_

#include "netsock.hpp"

typedef struct _tp_ip_time
{
    IPv4    ipaddr;
    uint32  timeout;
}GIPTime;

//test google
unsigned int initTest(void);

int tconn(unsigned int);

int tssl(int);

#endif
