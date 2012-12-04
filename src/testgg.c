/* 功能描述
 * 一、在指定IP段随机生成IPv4地址.
 * 二、多次connect该IP地址443端口，计算平均消耗时间。
 * 三、发送ssl handshake，测试该IP地址是否支持https访问。
 * */

#include "util.h"
#include "testgg.h"

#define conn_TIMEOUT    1000    //连接超时 单位:ms
#define conn_TIMES      4       //连接次数
#define ssl_TIMEOUT     1000    //ssl超时 单位:ms

static IPv4*    IPh = NULL;
static IPv4*    Mask = NULL;

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

    timeout.tv_sec = ssl_TIMEOUT/1000;
    timeout.tv_usec = ssl_TIMEOUT%1000 * 1000;
    FD_ZERO(&fdset);
    FD_SET(sk, &fdset);
    select(sk+1, NULL, &fdset, NULL, &timeout);	
    if (FD_ISSET(sk, &fdset) == 0)
    {
        return ssl_TIMEOUT;
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

    timeout.tv_sec = ssl_TIMEOUT/1000;
    timeout.tv_usec = ssl_TIMEOUT%1000 * 1000;
    FD_ZERO(&fdset);
    FD_SET(sk, &fdset);
    select(sk+1, &fdset, NULL, NULL, &timeout);	
    if (FD_ISSET(sk, &fdset))
    {
        iret = recv(sk, &sslh, sizeof(sslh), 0);
        if (iret != sizeof(sslh)) return ssl_TIMEOUT * 4;//connect reset!
        iret = recv(sk, &HSh, sizeof(HSh), 0);
        if (iret != sizeof(HSh)) return ssl_TIMEOUT * 4;
        if (sslh.ContentType != 0x16 || HSh.HandshakeType != 0x2) return ssl_TIMEOUT * 4;
        return ssl_TIMEOUT - (timeout.tv_sec*1000 + (timeout.tv_usec+500)/1000);
    }

    return ssl_TIMEOUT;
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

    int i, j, issl = 0;
    for (i = 0; i < conn_TIMES; i++)
    {
        if ((skfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
        {
            Notify(PRT_ERROR, "[testgg:%d]socket error", __LINE__);
            return -1;
        }
        SetSocketBlock(skfd, false);
        for (j = conn_TIMEOUT; j > 0; j -= 50)
        {
            iret = connect(skfd, (struct sockaddr*)&gsrv, sizeof(struct sockaddr));
            if (iret == 0 || errno == EISCONN) break;//连接正常
            if (iret == -1 && errno != EINPROGRESS && errno != EALREADY)
            {
                SetSocketBlock(skfd, true);
                Notify(PRT_ERROR, "[testgg:%d] [%d]connect [%d.%d.%d.%d] error[%d:%s]\n", __LINE__, j, ip.ipc[0], ip.ipc[1], ip.ipc[2], ip.ipc[3], errno, strerror(errno));
                return -1;
            }
            usleep(50 * 1000);
        }
        SetSocketBlock(skfd, true);

        if (i == conn_TIMES - 1)//最后一次进行ssl handshake
        {
            issl = tssl(skfd);
            close(skfd);
            return issl;
        }
        close(skfd);
    }

    return conn_TIMEOUT;
}

unsigned int initTest(int Cnt, char** list)
{
    unsigned int a,b,c,d,l;
    int i;
    IPh = (IPv4*)malloc(Cnt * sizeof(IPv4));
    Mask = (IPv4*)malloc(Cnt * sizeof(IPv4));
    for (i = 0; i < Cnt; i++)
    {
        if (sscanf(list[i], "%d.%d.%d.%d/%d", &a, &b, &c, &d ,&l) != 5)
        {
            break;
        }
        IPh[i].ipc[0] = a;
        IPh[i].ipc[1] = b;
        IPh[i].ipc[2] = c;
        IPh[i].ipc[3] = d;
        Mask[i].ipc[0] = 0xFF >> l;
        Mask[i].ipc[1] = 0xFF >> (l >= 8?l-8:0);
        Mask[i].ipc[2] = 0xFF >> (l >= 16?l-16:0);
        Mask[i].ipc[3] = 0xFF >> (l >= 24?l-24:0);
        if (l >= 32) Mask[i].ipv4 = 0U;

        Notify(PRT_INFO, "[testgg:%d] IP[%lx] mask[%lx]", __LINE__, IPh[i].ipv4, Mask[i].ipv4);
    }

    return i;
}

#ifdef ONLY_RUN
int main(int argc, char** argv)
{
    char *address[3] = {"74.125.0.0/16", "173.194.0.0/16", "72.14.192.0/18"};
    int i = 0, isel, timet;
    unsigned int rand;
    IPv4 tip;
    int AddrCnt;

    if (argc != 1)
    {
        initTest(argc - 1, argv+1);
        AddrCnt = argc - 1;
    }
    else
    {
        initTest(3, address);
        AddrCnt = 3;
    }

    for (i = 0; i < 100; i++)
    {
        rand = random32();
        isel = rand % AddrCnt;
        tip.ipv4 = (rand & Mask[isel].ipv4) | IPh[isel].ipv4;

        timet = tconn(tip.ipv4);
        if (timet > 0 && timet < ssl_TIMEOUT)
        {
            Notify(PRT_NOTICE, "[%d][%d.%d.%d.%d]+[.%d]", i, tip.ipc[0], tip.ipc[1], tip.ipc[2], tip.ipc[3], timet);
        }
        else
        {
            Notify(PRT_WARNING, "[%d][%d.%d.%d.%d]+[fail]", i, tip.ipc[0], tip.ipc[1], tip.ipc[2], tip.ipc[3]);
        }
    }
    return 0;
}
#endif
