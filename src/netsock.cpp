#include "util.hpp"
#include "netsock.hpp"

IPBlock::NetIP(void)
{
    IPnet = IPmask = NULL;
}
IPBlock::NetIP(const char* blocks)
{

    unsigned int a,b,c,d,l;
    int i;

    for (i = 0;; i++)
    {
        if (sscanf(blocks, "%d.%d.%d.%d/%d", &a, &b, &c, &d ,&l) != 5)
        {
            break;
        }
        IPnet = (IPv4*)realloc(IPnet, (i+1)*sizeof(IPv4));
        IPmask = (IPv4*)realloc(IPmask, (i+1)*sizeof(IPv4));

        IPnet[i].ipc[0] = a;
        IPnet[i].ipc[1] = b;
        IPnet[i].ipc[2] = c;
        IPnet[i].ipc[3] = d;

        IPmask[i].ipc[0] = 0xFF >> l;
        IPmask[i].ipc[1] = 0xFF >> (l >= 8?l-8:0);
        IPmask[i].ipc[2] = 0xFF >> (l >= 16?l-16:0);
        IPmask[i].ipc[3] = 0xFF >> (l >= 24?l-24:0);
        if (l >= 32) IPmask[i].ipv4 = 0U;

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
}

NetTCP::NetTCP()
{
#ifdef _WIN32
    wsaStatus(true);
#endif
}

NetTCP::~NetTCP()
{
    if (m_sock != -1)
    {
        close(m_sock);
        m_sock = -1;
    }
#ifdef _WIN32
    wsaStatus(false);
#endif
    return;
}

#ifdef _WIN32
bool NetTCP::wsaStatus(bool status)
{
    bool curStatus = false;
    if (curStatus == status) return true;

    if (status == true)
    {//初始化wsa
        WSADATA wsaData;
        // Initialize Winsock version 2.2
        if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0)
        {
            Notify(PRT_ERROR, "WSAstart up error");
            return false;
        }

        return curStatus = true;
    }

    //清理wsa
    if (WSACleanup() == SOCKET_ERROR)
    {
        printf("WSACleanup failed with error %d\n", WSAGetLastError());
        return false;
    }

    curStatus = false;
    return true;
}
#endif

//connect in timeout~
int NetTCP::TCPConnect(int timeout)
{
    int i, iret;
    if ((m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    {
        Notify(PRT_ERROR, "[netsock:%d]socket error", __LINE__);
        return ERR_sock_error;
    }

    SetSockBlock(false);
    for (i = 0; i < timeout; i+= 50)
    {
        iret = connect(m_sock, (struct sockaddr*)&remote, sizeof(remote));
        if (iret == 0 || errno == EISCONN) break;//OK~
        if (iret == -1 && errno != EINPROGRESS && errno != EALREADY)
        {
            SetSockBlock(true);
            Notify(PRT_ERROR, "[%d]connect", __LINE__);
            return ERR_conn_timeout;
        }
        usleep(50 * 1000);
    }
    SetSockBlock(true);
    return 0;
}
void NetTCP::TCPClose()
{
    if (m_sock != -1)
    {
        close(m_sock);
        m_sock = -1;
    }
    return;
}

//send buf in timeout
//return time ( <0 in error)
int NetTCP::TCPSend(char* buf, int slen, int timeout)
{
    struct timeval tt;
    fd_set fdset;

    tt.tv_sec = timeout / 1000;
    tt.tv_usec = timeout % 1000 * 1000;
    FD_ZERO(&fdset);
    FD_SET(m_sock, &fdset);
    select(m_sock+1, NULL, &fdset, NULL, &tt);	
    if (FD_ISSET(m_sock, &fdset) == 0)
    {
        return ERR_send_error;
    }
    return send(m_sock, buf, slen, 0);
}

//recv buf in timeout
//return time ( <0 in error)
int NetTCP::TCPRecv(char *buf, int rlen, int timeout)
{
    struct  timeval tt;
    fd_set  fdset;

    tt.tv_sec = timeout/1000;
    tt.tv_usec = timeout%1000 * 1000;
    FD_ZERO(&fdset);
    FD_SET(m_sock, &fdset);
    select(m_sock+1, &fdset, NULL, NULL, &tt);	
    if (FD_ISSET(m_sock, &fdset) == 0)
    {
        return ERR_recv_error;
    }
    return recv(m_sock, buf, rlen, 0);
}

SSLTest::SSLTest()
{
    m_hello.ssl.ContentType = 22;
    m_hello.ssl.SSLVer = htons(0x0301);
    m_hello.ssl.ContentLen = htons(sizeof(m_hello) - sizeof(m_hello.ssl));
    m_hello.hands.HandshakeType = 0x01;
    m_hello.hands.LenHi = 0x0;
    m_hello.hands.LenLo = htons(sizeof(m_hello) - sizeof(m_hello.ssl) - sizeof(m_hello.hands));
    m_hello.SSLVer = htons(0x0301);
    m_hello.sidLen = 0x0;
    m_hello.CipherSuitesLen = htons(0x02);
    m_hello.CipherSuites = htons(0xc011);
    m_hello.CompMethLen = 0x1;
    m_hello.CompMeth = 0x0;

    SetIPPort(0UL, 443);
}
SSLTest::~SSLTest()
{
}

int SSLTest::RunTest(void)
{
    int iret;
    uint32 tstart, tend;
    SSLHead     sslh;
    SSLHSHead   HSh;

    Notify(PRT_INFO, "conn %lu", GetTimeMs());
    iret = TCPConnect(m_connect_timeout);
    if (iret >= ERR_no)
    {
        return iret;
    }

    iret = TCPSend((char*)&m_hello, sizeof(m_hello), m_SSL_send_timeout);
    if (iret >= ERR_no)
    {
        TCPClose();
        return ERR_send_timeout;
    }

    tstart = GetTimeMs();

    iret = TCPRecv((char*)&sslh, sizeof(sslh), m_SSL_recv_timeout);
    if (iret >= ERR_no)
    {
        TCPClose();
        return ERR_recv_timeout;
    }
    iret = TCPRecv((char*)&HSh, sizeof(HSh), m_SSL_recv_timeout);
    if (iret >= ERR_no)
    {
        TCPClose();
        return ERR_recv_timeout;
    }   
    if (sslh.ContentType != 0x16 || HSh.HandshakeType != 0x02)
    {
        TCPClose();
        return ERR_recv_error;
    }

    tend = GetTimeMs();
    TCPClose();

    return tend - tstart;
}

void SSLTest::SetTimeout(int conn, int send, int recv)
{
    m_connect_timeout = conn;
    m_SSL_send_timeout = send;
    m_SSL_recv_timeout = recv;
}
