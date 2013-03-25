/* 
 * 功能：dns报文处理
 * 解包封包
 * */

#include "util.hpp"
#include "dnsutil.hpp"

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
    int i;
    int buflen = *((*uint16)m_srvbuf);
    char *cur = m_srvbuf + sizeof(uint16) + sizeof(DnsHead);
    char *str;
    int offset, iQue;

    if (m_srvbuf == NULL || buflen <= sizeof(DnsHead))
    {
        Notify(PRT_INFO, "[transdns:%d] buf is null!", __LINE__);
        m_Qname[0] = m_Qname[1] = 0;
        return 0;
    }
    memcpy(&m_head, buf, sizeof(m_head));

    Notify(PRT_TEST, "ques=%d, ans=%d", ntohs(m_head.Quests), ntohs(m_head.Ansers));
    iQue = ntohs(m_head.Quests);
    if (iQue > DNSQUERYMAXREC)
    {
        m_Qname[0] = m_Qname[1] = 0;
        return ERR_dns_query_toomany;
    }

    for (i = 0; i < iQue; i++)
    {//如果查询多结果，最后一个会覆盖前面
        buflen = sizeof(uint16);
        str = cur;
        offset = 0;
        while (*str != 0x00)
        {
            if ((*str & 0xc0) == 0xc0)
            {
                if (offset == 0) cur++;
                offset = (*str&0x03)*256 + *(str+1);
                str = buf+offset;
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
    memcpy(m_Qname, buflen, siezof(uint16));
    Notify(PRT_INFO, "ques[%d] = %s", buflen, m_Qname + sizeof(uint16));

    return iQue;
}

//解析m_clibuf到m_AIP
int dnsutil::unpackAnswer(void)
{
    int i;
    int buflen = *((*uint16)m_clibuf);
    char *cur = m_clibuf + sizeof(uint16) + sizeof(DnsHead);
    char *str;
    int offset, iQue, iAns;

    if (m_clibuf == NULL || buflen <= sizeof(DnsHead))
    {
        Notify(PRT_INFO, "[transdns:%d] buf is null!", __LINE__);
        m_AIP[0] = m_AIP[1] = 0;
        return 0;
    }
    Notify(PRT_INFO, "ques=%d, ans=%d", ntohs(m_head.Quests), ntohs(m_head.Ansers));

    iQue = ntohs(m_head.Quests);

    for (i = 0; i < iQue; i++)
    {
        str = cur;
        offset = 0;
        while (*str != 0x00)
        {
            if ((*str & 0xc0) == 0xc0)
            {
                if (offset == 0) cur++;
                offset = (*str&0x03)*256 + *(str+1);
                str = buf+offset;
            }
            if (offset == 0) cur += *str + 1;
            str += *str+1;
        }
        cur += 5;
    }

    iAns = ntohs(m_head.Ansers);

    for (i = 0; i < iAns; i++)
    {
        str = cur;
        offset = 0;
        while (*str != 0x00)
        {
            if ((*str & 0xc0) == 0xc0)
            {
                if (offset == 0) cur++;
                offset = (*str&0x03)*256 + *(str+1);
                str = buf + offset;
            }
            if (offset == 0) cur += *str + 1;
            str += *str+1;
        }
        cur++;
        ans[i].type = ntohs(*((short*)cur));
        cur+=2;
        ans[i].class = ntohs(*((short*)cur));
        cur += 2;
        ans[i].ttl = ntohl(*((long*)cur));
        cur+=4;
        ans[i].Addlen = ntohs(*((short*)cur));
        cur+=2;
        ans[i].ipadd.ipv4 = ntohl(*((unsigned long*)cur));
        cur += answer[i].Addlen;
        printf("[%s][%x][%x][%lx][%x]\n", answer[i].name, answer[i].type, answer[i].Class, ans[i].ttl, ans[i].Addlen);
        if (ans[i].Addlen == 4 && ans[i].type == 0x01 && ans[i].Class == 0x01)
        {
            return i;
        }
    }
    return 0;
}

int packQuery(char* buf, QueryRec* qry)
{}

//udp查询dns
int udpquery(char* sbuf, int slen)
{ 
    int skfd = -1, iret;
    struct sockaddr_in dsrv;
    struct timeval timeout;
    fd_set fdset;

    bzero(&dsrv, sizeof(dsrv));

    dsrv.sin_family = AF_INET;
    dsrv.sin_port = htons(53);
    dsrv.sin_addr.s_addr = inet_addr(srv);

    if ((skfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        Notify(PRT_INFO, "[transdns:%d] socket error[%d:%s]", __LINE__, errno, strerror(errno));
        return -1;
    }

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    FD_ZERO(&fdset);
    FD_SET(skfd, &fdset);
    select(skfd+1, NULL, &fdset, NULL, &timeout);
    if (FD_ISSET(skfd, &fdset))
    {
        iret = sendto(skfd, sbuf, slen, 0, (struct sockaddr*)&dsrv, sizeof(dsrv));
        Notify(PRT_INFO, "[transdns:%d] send[%d]error[%d:%s]", __LINE__, slen, errno, strerror(errno));
    }
    else
    {
        Notify(PRT_INFO, "[transdns:%d] send select error[%d:%s]", __LINE__, errno, strerror(errno));
        close(skfd);
        return -2;
    }
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    FD_ZERO(&fdset);
    FD_SET(skfd, &fdset);
    iret = select(skfd+1, &fdset, NULL, NULL, &timeout);
    if (iret == 0)
    {
        Notify(PRT_INFO, "[transdns:%d] recv select timeout", __LINE__);
        close(skfd);
        return -2;
    }
    if (iret < 0)
    {
        Notify(PRT_INFO, "[transdns:%d] recv select error[%d:%s]", __LINE__, errno, strerror(errno));
        close(skfd);
        return -3;
    }
    if (!FD_ISSET(skfd, &fdset))
    {
        Notify(PRT_INFO, "[transdns:%d] recv select not set", __LINE__);
        close(skfd);
        return -4;
    }
    iret = recvfrom(skfd, sbuf, 512, 0, NULL, NULL);
    Notify(PRT_INFO, "recv[%d]", iret);

    close(skfd);
    return iret;
}

void gendata()
{
    DnsHead* dh = (DnsHead*)data;
    dh->TranID = htons(0x1024);
    dh->Flags = htons(0x0100);
    dh->Quests = htons(0x01);
    dh->Ansers = htons(0x00);
    dh->Auths = htons(0x00);
    dh->Addits = htons(0x00);
    int ilen = addr2dns(data+sizeof(DnsHead), qry);//

    udpquery(data, ilen+sizeof(DnsHead));
}

#ifdef ONLY_RUN
//参数：目标域名
int main(int argc, char** argv)
{
    qry = argv[1];
    gendata();
    return 0;
}
#endif
