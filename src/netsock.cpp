#include "util.hpp"
#include "netsock.hpp"

NetTCP::NetTCP()
{
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
    if ((m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    {
        Notify(PRT_ERROR, "[netsock:%d]socket error", __LINE__);
        return -1;
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
            return -2;
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
    select(m_sock+1, &fdset, NULL, NULL, &tt);	
    if (FD_ISSET(m_sock, &fdset) == 0)
    {
        return -1;
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
    struct timeval tstart, tend;
    SSLHead     sslh;
    SSLHSHead   HSh;

    iret = TCPConnect(m_connect_timeout);
    if (iret < 0 || iret >= m_connect_timeout)
    {
        return m_connect_timeout;
    }

    iret = TCPSend((char*)&m_hello, sizeof(m_hello), m_SSL_send_timeout);
    if (iret < 0 || iret >= m_SSL_send_timeout)
    {
        TCPClose();
        return m_SSL_send_timeout;
    }

    gettimeofday(&tstart, NULL);

    iret = TCPRecv((char*)&sslh, sizeof(sslh), m_SSL_recv_timeout);
    if (iret < 0)
    {
        TCPClose();
        return m_SSL_recv_timeout;
    }
    iret = TCPRecv((char*)&HSh, sizeof(HSh), m_SSL_recv_timeout);
    if (iret < 0 || sslh.ContentType != 0x16 || HSh.HandshakeType != 0x02)
    {
        TCPClose();
        return m_SSL_recv_timeout;
    }

    gettimeofday(&tend, NULL);
    TCPClose();

    return (tend.tv_sec - tstart.tv_sec) * 1000 +
        (tend.tv_usec - tstart.tv_usec + 500) / 1000;
}

void SSLTest::SetTimeout(int conn, int send, int recv)
{
    m_connect_timeout = conn;
    m_SSL_send_timeout = send;
    m_SSL_recv_timeout = recv;
}
