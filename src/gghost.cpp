#include "util.hpp"
#include "gghost.hpp"

//线程分类：
//主线程 - 所有数据IO，定期检测
//0~n 分块线程，测试各个分块
//历史线程

static ggHostCFG g_cfg;
static IPBlock  g_ips;

#ifdef ONLY_RUN
int Usage(void)
{
    fprintf(stderr, "Usage: testgg [-C timeout] [-S timeout] [-h count] [-f save-file] [-b ip-blocks] [-t test_time] [-c check_time]\n");
    fprintf(stderr, "\t -C : [%ld ms] timeout(ms) for connect to google IP.\n", g_cfg.ConnTimeout);
    fprintf(stderr, "\t -S : [%ld ms] timeout(ms) for send SSL package.\n", g_cfg.SSLTimeout);
    fprintf(stderr, "\t -h : [%ld] save count of host IP.\n", g_cfg.HostIPCnt);
    fprintf(stderr, "\t -f : [%s] save file.\n", g_cfg.BakFile);
    fprintf(stderr, "\t -b : IP blocks like '%s'.\n", g_cfg.IPBlocks);
    fprintf(stderr, "\t -t : [%ld ms] time(ms) between IP tries.\n", g_cfg.TestInter);
    fprintf(stderr, "\t -c : [%ld s] time(ms) between checks.\n", g_cfg.ChkInter);
    exit(1);
}

int main(int argc, char** argv)
{
    printf("build@ %s by %s %s\n", TIMESTAMP, AUTHER, TIMEVER);

    ggTest test;

    test.ParseArg(argc, argv);

    test.InitTest();

    for (;;)
    {
        test.LoopFunc();
    }
    return 0;
}
#else
int Usage(void)
{
    exit(0);
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
#ifdef _WIN32
    if (timeout < ERR_no)
        _snprintf(str, len, "%3d.%3d.%3d.%3d \t%lu", ipaddr.ipc[0],
                ipaddr.ipc[1], ipaddr.ipc[2], ipaddr.ipc[3], timeout);
    else
        _snprintf(str, len, "%3d.%3d.%3d.%3d \t%s", ipaddr.ipc[0],
                ipaddr.ipc[1], ipaddr.ipc[2], ipaddr.ipc[3], err2str(timeout));
#else
    if (timeout < ERR_no)
        snprintf(str, len, "%3d.%3d.%3d.%3d \t%lu", ipaddr.ipc[0],
                ipaddr.ipc[1], ipaddr.ipc[2], ipaddr.ipc[3], timeout);
    else
        snprintf(str, len, "%3d.%3d.%3d.%3d \t%s", ipaddr.ipc[0],
                ipaddr.ipc[1], ipaddr.ipc[2], ipaddr.ipc[3], err2str(timeout));
#endif
}

void ggTest::InitCfg(void)
{
    g_cfg.ConnTimeout = 230;        //-C
    g_cfg.SSLTimeout = 230;         //-S
    g_cfg.HostIPCnt = 30;           //-h
    g_cfg.BakFile = "test.txt";     //-f
    g_cfg.IPBlocks =                //-b
        "64.233.160.0/19,66.249.80.0/20,72.14.192.0/18,209.85.128.0/17,"
        "66.102.0.0/20,74.125.0.0/16,64.18.0.0/20,207.126.144.0/20,173.194.0.0/16";
    g_cfg.ChkInter = 600*1000;      //-c
    g_cfg.TestInter = 250;          //-s
}

#define PraseChar(var) \
    if (argv[i][2] != 0)\
    {\
        g_cfg.var = &argv[i][2];\
    }\
    else if (i < argc - 1) g_cfg.var = argv[++i];\
    else Usage();

#define PraseInt(var, Amin, Amax) \
    if (argv[i][2] != 0)\
    {\
        g_cfg.var = atoi(&argv[i][2]);\
    }\
    else if (i < argc - 1) g_cfg.var = atoi(argv[++i]);\
    else Usage();\
    if (m_cfg.var < Amin) g_cfg.var = Amin;\
    if (m_cfg.var > Amax) g_cfg.var = Amax;

void ggTest::ParseArg(int argc, char** argv)
{
    int i;

    InitCfg();

    for (i = 1; i < argc; i++)
    {
#ifdef ONLY_RUN
        if (argv[i][0] != '-' || argv[i][1] == 0) Usage(m_cfg);
#endif
        switch (argv[i][1])
        {
            case 'C': PraseInt(ConnTimeout, 100, 10000); break;
            case 'S': PraseInt(SSLTimeout, 100, 10000); break;
            case 'h': PraseInt(HostIPCnt, 5, 100); break;
            case 'f': PraseChar(BakFile); break;
            case 'b': PraseChar(IPBlocks); break;
            case 't': PraseInt(TestInter, 100, 86400000); break;
            case 'c': PraseInt(ChkInter, 1000, 86400000); break;
            default:
                      Usage();
        }
    }
}

ggTest::ggTest(void)
{
    m_next_test = 0;
    m_next_check = 0;
}
ggTest::~ggTest(void)
{
}

void ggTest::InitTest(void)
{

    g_ips.Init(m_cfg.IPBlocks);

    for (int i = 0; i < m_ipnet.GetCnt(); i++)
    {
        CreateThread(OneBlockFunc, i);
    }

    Load2Mem();
    CheckFunc();
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
        ggRec recread(ipread.ipv4, ERR_no);
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
    int i = 0;

    m_next_check = GetTimeMs() + m_cfg.ChkInter;

    ggListIter it;
    for (it = m_list.begin(); it != m_list.end(); it++)
    {
        iret = RunTest(it->GetIPAddr());
        it->SetTimeout(iret);

        char tstr[256];
        it->tostr(tstr, sizeof(tstr));
        Notify(PRT_DEBUG, "[%03d] Check ip %s", i++, tstr);
    }

    m_list.sort();

    Save2File();
}
void ggTest::TestFunc(void)
{
    uint32 tip, tout;

    m_next_test = GetTimeMs() + m_cfg.TestInter;

    tip = m_ipnet.GetRandIP();
    tout = RunTest(tip);

    ggRec test(tip, tout);

    char tstr[256];
    test.tostr(tstr, sizeof(tstr));
    Notify(PRT_DEBUG, "test ip %s", tstr);

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
            if (m_list.size() > m_cfg.HostIPCnt)
            {
                m_list.pop_back();
            }
            Save2File();
            break;
        }
    }

    if (it == m_list.end() && m_list.size() < m_cfg.HostIPCnt)
    {
        m_list.push_back(test);
        Notify(PRT_NOTICE, "insert tail");
        Save2File();
    }
}
void ggTest::LoopFunc(void)
{
    if (m_next_check <= GetTimeMs())
    {
        return CheckFunc();
    }
    if (m_next_test <= GetTimeMs())
    {
        return TestFunc();
    }

    if (m_next_check <= m_next_test)
    {
        SleepMS(m_next_check - GetTimeMs());
        return CheckFunc();
    }
    SleepMS(m_next_test - GetTimeMs());
    return TestFunc();
}

ggStore::ggStore(void)
{
    g_SizeStore = DefaultSizeStore;
    g_SizeHistory = DefaultSizeHistory;
    g_Store = new IPVal[g_SizeStore];
    g_History = new IPv4[g_SizeHistory];
}
ggStore::ggStore(int ist, int ihi)
{
    g_SizeStore = ist;
    g_SizeHistory = ihi;
    g_Store = new IPVal[g_SizeStore];
    g_History = new IPv4[g_SizeHistory];
}
ggStore::~ggStore(void)
{
    if (g_Store != NULL)
    {
        delete [] g_Store;
        g_Store = NULL;
    }
    if (g_History != NULL)
    {
        delete [] g_History;
        g_History = NULL;
    }
    g_SizeStore = 0;
    g_SizeHistory = 0;
}


int OneBlockFunc(void *arg)
{
    int idx = int(arg);
    uint32 tip, tout;
    SSLTest test;
    test.SetIPPort(0UL, 443);
    test.SetTimeout(g_cfg.ConnTimeout, g_cfg.SSLTimeout, g_cfg.SSLTimeout);

    for(;;)
    {
        tip = g_ipnet.GetIdxRandIP(idx);
        tout = test.RunTest(tip);
    }
}
