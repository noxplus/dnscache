#include "util.hpp"
#include "gghost.hpp"


#ifdef ONLY_RUN
int Usage(void)
{
    fprintf(stderr, "Usage: testgg [-C timeout] [-S timeout] [-h count] [-f save-file] [-b ip-blocks] [-t test_time] [-c check_time]\n");
    fprintf(stderr, "\t -C : timeout(ms) for connect to google IP.\n");
    fprintf(stderr, "\t -S : timeout(ms) for send SSL package.\n");
    fprintf(stderr, "\t -h : save count of host IP.\n");
    fprintf(stderr, "\t -f : save file.\n");
    fprintf(stderr, "\t -b : IP blocks like '74.125.0.0/16,173.194.0.0/16'.\n");
    fprintf(stderr, "\t -t : time(ms) between IP tries.\n");
    fprintf(stderr, "\t -c : time(ms) between checks.\n");
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
    snprintf(str, len, "%3d.%3d.%3d.%3d\t%4lu", ipaddr.ipc[0],
            ipaddr.ipc[1], ipaddr.ipc[2], ipaddr.ipc[3], timeout);
}

void ggTest::InitCfg(void)
{
    m_cfg.ConnTimeout = 500;       //-C
    m_cfg.SSLTimeout = 500;        //-S
    m_cfg.HostIPCnt = 30;           //-h
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
    else if (i < argc - 1) m_cfg.var = argv[++i];\
    else Usage();

#define PraseInt(var, Amin, Amax) \
    if (argv[i][2] != 0)\
    {\
        m_cfg.var = atoi(&argv[i][2]);\
    }\
    else if (i < argc - 1) m_cfg.var = atoi(argv[++i]);\
    else Usage();\
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
    SetTimeout(m_cfg.ConnTimeout, m_cfg.SSLTimeout, m_cfg.SSLTimeout);

    m_ipnet.Init(m_cfg.IPBlocks);

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

        char tstr[256];
        it->tostr(tstr, sizeof(tstr));
        Notify(PRT_DEBUG, "Check ip %s", tstr);
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
        usleep(1000 * (m_next_check - GetTimeMs()));
        return CheckFunc();
    }
    usleep(1000 * (m_next_test - GetTimeMs()));
    return TestFunc();
}
