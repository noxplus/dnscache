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
    int i = 0;
    ggTest test;

    test.InitTest(argc, argv);

    for (;;)
    {
        Notify(PRT_INFO, "loop:%d", ++i);
        test.LoopFunc();
    }
    return 0;
}
#endif

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

ggHostCFG::ggHostCFG(void)
{
    Connect_Timeout = 1000;     //-C
    SSL_Timeout = 1000;         //-S
    HostIPCnt = 20;             //-h
    BakFile = "test.txt";       //-f
    IPBlocks =                  //-b
        "74.125.0.0/16,173.194.0.0/16,72.14.192.0/18";
    Time_to_Check = 600;        //-c
    Time_Sleepms = 1000;        //-s
}
void ggHostCFG::ReadCfg(const char* cfgfile)
{
    return;
}

#define PraseArgChar(var) \
    if (argv[i][2] != 0)\
    {\
        var = &argv[i][2];\
    }\
    else var = argv[++i];

#define PraseArgInt(var, Amin, Amax) \
    if (argv[i][2] != 0)\
    {\
        var = atoi(&argv[i][2]);\
    }\
    else var = atoi(argv[++i]);\
    if (var < Amin) var = Amin;\
    if (var > Amax) var = Amax;

void ggHostCFG::ParseArg(int argc, char** argv)
{
    int i;
    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] != '-' || argv[i][1] == 0) Usage();
        switch (argv[i][1])
        {
            case 'C': PraseArgInt(Connect_Timeout, 100, 10000); break;
            case 'S': PraseArgInt(SSL_Timeout, 100, 10000); break;
            case 'h': PraseArgInt(HostIPCnt, 3, 100); break;
            case 'f': PraseArgChar(BakFile); break;
            case 'b': PraseArgChar(IPBlocks); break;
            case 's': PraseArgInt(Time_Sleepms, 100, 600000); break;
            case 'c': PraseArgInt(Time_to_Check, 1, 86400); break;
            default:
                Usage();
        }
    }
}

ggTest::ggTest(void)
{
    m_IPBlockCnt = 0;
    m_ipHead = NULL;
    m_ipMask = NULL;
    m_checkTime = 0;
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

void ggTest::InitTest(int argc, char** argv)
{
    unsigned int a,b,c,d,l;
    int i;
    const char* blocks = m_cfg.IPBlocks;

    m_cfg.ParseArg(argc, argv);

    SetTimeout(m_cfg.Connect_Timeout, m_cfg.SSL_Timeout, m_cfg.SSL_Timeout);

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
    CheckAll();
    SetIPPort(0UL, 443);
}
void ggTest::Load2Mem(void)
{
    FILE* fr = fopen(m_cfg.BakFile, "r");
    if (fr == NULL) return;
    char line[1024];

    int i, iret;
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
        ggRec recread(ipread.ipv4, m_cfg.SSL_Timeout);
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
void ggTest::CheckAll(void)
{
    int iret;

    time(&m_checkTime);

    ggListIter it;
    for (it = m_list.begin(); it != m_list.end(); it++)
    {
        iret = RunTest(it->GetIPAddr());
        if (iret > 0 && iret < m_cfg.SSL_Timeout)
            it->SetTimeout(iret);
        else it->SetTimeout(m_cfg.SSL_Timeout);
    }

    m_list.sort();

    Save2File();
}
void ggTest::LoopFunc(void)
{
    uint32 testip, tout;
    int isel;

    if (time(NULL) - m_checkTime > m_cfg.Time_to_Check) CheckAll();

    testip = random32();
    isel = testip % m_IPBlockCnt;
    testip = (testip & m_ipMask[isel].ipv4) | m_ipHead[isel].ipv4;

    tout = RunTest(testip);

    Notify(PRT_NOTICE, "new ip [%ul]", tout);

    ggRec test(testip, tout);

    test.print();

    ggListIter it;
    for(it = m_list.begin(); it != m_list.end(); ++it)
    {
        if (test < *it)
        {
            m_list.insert(it, test);
            Notify(PRT_NOTICE, "insert");
            break;
        }
    }

    if (m_list.size() > m_cfg.HostIPCnt)
    {
        m_list.pop_back();
    }
}
