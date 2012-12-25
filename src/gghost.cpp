#include "util.hpp"
#include "gghost.hpp"


typedef struct _cfg_gghost_globa
{
    int32          Connect_Timeout;//默认1000ms
    int32          SSL_Timeout;//默认1000ms
    int32          HostIPCnt; //默认：0～cnt-1存地址，cnt存新入的。共cnt+1
    const char*     BakFile;
    const char*     IPBlocks;
    int32          Time_to_Check;
    int32          Time_Sleepms;
}CFG_GGHost;

static IPv4*    IPh = NULL;
static IPv4*    Mask = NULL;
static GIPTime  *iptbl;//存储速度最快的IP地址
static int      IPBlockCnt = 0;
static time_t   Check_Time = 0;//

CFG_GGHost      HostCfg;

SSLTest ssltest;


int save2file(void)
{
    FILE* fw = fopen(HostCfg.BakFile, "w");
    if (fw == NULL) return 0;
    
    int i;
    for (i = 0; i < HostCfg.HostIPCnt; i++)
    {
        fprintf(fw, "%d.%d.%d.%d\t%lu\n",
                iptbl[i].ipaddr.ipc[0], iptbl[i].ipaddr.ipc[1], 
                iptbl[i].ipaddr.ipc[2], iptbl[i].ipaddr.ipc[3],
                iptbl[i].timeout);
    }

    fclose(fw);
    return i;
}

int load2mem(void)
{
    FILE* fr = fopen(HostCfg.BakFile, "r");
    if (fr == NULL) return 0;
    char line[1024];

    int i, iret;
    for (i = 0; i < HostCfg.HostIPCnt; i++)
    {
        int a,b,c,d;
        if (fgets(line, sizeof(line), fr) == NULL) break;
        iret = sscanf(line, "%d.%d.%d.%d", &a, &b, &c, &d);
        if (iret != 4) break;
        iptbl[i].ipaddr.ipc[0] = a;
        iptbl[i].ipaddr.ipc[1] = b;
        iptbl[i].ipaddr.ipc[2] = c;
        iptbl[i].ipaddr.ipc[3] = d;
    }
    fclose(fr);
    for (;i < HostCfg.HostIPCnt; i++)
    {
        iptbl[i].ipaddr.ipv4 = 0x0;
        iptbl[i].timeout = HostCfg.SSL_Timeout;
    }
    return HostCfg.HostIPCnt;
}

//1、重新获取所有的超时
//2、排序
void check_all(void)
{
    int iret;
    int i, j;

    time(&Check_Time);

    for (i = 0; i < HostCfg.HostIPCnt; i++)//reset
    {
        iret = tconn(iptbl[i].ipaddr.ipv4);
        if (iret > 0 && iret < HostCfg.SSL_Timeout)
            iptbl[i].timeout = iret;
        else iptbl[i].timeout = HostCfg.SSL_Timeout;
        Notify(PRT_NOTICE, "check[%d][%d.%d.%d.%d]+[.%d]", i, 
                iptbl[i].ipaddr.ipc[0], iptbl[i].ipaddr.ipc[1],
                iptbl[i].ipaddr.ipc[2], iptbl[i].ipaddr.ipc[3],
                iptbl[i].timeout);
    }

    //冒泡排序
    for (i = HostCfg.HostIPCnt - 1; i > 0; i--)
    {
        iret = 0;
        for (j = 0; j < i; j++)
        {
            if (iptbl[j].timeout > iptbl[j+1].timeout)
            {//swap
                iret++;
                memcpy(&iptbl[HostCfg.HostIPCnt], &iptbl[j], sizeof(GIPTime));
                memcpy(&iptbl[j], &iptbl[j+1], sizeof(GIPTime));
                memcpy(&iptbl[j+1], &iptbl[HostCfg.HostIPCnt], sizeof(GIPTime));
            }
        }
        if (iret == 0) break;
    }

    save2file();
}

//新插入一组数据，和最慢的做比较，并更新
int new_insert(void)
{
    int i, ifnd = HostCfg.HostIPCnt;

    for (i = 0; i < HostCfg.HostIPCnt; i++)
    {
        if (iptbl[i].ipaddr.ipv4 == iptbl[HostCfg.HostIPCnt].ipaddr.ipv4)
        {
            check_all();
            return HostCfg.HostIPCnt;
        }
        if (iptbl[i].timeout > iptbl[HostCfg.HostIPCnt].timeout || iptbl[i].timeout == 0)
        {
            ifnd = i;
            break;
        }
    }
    if (ifnd == HostCfg.HostIPCnt) return ifnd;

    for (i = HostCfg.HostIPCnt - 2; i >= ifnd; i--)
    {
        iptbl[i+1].ipaddr.ipv4 = iptbl[i].ipaddr.ipv4;
        iptbl[i+1].timeout = iptbl[i].timeout;
    }
    iptbl[ifnd].ipaddr.ipv4 = iptbl[HostCfg.HostIPCnt].ipaddr.ipv4;
    iptbl[ifnd].timeout = iptbl[HostCfg.HostIPCnt].timeout;
    
    save2file();

    return i;
}

unsigned int initTest(void)
{
    unsigned int a,b,c,d,l;
    const char * blocks;
    int i;

    IPBlockCnt = 0;
    IPh = Mask = NULL;
    iptbl = (GIPTime*)calloc(HostCfg.HostIPCnt + 1, sizeof(GIPTime));
    blocks = HostCfg.IPBlocks;
    for (i = 0;; i++)
    {
        if (sscanf(blocks, "%d.%d.%d.%d/%d", &a, &b, &c, &d ,&l) != 5)
        {
            break;
        }
        IPh = (IPv4*)realloc(IPh, (i+1)*sizeof(IPv4));
        Mask = (IPv4*)realloc(Mask, (i+1)*sizeof(IPv4));
        IPh[i].ipc[0] = a;
        IPh[i].ipc[1] = b;
        IPh[i].ipc[2] = c;
        IPh[i].ipc[3] = d;
        Mask[i].ipc[0] = 0xFF >> l;
        Mask[i].ipc[1] = 0xFF >> (l >= 8?l-8:0);
        Mask[i].ipc[2] = 0xFF >> (l >= 16?l-16:0);
        Mask[i].ipc[3] = 0xFF >> (l >= 24?l-24:0);
        if (l >= 32) Mask[i].ipv4 = 0U;

        IPBlockCnt++;

        Notify(PRT_INFO, "[testgg:%d] IP[%lx] mask[%lx]", __LINE__, IPh[i].ipv4, Mask[i].ipv4);
        blocks = strchr(blocks, ',');
        if (blocks == NULL) break;
        while(*blocks < '1' || *blocks > '9')
        {
            if (*++blocks == 0) break;
        }
    }

    ssltest.SetIPPort(0UL, 443);

    load2mem();
    check_all();

    return i;
}

int ReadCfg(void)
{
    return 0;
}

#ifdef ONLY_RUN
int Usage(void)
{
    fprintf(stderr, "Usage: testgg [-C timeout] [-S timeout] [-h count] [-f save-file] [-b ip-blocks] [-s sleep_time] [-c check_time]\n");
    fprintf(stderr, "\t -C : timeout(ms) for connect to google IP.\n");
    fprintf(stderr, "\t -S : timeout(ms) for send SSL package.\n");
    fprintf(stderr, "\t -h : save count of host IP.\n");
    fprintf(stderr, "\t -f : save file.\n");
    fprintf(stderr, "\t -b : IP blocks like '74.125.0.0/16,173.194.0.0/16'.\n");
    fprintf(stderr, "\t -s : sleep time(ms) between IP tries.\n");
    fprintf(stderr, "\t -c : time(s) between checks.\n");
    exit(1);
}

#define ArgChar(Section) \
    if (argv[i][2] != 0)\
    {\
        HostCfg.Section = &argv[i][2];\
    }\
    else HostCfg.Section = argv[++i];

#define ArgInt(Section, Amin, Amax) \
    if (argv[i][2] != 0)\
    {\
        HostCfg.Section = atoi(&argv[i][2]);\
    }\
    else HostCfg.Section = atoi(argv[++i]);\
    if (HostCfg.Section < Amin) HostCfg.Connect_Timeout = Amin;\
    if (HostCfg.Section > Amax) HostCfg.Connect_Timeout = Amax;

int ParseArg(int argc, char** argv)
{
    int i;
    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] != '-' || argv[i][1] == 0) Usage();
        switch (argv[i][1])
        {
            case 'C'://
                ArgInt(Connect_Timeout, 100, 10000);
                break;
            case 'S':// 
                ArgInt(SSL_Timeout, 100, 10000);
                break;
            case 'h'://
                ArgInt(HostIPCnt, 3, 100);
                break;
            case 'f':
                ArgChar(BakFile);
                break;
            case 'b':
                ArgChar(IPBlocks);
                break;
            case 's'://
                ArgInt(Time_Sleepms, 100, 600000);
                break;
            case 'c'://
                ArgInt(Time_to_Check, 1, 86400);
                break;
            default:
                Usage();
        }
    }

    return 0;
}

int main(int argc, char** argv)
{
    int i = 0, isel, timet;
    unsigned int rand;
    IPv4 tip;

    HostCfg.Connect_Timeout = 1000;     //-C
    HostCfg.SSL_Timeout = 1000;         //-S
    HostCfg.HostIPCnt = 20;             //-h
    HostCfg.BakFile = "test.txt";       //-f
    HostCfg.IPBlocks =                  //-b
        "74.125.0.0/16,173.194.0.0/16,72.14.192.0/18";
    HostCfg.Time_to_Check = 600;        //-c
    HostCfg.Time_Sleepms = 1000;        //-s

    ParseArg(argc, argv);

    if (HostCfg.Time_to_Check < 2 * HostCfg.HostIPCnt)
    {
        HostCfg.Time_to_Check = 2 * HostCfg.HostIPCnt;
    }

    initTest();

    for (;;)
    {
        i++;
        if (time(NULL) - Check_Time > HostCfg.Time_to_Check) check_all();
        rand = random32();
        isel = rand % IPBlockCnt;
        tip.ipv4 = (rand & Mask[isel].ipv4) | IPh[isel].ipv4;

        timet = tconn(tip.ipv4);
        if (timet > 0 && timet < HostCfg.SSL_Timeout)
        {
            iptbl[HostCfg.HostIPCnt].ipaddr.ipv4 = tip.ipv4;
            iptbl[HostCfg.HostIPCnt].timeout = timet;
            Notify(PRT_NOTICE, "[%d][%d.%d.%d.%d]+[.%d]", i, tip.ipc[0], tip.ipc[1], tip.ipc[2], tip.ipc[3], timet);
            new_insert();
        }
//        else
//        {
//            Notify(PRT_WARNING, "[%d][%d.%d.%d.%d]+[fail]", i, tip.ipc[0], tip.ipc[1], tip.ipc[2], tip.ipc[3]);
//        }
        usleep(HostCfg.Time_Sleepms * 1000);
    }
    return 0;
}
#endif
