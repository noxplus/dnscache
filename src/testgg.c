/* 功能描述
 * 一、在指定IP段随机生成IPv4地址.
 * 二、多次connect该IP地址443端口，计算平均消耗时间。
 * 三、发送ssl handshake，测试该IP地址是否支持https访问。
 * */

#include "inc.h"

#define conn_TIMEOUT    1000    //连接超时 单位:ms
#define conn_TIMES      4       //连接次数
#define ssl_TIMEOUT     1000    //ssl超时 单位:ms

static int      GAddrCnt = 0;
static IPv4*    IPh = NULL;
static IPv4*    Mask = NULL;

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

int tssl(int sk)
{
    int iret;
    SSLCliHello cli;

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

    send(sk, &cli, sizeof(cli), 0);//测试成功拿到ssl证书！

    fd_set Read;
    struct timeval timeout;
    SSLHead sslh;
    HSHead  HSh;
    timeout.tv_sec = ssl_TIMEOUT/1000;
    timeout.tv_usec = ssl_TIMEOUT%1000 * 1000;
    FD_ZERO(&Read);
    FD_SET(sk, &Read);
    select(sk+1, &Read, NULL, NULL, &timeout);	
    if (FD_ISSET(sk, &Read))
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
    struct sockaddr_in gsrv;
    IPv4 ip;

    ip.ipv4 = tip;
    memset(&gsrv, 0, sizeof(gsrv));

    gsrv.sin_family = AF_INET;
    gsrv.sin_port = htons(443);
    gsrv.sin_addr.s_addr = tip;

    int i, issl = 0;
    for (i = 0; i < conn_TIMES; i++)
    {
        if ((skfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
        {
            printf("socket error\n");
            return -1;
        }
        SetSocketBlock(skfd, false);

        if (connect(skfd, (struct sockaddr*)&gsrv, sizeof(struct sockaddr)) == -1
                && errno != EINPROGRESS)
        {
            SetSocketBlock(skfd, true);
            printf("connect [%d.%d.%d.%d] error[%d:%s]\n", ip.ipc[0], ip.ipc[1], ip.ipc[2], ip.ipc[3], errno, strerror(errno));
            return -1;
        }
        SetSocketBlock(skfd, true);

        fd_set Write;
        struct timeval timeout;
        timeout.tv_sec = conn_TIMEOUT/1000;
        timeout.tv_usec = conn_TIMEOUT%1000 * 1000;
        FD_ZERO(&Write);
        FD_SET(skfd, &Write);
        select(skfd+1, NULL, &Write, NULL, &timeout);	
        if (FD_ISSET(skfd, &Write))
        {
            if (i == conn_TIMES - 1)//最后一次进行ssl handshake
            {
                issl = tssl(skfd);
                close(skfd);
                return issl;
            }
            close(skfd);
        }
        else
        {
            close(skfd);
            return conn_TIMEOUT;
        }
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

        printf("IP[%lx] mask[%lx]\n", IPh[i].ipv4, Mask[i].ipv4);
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

    if (argc != 1)
    {
        initTest(argc - 1, argv+1);
    }
    else
    {
        initTest(3, address);
    }

    for (i = 0; i < 100; i++)
    {
        rand = random32();
        isel = rand % gaddCNT;
        tip.ipv4 = (rand & Mask[isel].ipv4) | IPh[isel].ipv4;

        timet = tconn(tip.ipv4);
        if (timet > 0 && timet < ssl_TIMEOUT)
        {
            printf("[%d][%d.%d.%d.%d]+[.%d]\n", i, tip.ipc[0], tip.ipc[1], tip.ipc[2], tip.ipc[3], timet);
        }
    }
    return 0;
}
#endif
