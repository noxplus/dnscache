/* 查询指定域名的ip地址
 * 支持udp/tcp协议
 * 后续支持：根据返回数据的ttl过滤掉dns污染
 * 迂回：当udp查询，返回多次结果时，全部https测试。
 * 主要是针对https的，所以兼顾测试一下是否支持https。
 * */

#include "inc.h"

static char* srv = "8.8.8.8";//默认使用google dns。后续加入配置文件
static char* qry = "www.google.com";

static char data[1024];

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
    dns[i+1+1] = 0x00;
    dns[i+1+2] = 0x01;//type = 0x0001 netbyte
    dns[i+1+3] = 0x00;
    dns[i+1+4] = 0x01;//class = 0x0001 netbyte
    return i + 6;
}
int dns2addr(char* addr, char* dns)
{
    return 0;
}

//生成域名的序号
uint32 GenIndex(DNSRecode* rec)
{
    uint32 Index = 0;
    int i;
    for (i = 0; i < 4; i++)
    {
        Index ^= rec->uname.iname[i];
    }

    rec->index = Index;
    return Index;
}

//解析报文内容，放置查询请求地址到query地址里。
//本函数申请空间，外部调用负责释放
int unpackQuery(char* buf, QueryRec** query)
{
    int i;
    DnsHead *pHead = (DnsHead*)buf;
    char *cur = buf+sizeof(DnsHead);
    char *str;
    QueryRec* qry = NULL;
    int offset, iQue;

    Notify(__FILE__, __LINE__, PRT_INFO, "ques=%d, ans=%d", ntohs(pHead->Quests), ntohs(pHead->Ansers));
    iQue = ntohs(pHead->Quests);
    *query = qry = (QueryRec*)calloc(iQue, sizeof(QueryRec));

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
            memcpy(qry[i].name+qry[i].namelen, str, *str+1);
            qry[i].namelen += *str+1;
            str += *str+1;
        }
        cur++;
        qry[i].type = ntohs(*((short*)cur));
        cur+=2;
        qry[i].class = ntohs(*((short*)cur));
        cur += 2;
    }
    Notify(__FILE__, __LINE__, PRT_INFO, "ques=%s", qry->name);

    return iQue;
}

//申请的空间外部负责释放！
int unpackAnswer(char* buf, AnswerRec** ans)
{
    int i;
    DnsHead *pHead = (DnsHead*)buf;
    printf("ques=%d, ans=%d\n", ntohs(pHead->Quests), ntohs(pHead->Ansers));
    char *cur = buf+sizeof(DnsHead);
    char *str;
    int offset, iQue, iAns;

    QueryRec* query = NULL;
    AnswerRec* answer = NULL;

    iQue = ntohs(pHead->Quests);
    query = (QueryRec*)calloc(iQue, sizeof(QueryRec));

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
            memcpy(query[i].name+query[i].namelen, str, *str+1);
            query[i].namelen += *str+1;
            str += *str+1;
        }
        cur++;
        query[i].type = ntohs(*((short*)cur));
        cur+=2;
        query[i].class = ntohs(*((short*)cur));
        cur += 2;
    }

    free(query);//查询的结果没什么用
    query = NULL;

    iAns = ntohs(pHead->Ansers);
    *ans = answer = (AnswerRec*)calloc(iAns, sizeof(AnswerRec));

    for (i = 0; i < iAns; i++)
    {
        str = cur;
        printf("off: %x\t", cur - buf);
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
            memcpy(answer[i].name+answer[i].namelen, str, *str+1);
            answer[i].namelen += *str+1;
            str += *str+1;
        }
        cur++;
        answer[i].type = ntohs(*((short*)cur));
        cur+=2;
        answer[i].class = ntohs(*((short*)cur));
        cur += 2;
        answer[i].ttl = ntohl(*((long*)cur));
        cur+=4;
        answer[i].Addlen = ntohs(*((short*)cur));
        cur+=2;
        answer[i].ipadd.ipv4 = ntohl(*((unsigned long*)cur));
        cur += answer[i].Addlen;
        if (answer[i].Addlen == 4 && answer[i].type == 0x01 && answer[i].class == 0x01)
        {
            return i;
        }
        printf("[%s][%x][%x][%lx][%x]\n", answer[i].name, answer[i].type, answer[i].class, answer[i].ttl, answer[i].Addlen);
    }
    return 0;
}

int udpquery(char* sbuf, int* slen)
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
        Notify(__FILE__, __LINE__, PRT_INFO, "socket error[%d:%s]", errno, strerror(errno));
        return -1;
    }

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    FD_ZERO(&fdset);
    FD_SET(skfd, &fdset);
    select(skfd+1, NULL, &fdset, NULL, &timeout);
    if (FD_ISSET(skfd, &fdset))
    {
        sendto(skfd, sbuf, *slen, 0, (struct sockaddr*)&dsrv, sizeof(dsrv));
    }
    else
    {
        Notify(__FILE__, __LINE__, PRT_INFO, "send select error[%d:%s]", errno, strerror(errno));
        close(skfd);
        return 1;
    }
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    while (timeout.tv_sec !=0 || timeout.tv_usec != 0)
    {
        FD_ZERO(&fdset);
        FD_SET(skfd, &fdset);
        iret = select(skfd+1, &fdset, NULL, NULL, &timeout);
        if (iret = 0)
        {
            Notify(__FILE__, __LINE__, PRT_INFO, "recv select timeout");
            close(skfd);
            return 2;
        }
        if (iret < 0)
        {
            Notify(__FILE__, __LINE__, PRT_INFO, "recv select error[%d:%s]", errno, strerror(errno));
            close(skfd);
            return 2;
        }
        if (!FD_ISSET(skfd, &fdset))
        {
            Notify(__FILE__, __LINE__, PRT_INFO, "recv select not set");
        }
        else break;
    }
    *slen = recvfrom(skfd, sbuf, 512, 0, NULL, NULL);
    Notify(__FILE__, __LINE__, PRT_INFO, "recv[%d]", *slen);

    close(skfd);
    return 0;
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
