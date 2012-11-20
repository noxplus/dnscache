/* 查询指定域名的ip地址
 * 支持udp/tcp协议
 * 后续支持：根据返回数据的ttl过滤掉dns污染
 * 迂回：当udp查询，返回多次结果时，全部https测试。
 * 主要是针对https的，所以兼顾测试一下是否支持https。
 * */

#include "inc.h"

char* srv = "8.8.8.8";//默认使用google dns。后续加入配置文件
char* qry = "www.google.com";

char data[1024];

int gq(char* dst, char* src)
{
    int i = 0, l = 0;
    while(*src)
    {
        if (++i == 63) break;
        if (*src == '.')
        {
            dst[l] = i - l - 1;
            l = i;
            src++;
            continue;
        }
        dst[i] = *src;
        src++;
    }
    dst[l] = i - l;
    dst[i + 1] = 0;
    dst[i+1+1] = 0x00;
    dst[i+1+2] = 0x01;//type = 0x0001 netbyte
    dst[i+1+3] = 0x00;
    dst[i+1+4] = 0x01;//class = 0x0001 netbyte
    return i + 6;
}
int addtree(char* name, uint32 addr)
{
    IPv4 ip;
    ip.ipv4 = addr;
    printf("[%s] = [%d.%d.%d.%d]\n", name, ip.ipc[3], ip.ipc[2],ip.ipc[1], ip.ipc[0]);
    return 0;
}

int unpackdns(char * buf)
{
    int i;
    DnsHead *pHead = (DnsHead*)buf;
    printf("ques=%d, ans=%d\n", ntohs(pHead->Quests), ntohs(pHead->Ansers));
    char *cur = buf+sizeof(DnsHead);
    char *str;
    int offset, iQue, iAns;

    Query* query = NULL;
    Answer* answer = NULL;

    iQue = ntohs(pHead->Quests);
    query = (Query*)malloc(sizeof(Query));
    bzero(query, iQue*sizeof(Query));

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

    iAns = ntohs(pHead->Ansers);
    answer = (Answer*)malloc(sizeof(Answer));
    bzero(answer, iAns*sizeof(Answer));

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
            addtree(query[0].name, answer[i].ipadd.ipv4);
            return 1;
        }
        printf("[%s][%x][%x][%lx][%x]\n", answer[i].name, answer[i].type, answer[i].class, answer[i].ttl, answer[i].Addlen);
    }
    return 0;
}

int udpquery(char* sd, int slen)
{ 
    char buf[1024];
    int skfd = -1;
    struct sockaddr_in dsrv;
    struct timeval timeout;
    fd_set fdset;

    bzero(&dsrv, sizeof(dsrv));

    dsrv.sin_family = AF_INET;
    dsrv.sin_port = htons(53);
    dsrv.sin_addr.s_addr = inet_addr(srv);

    if ((skfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        printf("socket error\n");
        return -1;
    }

    printf("will send\n");
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    FD_ZERO(&fdset);
    FD_SET(skfd, &fdset);
    select(skfd+1, NULL, &fdset, NULL, &timeout);
    if (FD_ISSET(skfd, &fdset))
    {
        sendto(skfd, sd, slen, 0, (struct sockaddr*)&dsrv, sizeof(dsrv));
        printf("send ok\n");
    }
    else
    {
        printf("select error\n");
        close(skfd);
        return 1;
    }
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    FD_ZERO(&fdset);
    FD_SET(skfd, &fdset);
    select(skfd+1, &fdset, NULL, NULL, &timeout);
    if (FD_ISSET(skfd, &fdset))
    {
        recvfrom(skfd, buf, 1024, 0, NULL, NULL);
        printf("recv ok\n");
        unpackdns(buf);
    }
    else
    {
        printf("recv select error\n");
        close(skfd);
        return 2;
    }

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
    int ilen = gq(data+sizeof(DnsHead), qry);//

    udpquery(data, ilen+sizeof(DnsHead));
}


//参数：目标域名
int main(int argc, char** argv)
{
    qry = argv[1];
    gendata();
    return 0;
}
