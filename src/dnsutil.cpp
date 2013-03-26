/* 
 * 功能：dns报文处理
 * 解包封包
 * */

#include "util.hpp"
#include "dnsutil.hpp"

//class=0x0001 type=0x0001//net byte order
const static char TypeClass[4] = {0x00, 0x01, 0x00, 0x01};

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

//解析m_srvbuf到m_dnshead, m_Qname
int dnsutil::unpackQuery(void)
{
    uint16 buflen = *((uint16*)m_srvbuf);
    char *cur = m_srvbuf + sizeof(uint16);
    char *str;
    int offset, iQue;

    if (buflen <= sizeof(m_head))
    {
        Notify(PRT_INFO, "[transdns:%d] buf error!", __LINE__);
        memset(m_Qname, 0, DNSNAMEMAXLEN);
        return 0;
    }
    memcpy(&m_head, cur, sizeof(m_head));
    cur += sizeof(m_head);

    Notify(PRT_TEST, "ques=%d, ans=%d", ntohs(m_head.Quests), ntohs(m_head.Ansers));
    iQue = ntohs(m_head.Quests);
    if (iQue > DNSQUERYMAXREC)
    {
        memset(m_Qname, 0, DNSNAMEMAXLEN);
        return ERR_dns_query_toomany;
    }

    for (int i = 0; i < iQue; i++)
    {//如果查询多结果，后一个会覆盖前面
        buflen = sizeof(uint16);
        str = cur;
        offset = 0;
        while (*str != 0x00)
        {
            if ((*str & 0xc0) == 0xc0)
            {
                if (offset == 0) cur++;
                offset = (*str&0x03)*256 + *(str+1);
                str = m_srvbuf + sizeof(uint16) + offset;
            }
            if (offset == 0) cur += *str + 1;
            memcpy(m_Qname + buflen, str, *str+1);
            buflen += *str+1;
            str += *str+1;
        }
        //cur++;
        //type = ntohs(*((short*)cur));
        //cur+=2;
        //class = ntohs(*((short*)cur));
        //cur += 2;
        cur += 5;
    }
    *((uint16*)m_Qname) = buflen - sizeof(uint16);
    Notify(PRT_INFO, "ques[%d] = %s", buflen, m_Qname + sizeof(uint16));

    return iQue;
}

//解析m_clibuf到m_DNSRec
int dnsutil::unpackAnswer(void)
{
    //uint16 buflen = *((uint16*)m_clibuf);
    char *cur = m_clibuf + sizeof(uint16);
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
                str = m_clibuf + sizeof(uint16) + offset;
            }
            if (offset == 0) cur += *str + 1;
            str += *str+1;
        }
        cur += 5;
    }

    AnswerRec   ans;
    iAns = ntohs(thead->Ansers);
    for (int i = 0; i < iAns; i++)
    {
        str = cur;
        offset = 0;
        while (*str != 0x00)
        {
            if ((*str & 0xc0) == 0xc0)
            {
                if (offset == 0) cur++;
                offset = (*str&0x03)*256 + *(str+1);
                str = m_clibuf + sizeof(uint16) + offset;
            }
            if (offset == 0) cur += *str + 1;
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
        printf("[%s][%x][%x][%lx][%x]\n", ans.Name, ans.Type, ans.Class, ans.TTL, ans.Addlen);
        if (ans.Addlen == 4 && ans.Type == 0x01 && ans.Class == 0x01)
        {
            m_DNSRec->ttl = ans.TTL;
            m_DNSRec->ip.ipv4 = ans.IPAdd.ipv4;
            return i + 1;//找到可用地址
        }
    }
    return 0;
}

//从m_Qname组织查询报文到m_clibuf
int dnsutil::packQuery(void)
{
    char* cur = m_clibuf + sizeof(uint16);
    uint16 len = *((uint16*)m_Qname);
    DNSHead* dh = (DNSHead*)cur;
    dh->TranID = htons(0x1024);
    dh->Flags = htons(0x0100);
    dh->Quests = htons(0x01);
    dh->Ansers = htons(0x00);
    dh->Auths = htons(0x00);
    dh->Addits = htons(0x00);
    cur += sizeof(DNSHead);
    memcpy(cur, m_Qname + sizeof(uint16), len);
    cur += len;
    memcpy(cur, TypeClass, sizeof(TypeClass));

    *((uint16*)m_clibuf) = sizeof(DNSHead) + len + sizeof(TypeClass);

    return sizeof(DNSHead) + len + sizeof(TypeClass);
}

//根据m_head, m_Qname, m_DNSRec, 组织回复报文到m_srvbuf
int dnsutil::packAnswer(void)
{
    char* cur = m_srvbuf + sizeof(uint16);
    uint16 len = *((uint16*)m_Qname);
    memcpy(cur, &m_head, sizeof(DNSHead));
    cur += sizeof(DNSHead);
    //query
    memcpy(cur, m_Qname + sizeof(uint16), len);
    cur += len;
    memcpy(cur, TypeClass, sizeof(TypeClass));
    cur += sizeof(TypeClass);
    //answer
    memcpy(cur, m_Qname + sizeof(uint16), len);
    cur += len;
    memcpy(cur, TypeClass, sizeof(TypeClass));
    cur += sizeof(TypeClass);
    *((uint32*)cur) = htonl(m_DNSRec->ttl);
    cur += sizeof(uint32);
    *((uint16*)cur) = htonl(sizeof(m_DNSRec->ip));
    cur += sizeof(uint16);
    *((uint32*)cur) = htonl(sizeof(m_DNSRec->ip.ipv4));
    cur += sizeof(uint32);

    return *((uint16*)m_srvbuf) = cur - m_srvbuf - sizeof(uint16);
}

#ifdef ONLY_RUN
//参数：目标域名
int main(int argc, char** argv)
{
    return 0;
}
#endif
