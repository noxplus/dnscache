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
int udpquery(char* sd, int slen)
{ 
    int skfd = -1;
    struct sockaddr_in dsrv;

    memset(&dsrv, 0, sizeof(dsrv));

    dsrv.sin_family = AF_INET;
    dsrv.sin_port = htons(53);
    dsrv.sin_addr.s_addr = inet_addr(srv);

    if ((skfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        printf("socket error\n");
        return -1;
    }

    sendto(skfd, sd, slen, 0, (struct sockaddr*)&dsrv, sizeof(dsrv));

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
    gendata();
    return 0;
}
