#include "util.hpp"
#include "gghost.hpp"


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

int main(int argc, char** argv)
{
    ggTest test;

    test.ParseArg(argc, argv);

    test.InitTest();

    for (;;)
    {
        test.LoopFunc();
    }
    return 0;
}
#endif

//重载比较运算符
//时间相等就比地址。
bool ggRec::operator>(const ggRec& rhs)
{
    if (timeout > rhs.timeout) return true;
    if (timeout < rhs.timeout) return false;
    if (ipaddr.ipv4 > rhs.ipaddr.ipv4) return true;
    return false;
}
bool ggRec::operator<(const ggRec& rhs)
{
    if (timeout < rhs.timeout) return true;
    if (timeout > rhs.timeout) return false;
    if (ipaddr.ipv4 < rhs.ipaddr.ipv4) return true;
    return false;
}
bool ggRec::operator==(const ggRec& rhs)
{
    if (timeout == rhs.timeout && ipaddr.ipv4 == rhs.ipaddr.ipv4)
        return true;
    return false;
}
void ggRec::print(FILE* fw)
{
    fprintf(fw, "%d.%d.%d.%d\t%lu\n", ipaddr.ipc[0],
            ipaddr.ipc[1], ipaddr.ipc[2], ipaddr.ipc[3], timeout);
}
void ggRec::SetIPAddr(uint32 ip)
{
    ipaddr.ipv4 = ip;
}
void ggRec::SetTimeout(uint32 time)
{
    timeout = time;
}
uint32 ggRec::GetIPAddr(void)
{
    return ipaddr.ipv4;
}
uint32 ggRec::GetTimeout(void)
{
    return timeout;
}
void ggRec::tostr(char* str, int len)
{
    snprintf(str, len, "%3d.%3d.%3d.%3d\t%4lu", ipaddr.ipc[0],
            ipaddr.ipc[1], ipaddr.ipc[2], ipaddr.ipc[3], timeout);
}

void ggTest::InitCfg(void)
{
    m_cfg.ConnTimeout = 1000;       //-C
    m_cfg.SSLTimeout = 1000;        //-S
    m_cfg.HostIPCnt = 20;           //-h
    m_cfg.BakFile = "test.txt";     //-f
    m_cfg.IPBlocks =                //-b
        "74.125.0.0/16,173.194.0.0/16,72.14.192.0/18";
    m_cfg.ChkInter = 600*1000;      //-c
    m_cfg.TestInter = 1000;         //-s
}

#define PraseChar(var) \
    if (argv[i][2] != 0)\
    {\
        m_cfg.var = &argv[i][2];\
    }\
    else m_cfg.var = argv[++i];

#define PraseInt(var, Amin, Amax) \
    if (argv[i][2] != 0)\
    {\
        m_cfg.var = atoi(&argv[i][2]);\
    }\
    else m_cfg.var = atoi(argv[++i]);\
    if (m_cfg.var < Amin) m_cfg.var = Amin;\
    if (m_cfg.var > Amax) m_cfg.var = Amax;

void ggTest::ParseArg(int argc, char** argv)
{
    int i;

    InitCfg();

    for (i = 1; i < argc; i++)
    {
#ifdef ONLY_RUN
        if (argv[i][0] != '-' || argv[i][1] == 0) Usage();
#endif
        switch (argv[i][1])
        {
            case 'C': PraseInt(ConnTimeout, 100, 10000); break;
            case 'S': PraseInt(SSLTimeout, 100, 10000); break;
            case 'h': PraseInt(HostIPCnt, 3, 100); break;
            case 'f': PraseChar(BakFile); break;
            case 'b': PraseChar(IPBlocks); break;
            case 't': PraseInt(ChkInter, 1000, 86400000); break;
            case 'c': PraseInt(TestInter, 1, 86400000); break;
            default:
#ifdef ONLY_RUN
                Usage();
#else
                exit(0);
#endif
        }
    }
}

ggTest::ggTest(void)
{
    m_IPBlockCnt = 0;
    m_ipHead = NULL;
    m_ipMask = NULL;
    m_next_test = 0;
    m_next_check = 0;
}
ggTest::~ggTest(void)
{
    if (m_ipHead != NULL)
    {
        free(m_ipHead);
        m_ipHead = NULL;
    }
    if (m_ipMask != NULL)
    {
        free(m_ipMask);
        m_ipMask = NULL;
    }
}

void ggTest::InitTest(void)
{
    unsigned int a,b,c,d,l;
    int i;
    const char* blocks = m_cfg.IPBlocks;

    SetTimeout(m_cfg.ConnTimeout, m_cfg.SSLTimeout, m_cfg.SSLTimeout);

    for (i = 0;; i++)
    {
        if (sscanf(blocks, "%d.%d.%d.%d/%d", &a, &b, &c, &d ,&l) != 5)
        {
            break;
        }
        m_ipHead = (IPv4*)realloc(m_ipHead, (i+1)*sizeof(IPv4));
        m_ipMask = (IPv4*)realloc(m_ipMask, (i+1)*sizeof(IPv4));

        m_ipHead[i].ipc[0] = a;
        m_ipHead[i].ipc[1] = b;
        m_ipHead[i].ipc[2] = c;
        m_ipHead[i].ipc[3] = d;

        m_ipMask[i].ipc[0] = 0xFF >> l;
        m_ipMask[i].ipc[1] = 0xFF >> (l >= 8?l-8:0);
        m_ipMask[i].ipc[2] = 0xFF >> (l >= 16?l-16:0);
        m_ipMask[i].ipc[3] = 0xFF >> (l >= 24?l-24:0);
        if (l >= 32) m_ipMask[i].ipv4 = 0U;

        m_IPBlockCnt++;

        Notify(PRT_INFO, "[testgg:%d] IP[%lx] mask[%lx]", __LINE__,
                m_ipHead[i].ipv4, m_ipMask[i].ipv4);
        blocks = strchr(blocks, ',');
        if (blocks == NULL) break;
        while(*blocks < '1' || *blocks > '9')
        {
            if (*++blocks == 0) break;
        }
    }

    Load2Mem();
    CheckFunc();
    SetIPPort(0UL, 443);
}
void ggTest::Load2Mem(void)
{
    FILE* fr = fopen(m_cfg.BakFile, "r");
    if (fr == NULL) return;
    char line[1024];

    uint32 i;
    int iret;
    IPv4 ipread;
    for (i = 0; i < m_cfg.HostIPCnt; i++)
    {
        int a,b,c,d;
        if (fgets(line, sizeof(line), fr) == NULL) break;
        iret = sscanf(line, "%d.%d.%d.%d", &a, &b, &c, &d);
        if (iret != 4) break;
        ipread.ipc[0] = a;
        ipread.ipc[1] = b;
        ipread.ipc[2] = c;
        ipread.ipc[3] = d;
        ggRec recread(ipread.ipv4, m_cfg.SSLTimeout);
        m_list.push_back(recread);
    }
    fclose(fr);
    return;
}
void ggTest::Save2File(void)
{
    FILE* fw = fopen(m_cfg.BakFile, "w");
    if (fw == NULL) return;
    
    ggListIter it;
    for (it = m_list.begin(); it != m_list.end(); it++)
    {
        it->print(fw);
    }

    fclose(fw);
    return;
}
void ggTest::CheckFunc(void)
{
    int iret;
    
    m_next_check = GetTimeMs() + m_cfg.ChkInter;

    ggListIter it;
    for (it = m_list.begin(); it != m_list.end(); it++)
    {
        iret = RunTest(it->GetIPAddr());
        if (iret > ERR_no) it->SetTimeout(m_cfg.SSLTimeout);
        else it->SetTimeout(iret);
    }

    m_list.sort();

    Save2File();
}
void ggTest::TestFunc(void)
{
    uint32 tip, tout;
    int isel;

    m_next_test = GetTimeMs() + m_cfg.TestInter;
    Notify(PRT_INFO, "next test %lu", m_next_test);

    tip = random32();
    isel = tip % m_IPBlockCnt;
    tip = (tip & m_ipMask[isel].ipv4) | m_ipHead[isel].ipv4;

    tout = RunTest(tip);

    ggRec test(tip, tout);

    char tstr[256];
    test.tostr(tstr, sizeof(tstr));
    Notify(PRT_INFO, "new ip %s", tstr);

    if (tout > ERR_no) return;

    ggListIter it;
    uint32 id = 0;
    for(it = m_list.begin(); it != m_list.end(); ++it, ++id)
    {
        if (test < *it)
        {
            if (id < m_cfg.HostIPCnt/3)
            {//强制措施：前1/3数据被替换时，重新检测
                CheckFunc();
                it = m_list.begin();
                id = m_cfg.HostIPCnt/3 + 1;
                continue;
            }
            m_list.insert(it, test);
            Notify(PRT_NOTICE, "insert");
            Save2File();
            break;
        }
    }

    if (m_list.size() > m_cfg.HostIPCnt)
    {
        m_list.pop_back();
    }
}
void ggTest::LoopFunc(void)
{
    uint32 nowtime;
    
    nowtime = GetTimeMs();
    Notify(PRT_INFO, "now %lu", nowtime);

    if (m_next_check <= nowtime)
    {
        return CheckFunc();
    }
    if (m_next_test <= nowtime)
    {
        return TestFunc();
    }

    if (m_next_check <= m_next_test)
    {
        usleep(1000 * (m_next_check - nowtime));
        return CheckFunc();
    }
    Notify(PRT_INFO, "sleep %lu", m_next_test - nowtime);
    usleep(1000 * (m_next_test - nowtime));
    return TestFunc();
}
