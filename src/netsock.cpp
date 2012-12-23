#include "netsock.hpp"

NetTCP::NetTCP()
{
    if ((m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    {
        Notify(PRT_ERROR, "[testgg:%d]socket error", __LINE__);
        return;
    }
}

NetTCP::~NetTCP()
{
    if (m_sock != -1)
    {
        close(m_sock);
        m_sock = -1;
    }
    return;
}

//connect in timeout~
int NetTCP::TCPConnect(int timeout)
{
    int i, iret;
    SetSockBlock(false);
    for (i = 0; i < timeout; i+= 50)
    {
        iret = connect(m_sock, (struct sockaddr*)&remote, sizeof(remote));
        if (iret == 0 || errno == EISCONN) break;//OK~
        if (iret == -1 && errno != EINPROGRESS && errno != EALREADY)
        {
            SetSockBlock(true);
            Notify(PRT_ERROR, "[%d]connect", __LINE__);
            return -1;
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
        return -1;
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
    select(m_sock+1, &fdset, NULL, NULL, &timeout);	
    if (FD_ISSET(m_sock, &fdset) == 0)
    {
        return -1;
    }
    return recv(m_sock, buf, rlen, 0);
}

SSLTest::SSLTest()
{
    NetTCP();

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

    SetIPPort(0L, 443);
}

int SSLTest::RunTest(void)
{
    int iret;
    SSLHead     sslh;
    DNSHSHead   HSh;

    iret = TCPConnect();
    if (iret < 0 || iret >= m_Conn_Timeout)
    {
        return -2;
    }

    iret = TCPSend(&m_hello, sizeof(m_hello), m_SSL_Timeout);
    if (iret < 0 || iret >= m_SSL_Timeout)
    {
        return -2;
    }

    iret = TCPRecv(&sslh, sizeof(sslh));
    iret = TCPRecv(&HSh, sizeof(HSh));
    if (sslh.ContentLen != 0x16 || HSh.HandshakeType != 0x02)
    {
        return m_SSL_Timeout;
    }

    return iret;
}
