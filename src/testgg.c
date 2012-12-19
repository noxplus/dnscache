/* 功能描述
 * 一、在指定IP段随机生成IPv4地址.
 * 二、connect该IP地址443端口，计算平均消耗时间。
 * 三、发送ssl handshake，测试该IP地址是否支持https访问。
 * */

#include "util.h"
#include "testgg.h"

typedef struct _cfg_gghost_globa
{
    int32       Connect_Timeout;//默认1000ms
    int32       SSL_Timeout;//默认1000ms
    int32       HostIPCnt; //默认：0～cnt-1存地址，cnt存新入的。共cnt+1
    char*       BakFile;
    char*       IPBlocks;
}CFG_GGHost;

static IPv4*    IPh = NULL;
static IPv4*    Mask = NULL;
static GIPTime  *iptbl;//存储速度最快的IP地址
static int      IPBlockCnt = 0;

CFG_GGHost      HostCfg;

//设置socket阻塞/非阻塞
bool SetSocketBlock(int fd, bool block)//true/false
{
   if (fd < 0) return false;

#ifdef WIN32
   unsigned long mode = block == true ? 0 : 1;
   return (ioctlsocket(fd, FIONBIO, &mode) == 0) ? true : false;
#else
  int flags;
  if ((flags = fcntl(fd, F_GETFL, 0)) < 0) return false;
  flags = block == true ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
  return (fcntl(fd, F_SETFL, flags) == 0) ? true : false;
#endif
}

//给指定socket发送ssl 
int tssl(int sk)
{
    int iret;
    fd_set fdset;
    struct timeval timeout;
    SSLCliHello cli;

    timeout.tv_sec = HostCfg.SSL_Timeout / 1000;
    timeout.tv_usec = HostCfg.SSL_Timeout % 1000 * 1000;
    FD_ZERO(&fdset);
    FD_SET(sk, &fdset);
    select(sk+1, NULL, &fdset, NULL, &timeout);	
    if (FD_ISSET(sk, &fdset) == 0)
    {
        return HostCfg.SSL_Timeout;
    }

    cli.ssl.ContentType = 22;
    cli.ssl.SSLVer = htons(0x0301);
    cli.ssl.ContentLen = htons(sizeof(cli) - sizeof(cli.ssl));
    cli.hands.HandshakeType = 0x01;
    cli.hands.LenHi = 0x0;
    cli.hands.LenLo = htons(sizeof(cli) - sizeof(cli.ssl) - sizeof(cli.hands));
    cli.SSLVer = htons(0x0301);
    cli.sidLen = 0x0;
    cli.CipherSuitesLen = htons(0x02);
    cli.CipherSuites = htons(0xc011);
    cli.CompMethLen = 0x1;
    cli.CompMeth = 0x0;

    iret = send(sk, &cli, sizeof(cli), 0);//测试成功拿到ssl证书！

    SSLHead sslh;
    HSHead  HSh;

    timeout.tv_sec = HostCfg.SSL_Timeout/1000;
    timeout.tv_usec = HostCfg.SSL_Timeout%1000 * 1000;
    FD_ZERO(&fdset);
    FD_SET(sk, &fdset);
    select(sk+1, &fdset, NULL, NULL, &timeout);	
    if (FD_ISSET(sk, &fdset))
    {
        iret = recv(sk, &sslh, sizeof(sslh), 0);
        if (iret != sizeof(sslh)) return HostCfg.SSL_Timeout * 4;//connect reset!
        iret = recv(sk, &HSh, sizeof(HSh), 0);
        if (iret != sizeof(HSh)) return HostCfg.SSL_Timeout * 4;
        if (sslh.ContentType != 0x16 || HSh.HandshakeType != 0x2) return HostCfg.SSL_Timeout * 4;
        return HostCfg.SSL_Timeout - (timeout.tv_sec*1000 + (timeout.tv_usec+500)/1000);
    }

    return HostCfg.SSL_Timeout;
}

int tconn(unsigned int tip)
{
    int skfd = -1;
    int iret;
    struct sockaddr_in gsrv;
    IPv4 ip;

    ip.ipv4 = tip;
    memset(&gsrv, 0, sizeof(gsrv));

    gsrv.sin_family = AF_INET;
    gsrv.sin_port = htons(443);
    gsrv.sin_addr.s_addr = tip;

    int j, issl = 0;
    if ((skfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    {
        Notify(PRT_ERROR, "[testgg:%d]socket error", __LINE__);
        return -1;
    }
    SetSocketBlock(skfd, false);
    for (j = 0; j <= HostCfg.Connect_Timeout; j += 50)
    {
        iret = connect(skfd, (struct sockaddr*)&gsrv, sizeof(struct sockaddr));
        if (iret == 0 || errno == EISCONN) break;//连接正常
        if (iret == -1 && errno != EINPROGRESS && errno != EALREADY)
        {
            SetSocketBlock(skfd, true);
            Notify(PRT_ERROR, "[testgg:%d][%d]connect[%d.%d.%d.%d]error[%d:%s]", __LINE__, j, ip.ipc[0],ip.ipc[1],ip.ipc[2],ip.ipc[3], errno, strerror(errno));
            return -1;
        }
        usleep(50 * 1000);
    }
    SetSocketBlock(skfd, true);

    issl = tssl(skfd);
    close(skfd);
    return issl;
}

int save2file(void)
{
    FILE* fw = fopen(HostCfg.BakFile, "w");
    if (fw == NULL) return 0;
    
    int i;
    for (i = 0; i < HostCfg.HostIPCnt; i++)
    {
        fprintf(fw, "%d.%d.%d.%d    %lu\n",
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

    int i;
    for (i = 0; i < HostCfg.HostIPCnt; i++)
    {
        int a,b,c,d,e;
        int iret = fscanf(fr, "%d.%d.%d.%d    %d\n", &a, &b, &c, &d, &e);
        if (iret != 5) break;
        iptbl[i].ipaddr.ipc[0] = a;
        iptbl[i].ipaddr.ipc[1] = b;
        iptbl[i].ipaddr.ipc[2] = c;
        iptbl[i].ipaddr.ipc[3] = d;
        iptbl[i].timeout = e;
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
    char * blocks;
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

    load2mem();
    check_all();

    return i;
}

#ifdef ONLY_RUN
int Usage(void)
{
    fprintf(stderr, "Usage: testgg [-C timeout] [-S timeout] [-h count] [-f save-file] [-b ip-blocks]\n");
    fprintf(stderr, "\t -C : timeout(ms) for connect to google IP.\n");
    fprintf(stderr, "\t -S : timeout(ms) for send SSL package.\n");
    fprintf(stderr, "\t -h : save count of host IP.\n");
    fprintf(stderr, "\t -f : save file.\n");
    fprintf(stderr, "\t -b : IP blocks like '74.125.0.0/16,173.194.0.0/16'.\n");
    exit(1);
}

int main(int argc, char** argv)
{
    int i = 0, isel, timet;
    unsigned int rand;
    IPv4 tip;
    char opt;

    HostCfg.Connect_Timeout = 1000;     //-C
    HostCfg.SSL_Timeout = 1000;         //-S
    HostCfg.HostIPCnt = 20;             //-h
    HostCfg.BakFile = "test.txt";       //-f
    HostCfg.IPBlocks =                  //-b
        "74.125.0.0/16,173.194.0.0/16,72.14.192.0/18";

    while ((opt = getopt(argc, argv, "C:S:h:f:b:h?")) != -1)
    {
        switch(opt)
        {
            case 'C':// connect timeout
                HostCfg.Connect_Timeout = atoi(optarg);
                if (HostCfg.Connect_Timeout < 100) HostCfg.Connect_Timeout = 100;
                if (HostCfg.Connect_Timeout > 10000) HostCfg.Connect_Timeout = 10000;
                break;
            case 'S':// connect timeout
                HostCfg.SSL_Timeout = atoi(optarg);
                if (HostCfg.SSL_Timeout < 100) HostCfg.SSL_Timeout = 100;
                if (HostCfg.SSL_Timeout > 10000) HostCfg.SSL_Timeout = 10000;
                break;
            case 'h':// connect timeout
                HostCfg.HostIPCnt = atoi(optarg);
                if (HostCfg.HostIPCnt < 3) HostCfg.HostIPCnt = 3;
                if (HostCfg.HostIPCnt > 100) HostCfg.HostIPCnt = 100;
                break;
            case 'f':
                HostCfg.BakFile = optarg;
                break;
            case 'b':
                HostCfg.IPBlocks = optarg;
                break;
            default:
                Usage();
        }
    }
    if (optind < argc) Usage(); //有未做处理的参数

    initTest();

    for (;;)
    {
        i++;
        if (1 % 100 == 0) check_all();
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
    }
    return 0;
}
#endif
