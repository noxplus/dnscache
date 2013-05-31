#include "util.hpp"
#include "netsock.hpp"

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif

/*===================================================*/
class NetInit
{
    private:
        NetInit(NetInit&);
        NetInit& operator=(const NetInit&);
    public:
        static NetInit* GetInstance()
        {
            static NetInit m_Inst;
            return &m_Inst;
        };

    private:
        NetInit()
        {
            Notify(PRT_NOTICE, "init Net Mod");
#ifdef _WIN32
            WSADATA wsaData;
            // Initialize Winsock version 2.2
            if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0)
            {
                Notify(PRT_ERROR, "WSACleanup error %d",
                        WSAGetLastError());
            }
#endif
        }
    public:
        ~NetInit()
        {
#ifdef _WIN32
            if (WSACleanup() == SOCKET_ERROR)
            {
                Notify(PRT_ERROR, "WSACleanup error %d",
                        WSAGetLastError());
            }
#endif
        }
};

NetInit * initnetmod = NetInit::GetInstance();
/*===================================================*/

IPBlock::IPBlock(void)
{
    m_IPnet = m_IPmask = NULL;
    m_blockcnt = 0;
}
IPBlock::IPBlock(const char* blocks)
{
    m_IPnet = m_IPmask = NULL;
    m_blockcnt = 0;
    Init(blocks);
}
IPBlock::~IPBlock(void)
{
    if (m_IPnet != NULL)
    {
        free(m_IPnet);
        m_IPnet = NULL;
    }
    if (m_IPmask != NULL)
    {
        free(m_IPmask);
        m_IPmask = NULL;
    }
}
int IPBlock::Init(const char* blocks)
{
    unsigned int a,b,c,d,l;
    int i;

    for (i = 0;; i++)
    {
        if (sscanf(blocks, "%d.%d.%d.%d/%d", &a, &b, &c, &d ,&l) != 5)
        {
            break;
        }
        m_IPnet = (IPv4*)realloc(m_IPnet, (i+1)*sizeof(IPv4));
        m_IPmask = (IPv4*)realloc(m_IPmask, (i+1)*sizeof(IPv4));

        m_IPnet[i].ipc[0] = a;
        m_IPnet[i].ipc[1] = b;
        m_IPnet[i].ipc[2] = c;
        m_IPnet[i].ipc[3] = d;

        m_IPmask[i].ipc[0] = 0xFF >> l;
        m_IPmask[i].ipc[1] = 0xFF >> (l >= 8?l-8:0);
        m_IPmask[i].ipc[2] = 0xFF >> (l >= 16?l-16:0);
        m_IPmask[i].ipc[3] = 0xFF >> (l >= 24?l-24:0);
        if (l >= 32) m_IPmask[i].ipv4 = 0U;

        m_blockcnt++;

        Notify(PRT_INFO, "[IPBlock]IP[%lx]mask[%lx]", __LINE__,
                m_IPnet[i].ipv4, m_IPmask[i].ipv4);
        blocks = strchr(blocks, ',');
        if (blocks == NULL) break;
        while(*blocks < '1' || *blocks > '9')
        {
            if (*++blocks == 0) break;
        }
    }

    return m_blockcnt;
}
uint32 IPBlock::GetCnt(void)
{
    return m_blockcnt;
}
uint32 IPBlock::GetRandIP(void)
{
    uint32 tip, isel;
    tip = random32();
    isel = tip % m_blockcnt;
    tip = (tip & m_IPmask[isel].ipv4) | m_IPnet[isel].ipv4;
    return tip;
}

void Network::Close()
{
    if ((m_scktype & SCKTP_THIS) && (m_sock != -1))
    {
#ifdef _WIN32
        closesocket(m_sock);
#endif
#ifdef __linux__
        close(m_sock);
#endif
        m_sock = -1;
        m_scktype = SCKTP_NONE;
    }
    return;
}

NetUDP::NetUDP()
{
    m_scktype = SCKTP_UDP | SCKTP_THIS;
    m_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
}
NetUDP::NetUDP(int sock)
{
    m_scktype = SCKTP_UDP | SCKTP_COPY;
    m_sock = sock;
}
NetUDP::NetUDP(NetUDP& rhs)
{
    m_scktype = SCKTP_UDP | SCKTP_COPY;
    m_sock = rhs.m_sock;
    memcpy(&remote, &(rhs.remote), sizeof(remote));
}
NetUDP::~NetUDP()
{
    Close();
}
int NetUDP::UDPBind(uint16 port)
{
    struct sockaddr_in  local;

    bzero(&local, sizeof(local));
    local.sin_family = AF_INET;
    local.sin_port = htons(port);
    local.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (bind(m_sock, (struct sockaddr*)&local, sizeof(local)) == -1)
    {
        Notify(PRT_ERROR, err2str(ERR_net_bind_error));
        return ERR_net_bind_error;
    }

    return 0;
}

int NetUDP::UDPSend(const char* buf, int len, int timeout)
{
    int iret;
    struct timeval tv;
    fd_set fdw;

    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;
    FD_ZERO(&fdw);
    FD_SET(m_sock, &fdw);
    iret = select(SelSck(m_sock), NULL, &fdw, NULL, &tv);
    if (iret == 0) return ERR_net_select_timeout;
    if (iret < 0 || FD_ISSET(m_sock, &fdw) == 0)
    {
        Notify(PRT_ERROR, err2str(ERR_net_select_error));
        return ERR_net_select_error;
    }

    iret = sendto(m_sock, buf, len, 0, (sockaddr*)&remote, sizeof(remote));
    if (iret < 0) return ERR_net_send_error;

    return iret;
}
int NetUDP::UDPRecv(char* buf, int maxlen, int timeout)
{
    int iret;
    socklen_t   slen = sizeof(struct sockaddr_in);
    struct timeval tv;
    fd_set fdr;

    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;
    FD_ZERO(&fdr);
    FD_SET(m_sock, &fdr);
    iret = select(SelSck(m_sock), &fdr, NULL, NULL, &tv);
    if (iret == 0) return ERR_net_recv_timeout;
    if (iret < 0 || FD_ISSET(m_sock, &fdr) == 0)
    {
        Notify(PRT_ERROR, err2str(ERR_net_select_error));
        return ERR_net_select_error;
    }

    iret = recvfrom(m_sock, buf, maxlen, 0, (sockaddr*)&remote, &slen);
    if (iret < 0) return ERR_net_recv_error;

    return iret;
}

NetTCP::NetTCP()
{
    m_scktype = SCKTP_TCP | SCKTP_THIS;
    m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
}

NetTCP::~NetTCP()
{
    Close();
}

//connect in timeout~
int NetTCP::TCPConnect(int timeout)
{
    int i, iret = -1;

    if (m_sock == -1)
    {
        m_scktype = SCKTP_TCP | SCKTP_THIS;
        m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    }

    SetSockBlock(false);
    for (i = 0; i < timeout; i+= 50)
    {
        iret = connect(m_sock, (struct sockaddr*)&remote, sizeof(remote));
#ifdef _WIN32
        if (iret == 0 || WSAGetLastError() == WSAEISCONN)
        {
            SetSockBlock(true);
            return 0;
        }
        if (iret == -1 &&
                WSAGetLastError() != WSAEWOULDBLOCK &&
                WSAGetLastError() != WSAEINPROGRESS &&
                WSAGetLastError() != WSAEALREADY
           )
        {
            Notify(PRT_ERROR, err2str(ERR_net_connect_error));
            SetSockBlock(true);
            return ERR_net_connect_error;
        }
#endif
#ifdef __linux__
        if (iret == 0 || errno == EISCONN)
        {
            SetSockBlock(true);
            return 0;
        }
        if (iret == -1 && errno != EINPROGRESS && errno != EALREADY)
        {
            Notify(PRT_ERROR, err2str(ERR_net_connect_error));
            SetSockBlock(true);
            return ERR_net_connect_error;
        }
#endif
        SleepMS(50);
    }
    SetSockBlock(true);
    if (iret != 0)
    {
        return ERR_net_connect_timeout;
    }
    return 0;
}

//send buf in timeout
int NetTCP::TCPSend(const char* buf, int slen, int timeout)
{
    int iret = -1, isent = 0;
    struct timeval tt;
    fd_set fdset;

    tt.tv_sec = timeout / 1000;
    tt.tv_usec = timeout % 1000 * 1000;
    while (tt.tv_sec != 0 || tt.tv_usec != 0)
    {
        FD_ZERO(&fdset);
        FD_SET(m_sock, &fdset);
        select(SelSck(m_sock), NULL, &fdset, NULL, &tt);
        if (FD_ISSET(m_sock, &fdset) == 0)
        {
            continue;
        }
        iret = send(m_sock, buf + isent, slen - isent, 0);
        if (iret < 0) return ERR_net_send_error;
        isent += iret;
        if (isent >= slen) return isent;
    }
    return isent;
}

//recv buf in timeout
//return time
int NetTCP::TCPRecv(char *buf, int rlen, int timeout)
{
    int iret = -1, irecv = 0;
    struct  timeval tt;
    fd_set  fdset;

    tt.tv_sec = timeout/1000;
    tt.tv_usec = timeout%1000 * 1000;
    while (tt.tv_sec != 0 || tt.tv_usec != 0)
    {
        FD_ZERO(&fdset);
        FD_SET(m_sock, &fdset);
        select(SelSck(m_sock), &fdset, NULL, NULL, &tt);
        if (FD_ISSET(m_sock, &fdset) == 0)
        {
            continue;
        }
        iret = recv(m_sock, buf + irecv, rlen - irecv, 0);
        if (iret < 0) return ERR_net_recv_error;
        irecv += iret;
        if (irecv >= rlen) return irecv;
    }
    return irecv;
}

//clear recv buf in timeout
//return bytes
int NetTCP::TCPClear(int timeout)
{
    int iret = -1, irecv = 0;
    char buf[1024];
    struct  timeval tt;
    fd_set  fdset;

    tt.tv_sec = timeout/1000;
    tt.tv_usec = timeout%1000 * 1000;
    while (tt.tv_sec != 0 || tt.tv_usec != 0)
    {
        FD_ZERO(&fdset);
        FD_SET(m_sock, &fdset);
        iret = select(SelSck(m_sock), &fdset, NULL, NULL, &tt);
        if (iret == 0)//timeout
        {
            return irecv;
        }
        if (FD_ISSET(m_sock, &fdset) == 0)
        {
            continue;
        }
        iret = recv(m_sock, buf, 1024, 0);
        if (iret < 0) return ERR_net_recv_error;
        irecv += iret;
    }
    return irecv;
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
    uint32 tstart = 0, tend = 0;
    SSLHead     sslh;
    SSLHSHead   HSh;

    iret = TCPConnect(m_connect_timeout);
    if (iret >= ERR_no)
    {
        Close();
        return iret;
    }

    iret = TCPSend((char*)&m_hello, sizeof(m_hello), m_SSL_send_timeout);
    if (iret >= ERR_no)
    {
        Close();
        return ERR_net_send_timeout;
    }

    tstart = GetTimeMs();

    iret = TCPRecv((char*)&sslh, sizeof(sslh), m_SSL_recv_timeout);
    if (iret >= ERR_no)
    {
        Close();
        return ERR_net_recv_timeout;
    }
    iret = TCPRecv((char*)&HSh, sizeof(HSh), m_SSL_recv_timeout);
    if (iret >= ERR_no)
    {
        Close();
        return ERR_net_recv_timeout;
    }
    if (sslh.ContentType != 0x16 || HSh.HandshakeType != 0x02)
    {
        Close();
        return ERR_net_recv_error;
    }

    tend = GetTimeMs();

    iret = TCPClear(100);
    //Notify(PRT_INFO, "clear [%d]", iret);

    Close();

    return tend - tstart;
}

void SSLTest::SetTimeout(int conn, int send, int recv)
{
    m_connect_timeout = conn;
    m_SSL_send_timeout = send;
    m_SSL_recv_timeout = recv;
}
