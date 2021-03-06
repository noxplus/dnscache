/* 
 * 功能：dns报文处理
 * 解包封包
 * */
#include <pthread.h>

#include "util.hpp"
#include "dnscache.hpp"

//type=0x0001 class=0x0001//net byte order
const static char TypeClass[4] = {0x00, 0x01, 0x00, 0x01};

static StaticCache srv_add;//查询网址的dns服务器
static StaticCache srv_dom;//查询域名的dns服务器
static StaticCache host_add;//存储网址的IP地址
static StaticCache host_dom;//存储域名的IP地址
static DynamicCache dnscache;//存储查询的缓存数据
static int rcount = 0;
static int tcount = 0;

#define BUFCNT  8
DNSutil dns[BUFCNT];

pthread_mutex_t mlock;
pthread_cond_t  mcond;

void* thd1func(void*);
void* thd2func(void*);

int GetIdle(void)
{
    int i = 0;
    while(true)
    {
        for (i = 0; i < BUFCNT; i++)
        {
            if (dns[i].SetUse() == STA_NONE) return i;
        }
        SleepMS(100);
    }
}

//参数：目标域名
int main(int argc, char** argv)
{
    int id = 0;
    pthread_t t1;
    pthread_cond_init(&mcond, NULL);
    NetUDP  srv;

    pthread_create(&t1, NULL, thd1func, NULL);
    srv.UDPBind(53);
    while(true)
    {
        Notify(PRT_TEST, "will recv");
        id = GetIdle();
        dns[id].SrvRecv(srv);
        if (dns[id].GetSta() & STA_SRV)
        {
            rcount++;
            Notify(PRT_TEST, "signal");
            pthread_cond_signal(&mcond);
        }
        else
        {
            dns[id].Release();
        }
    }
    return 0;
}

void* thd1func(void* tmp)
{
    int id;
    pthread_t t2;
    while (true)
    {
        pthread_mutex_lock(&mlock);
        while (rcount == tcount) pthread_cond_wait(&mcond, &mlock);
        if (rcount < tcount)
        {//
        }
        tcount++;
        for (id = 0; id < BUFCNT; id++)
        {
            if (dns[id].GetSta() & STA_SRV)
            {
                break;
            }
        }
        if (id == BUFCNT) continue;
        pthread_mutex_unlock(&mlock);

        dns[id].unpackQuery();
        dns[id].Search();

        if ((dns[id].GetSta() & STA_FND) == 0)
        {
            pthread_create(&t2, NULL, thd2func, (void*)id);
            continue;
        }
        dns[id].packAnswer();
        dns[id].SrvSend();
        if (dns[id].GetSta() & STA_CHK)
        {
            pthread_create(&t2, NULL, thd2func, (void*)id);
        }
        else dns[id].Release();
    }

    return NULL;
}

void* thd2func(void* tmp)
{
    int id = (int)tmp;

    dns[id].packQuery();
    dns[id].CliSend();
    dns[id].CliRecv();
    dns[id].unpackAnswer();

    if ((dns[id].GetSta() & STA_ACK) == 0)
    {
        dns[id].packAnswer();
        dns[id].SrvSend();
    }

    dns[id].UpdCache();
    dns[id].Release();

    return NULL;
}

//dns报文采取后一种格式存储地址，并做适当压缩。
//为了统一，内部也如此存储。
//配置文件/外部参数使用.分割字段，加上转换函数。
//地址格式转换：可视 => dns存储格式
//aa.bbb.cd => 2aa3bbb2cd
int addr2dns(char* dns, char* addr)
{
    int i = 0, l = 0;
    while(*addr)
    {
        if (++i == 63) break;
        if (*addr == '.')
        {
            dns[l] = i - l - 1;
            l = i;
            addr++;
            continue;
        }
        dns[i] = *addr;
        addr++;
    }
    dns[l] = i - l;
    dns[i + 1] = 0;

    return i + 2;//必须包含末尾的0x00
    //dns[i+1+1] = 0x00;
    //dns[i+1+2] = 0x01;//type = 0x0001 netbyte
    //dns[i+1+3] = 0x00;
    //dns[i+1+4] = 0x01;//class = 0x0001 netbyte
    //return i + 6;
}
//地址格式转换：
//2aa3bbb2cd => aa.bbb.cd
int dns2addr(char* addr, char* dns)
{
    int i = 0, l, t=0;
    while (*dns)
    {
        t++;
        l = *dns++;
        if (l < 0 || l >= 64) return -1;
        for(i = 0; *dns && i < l; i++)
        {
            *addr++ = *dns++;
            t++;
        }
        if (*dns == 0)
        {
            *addr = 0;
            break;
        }
        *addr++ = '.';
    }
    return t;
}

QName::QName(void)
{
    m_len = 0;
    memset(m_name, 0, sizeof(m_name));
}
QName::QName(const char* str)
{//分段copy，严格校验
    int dlen = 0;
    m_len = 0;
    memset(m_name, 0, sizeof(m_name));
    while (m_len + *str < DNSNAMEMAXLEN - 1 && *str != 0)
    {
        dlen = *str + 1;
        memcpy(m_name + m_len, str, dlen);
        m_len += dlen;
        str += dlen;
    }
    m_name[m_len] = 0;
    m_len++;
    for (int i = 0; i < m_len; i++)
    {
        if ('A' <= m_name[i] && m_name[i] <= 'Z')
        {
            m_name[i] &= 0x20;//转小写
        }
    }
}
int QName::reset(const char* str)
{//分段copy，严格校验
    int dlen = 0;
    m_len = 0;
    memset(m_name, 0, sizeof(m_name));
    while (m_len + *str < DNSNAMEMAXLEN - 1 && *str != 0)
    {
        dlen = *str + 1;
        memcpy(m_name + m_len, str, dlen);
        m_len += dlen;
        str += dlen;
    }
    m_name[m_len] = 0;
    m_len++;
    for (int i = 0; i < m_len; i++)
    {
        if ('A' <= m_name[i] && m_name[i] <= 'Z')
        {
            m_name[i] &= 0x20;//转小写
        }
    }
    return m_len;
}
QName::QName(const QName& rr)
{
    m_len = rr.m_len;
    memcpy(m_name, rr.m_name, sizeof(m_name));
}
int QName::reset(const QName& rr)
{
    m_len = rr.m_len;
    memcpy(m_name, rr.m_name, sizeof(m_name));
    return m_len;
}

int QName::GetSubName(QName& to)
{
    int offset = (int)m_name[0];
    if (offset + 1 >= m_len) return 0;

    to.m_len = m_len - offset -1;
    memcpy(to.m_name, m_name + offset + 1, to.m_len);
    return to.m_len;
}
int QName::Cut(void)
{
    int offset = (int)m_name[0];
    if (offset + 1 >= m_len) return 0;

    m_len = m_len - offset -1;
    memcpy(m_name, m_name + offset + 1, m_len);
    return m_len;
}

bool QName::operator<(const QName& rr) const
{
    if (m_len < rr.m_len) return true;
    if (m_len > rr.m_len) return false;
    return (memcmp(m_name, rr.m_name, m_len) < 0);
}
bool QName::operator>(const QName& rr) const
{
    if (m_len > rr.m_len) return true;
    if (m_len < rr.m_len) return false;
    return (memcmp(m_name, rr.m_name, m_len) > 0);
}
bool QName::operator==(const QName& rr) const
{
    if (m_len != rr.m_len) return false;
    return (memcmp(m_name, rr.m_name, m_len) == 0);
}

int QName::getlen(void) const
{
    return m_len;
}
int QName::clear(void)
{
    memset(m_name, 0, sizeof(m_name));
    return (m_len = 0);
}
int QName::copyto(char* str)
{
    memcpy(str, m_name, m_len);
    return m_len;
}

void QName::print(void) const
{
    Notify(PRT_TEST, "%s", m_name);
}


//解析m_srvbuf到m_dnshead, m_Qname
int DNSutil::unpackQuery(void)
{
    char *cur = m_srvbuf;
    char *str;
    int offset, iQue;
    DNSHead*    dh = (DNSHead*)cur;

    if (m_srvlen <= sizeof(DNSHead))
    {
        Notify(PRT_INFO, "[transdns:%d] buf error!", __LINE__);
        m_Qname.clear();
        return 0;
    }
    cur += sizeof(DNSHead);

    m_srvID = dh->TranID;
    Notify(PRT_TEST, "ques=%d, ans=%d", ntohs(dh->Quests), ntohs(dh->Ansers));
    iQue = ntohs(dh->Quests);
    if (iQue > DNSQUERYMAXREC)
    {
        m_Qname.clear();
        return ERR_dns_query_toomany;
    }

    for (int i = 0; i < iQue; i++)
    {//如果查询多结果，后一个会覆盖前面
        char Qname[DNSNAMEMAXLEN] = {0};
        int  buflen = 0;
        str = cur;
        offset = 0;
        while (*str != 0x00)
        {
            if ((*str & 0xc0) == 0xc0)
            {
                if (offset == 0) cur++;
                offset = (*str&0x03)*256 + *(str+1);
                str = &m_srvbuf[offset];
            }
            if (offset == 0) cur += *str + 1;
            memcpy(Qname + buflen, str, *str+1);
            buflen += *str+1;
            str += *str+1;
        }
        //cur++;
        //type = ntohs(*((short*)cur));
        //cur+=2;
        //class = ntohs(*((short*)cur));
        //cur += 2;
        cur += 5;
        m_Qname.reset(Qname);
        m_Qname.print();
    }

    return iQue;
}

//解析m_clibuf到m_DNSRec
int DNSutil::unpackAnswer(void)
{
    char *cur = m_clibuf;
    char *str;
    int offset, iQue, iAns;
    DNSHead *thead = (DNSHead*)cur;

    cur += sizeof(DNSHead);
    Notify(PRT_INFO, "ques=%d, ans=%d", ntohs(thead->Quests), ntohs(thead->Ansers));

    iQue = ntohs(thead->Quests);
    for (int i = 0; i < iQue; i++)
    {
        str = cur;
        offset = 0;
        while (*str != 0x00)
        {
            if ((*str & 0xc0) == 0xc0)
            {
                if (offset == 0) cur++;
                offset = (*str&0x03)*256 + *(str+1);
                str = &m_clibuf[offset];
            }
            if (offset == 0) cur += *str + 1;
            str += *str+1;
        }
        cur += 5;
    }

    AnswerRec   ans;
    memset(&ans, 0, sizeof(ans));
    iAns = ntohs(thead->Ansers);
    for (int i = 0; i < iAns; i++)
    {
        int slen = 0;
        str = cur;
        offset = 0;
        while (*str != 0x00)
        {
            if ((*str & 0xc0) == 0xc0)
            {
                if (offset == 0) cur++;
                offset = (*str&0x03)*256 + *(str+1);
                str = &m_clibuf[offset];
            }
            if (offset == 0) cur += *str + 1;
            memcpy(ans.Name+slen, str, *str+1);
            slen += *str+1;
            str += *str+1;
        }
        cur++;
        ans.Type = ntohs(*((uint16*)cur));
        cur += 2;
        ans.Class = ntohs(*((uint16*)cur));
        cur += 2;
        ans.TTL = ntohl(*((uint32*)cur));
        cur += 4;
        ans.Addlen = ntohs(*((uint16*)cur));
        cur += 2;
        ans.IPAdd.ipv4 = ntohl(*((uint32*)cur));
        cur += ans.Addlen;
        if (ans.Addlen == 4 && ans.Type == 0x01 && ans.Class == 0x01)
        {
            m_DNSRec.ttl = ans.TTL + time(NULL);
            m_DNSRec.ip.ipv4 = ans.IPAdd.ipv4;
            return i + 1;//找到可用地址
        }
    }
    return 0;
}

//从m_Qname组织查询报文到m_clibuf
int DNSutil::packQuery(void)
{
    char* cur = m_clibuf;
    DNSHead* dh = (DNSHead*)cur;
    dh->TranID = m_srvID;
    dh->Flags = htons(0x0100);
    dh->Quests = htons(0x01);
    dh->Ansers = htons(0x00);
    dh->Auths = htons(0x00);
    dh->Addits = htons(0x00);
    cur += sizeof(DNSHead);
    m_Qname.copyto(cur);
    cur += m_Qname.getlen();
    memcpy(cur, TypeClass, sizeof(TypeClass));

    m_clilen= sizeof(DNSHead) + m_Qname.getlen() + sizeof(TypeClass);

    return m_clilen;
}

//根据m_head, m_Qname, m_DNSRec, 组织回复报文到m_srvbuf
int DNSutil::packAnswer(void)
{
    int32 ttl;
    char* cur = m_srvbuf;
    DNSHead *dh = (DNSHead*)cur;
    dh->TranID = m_srvID;
    dh->Flags = 0x8180;
    dh->Quests = htons(1);
    dh->Ansers = htons(1);
    dh->Auths = 0;
    dh->Addits = 0;
    cur += sizeof(DNSHead);
    //query
    m_Qname.copyto(cur);
    cur += m_Qname.getlen();
    memcpy(cur, TypeClass, sizeof(TypeClass));
    cur += sizeof(TypeClass);
    //answer
    m_Qname.copyto(cur);
    cur += m_Qname.getlen();
    memcpy(cur, TypeClass, sizeof(TypeClass));
    cur += sizeof(TypeClass);
    ttl = m_DNSRec.ttl - time(NULL);
    *((uint32*)cur) = ttl < 0 ? 0 : ttl;
    cur += sizeof(uint32);
    *((uint16*)cur) = htonl(sizeof(m_DNSRec.ip));
    cur += sizeof(uint16);
    *((uint32*)cur) = htonl(sizeof(m_DNSRec.ip.ipv4));
    cur += sizeof(uint32);

    return m_srvlen = cur - m_srvbuf;
}

int DNSutil::SrvRecv(NetUDP& net)
{
    int iret;
    m_srv.SetSocket(net.GetSocket());
    iret = m_srv.UDPRecv(m_srvbuf, sizeof(m_srvbuf), 5000);
    if (iret >= ERR_no) m_srvlen = 0;
    else
    {
        m_srvlen = iret;
        m_sta |= STA_SRV;
    }
    return iret;
}
int DNSutil::SrvSend(void)
{
    m_sta |= STA_ACK;
    return m_srv.UDPSend(m_srvbuf, m_srvlen, 1000);
}

int DNSutil::CliSend(void)
{
    DNSServer();
    m_cli.SetIPPort(m_dnssrv.ipv4, 53);
    return m_cli.UDPSend(m_clibuf, m_clilen, 1000);
}
int DNSutil::CliRecv(void)
{
    int iret = m_cli.UDPRecv(m_clibuf, sizeof(m_clibuf), 5000);
    if (iret >= ERR_no) m_clilen = 0;
    else m_clilen = iret;
    return iret;
}

void DNSutil::Search(void)
{
    DNSRecord* fnd;

    if ((fnd = host_add.search(m_Qname)) != NULL)
    {
        memcpy(&m_DNSRec, fnd, sizeof(DNSRecord));
        m_sta |= STA_FND;
        return;
    }
        
    QName qdomain(m_Qname);
    while(qdomain.Cut() != 0)
    {
        if ((fnd = host_dom.search(qdomain)) != NULL)
        {
            memcpy(&m_DNSRec, fnd, sizeof(DNSRecord));
            m_sta |= STA_FND;
            return;
        }
    }

    if ((fnd = dnscache.search(m_Qname)) != NULL)
    {
        memcpy(&m_DNSRec, fnd, sizeof(DNSRecord));
        m_sta |= STA_FND;
        if (m_DNSRec.ttl < (uint32)time(NULL)) m_sta |= STA_CHK;
        return;
    }
}
void DNSutil::DNSServer(void)
{
    DNSRecord* fnd;
    m_dnssrv.ipv4 = 0x08080808;

    if ((fnd = srv_add.search(m_Qname)) != NULL)
    {
        m_dnssrv.ipv4 = fnd->ip.ipv4;
        return;
    }
        
    QName qdomain(m_Qname);
    while(qdomain.Cut() != 0)
    {
        if ((fnd = host_dom.search(qdomain)) != NULL)
        {
            m_dnssrv.ipv4 = fnd->ip.ipv4;
            return;
        }
    }
}
void DNSutil::UpdCache(void)
{
    dnscache.update(m_Qname, m_DNSRec);
}

int DNSutil::GetSta(void)
{
    return m_sta;
}
int DNSutil::SetUse(void)
{
    if (m_sta != STA_NONE) return m_sta;
    m_sta = STA_INIT;
    return STA_NONE;
}
int DNSutil::Release(void)
{
    m_sta = STA_NONE;
    m_srvlen = m_clilen = 0;
    m_Qname.clear();
    return STA_NONE;
}

DNSRecord* MemCache::search(const QName& qry)
{
    std::map<QName, DNSRecord>::iterator it;
    it = cache.find(qry);
    if (it == cache.end()) return NULL;
    return &(it->second);
}

int MemCache::update(const QName& ins, const DNSRecord& rec)
{
    std::pair<std::map<QName, DNSRecord>::iterator, bool> pret;
    pret = cache.insert(std::pair<QName, DNSRecord>(ins, rec));

    if (pret.second == false)
    {
        memcpy(&(pret.first->second), &rec, sizeof(DNSRecord));
    }
    return cache.size();
}

int StaticCache::init(char* file)
{
    return 0;
}

int DynamicCache::load(char* file)
{
    return 0;
}
int DynamicCache::save(char* file)
{
    return 0;
}
