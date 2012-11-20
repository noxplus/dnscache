/* add by nox @ 2012-11-08
 * 1、hash算法：取name的前16byte，按4个int32异或为一个int32
 * 2、四棵树。Srv_address, Srv_domain, Host_address, Host_domain
 *    address树存放地址，完全匹配。
 *    domain树存放域名后面，循环剥离。
 *    Srv树存放dns服务器地址，决定从哪里查询。
 *    Host树存放域名的地址，配置文件/新查询的地址都扔进去。
 * 3、step1：读取配置文件，生成树。
 *    step2：监听端口
 *    step3：收到查询，分出线程负责。
 *           在Host树中查找是否已经存在。如果存在，返回结果。
 *           如果不存在，或者ttl超时，查找Srv树，决定使用哪个DNS服务器。
 *           在服务器查询，（判断多个返回值及dns污染情况）
 *           回复给查询。结果上树。
 * modify by nox @ 2012.11.20
 * */

#include "inc.h"

void* srvfunc(void* arg)
{
    LocalQuery query = (LocalQuery*)arg;
}

void startwork(void)
{
    int     skfd, iret, len;
    fd_set  fdread;
    struct timeval  timeout;
    struct sockaddr_in lsrv;

    LocalQuery  *query;

    if ((skfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        printf("socket error\n");
        retuen;
    }
    bzero(&lsr, sizeof(lsrv));
    lsrv.sin_family = AF_INET;
    lsrv.sin_port = htons(53);
    lsrv.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (bind(skfd, &lsrv, sizeof(lsrv)) == -1)
    {
        printf("bind error\n");
        retuen;
    }

    while(1)
    {
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        FD_ZERO(&fdread);
        FD_SET(skfd, &fdread);
        iret = select(skfd+1, &rdread, NULL, NULL, &timeout);
        if (iret < 0)
        {
            printf("select error\n");
            return;
        }
        if (iret == 0) continue; //timeout
        if (FD_ISSET(skfd, &fdread) == 0) continue;//other
        query = (LocalQuery*) malloc (sizeof(LocalQuery));
        bzero(&query, sizeof(query));
        iret = recvfrom(skfd, query->data, sizeof(query->data), 0 , &(query->localadd), &len);
        if (iret > 0)
        {
            pthread_t tid;
            query.dlen = iret;
            pthread_create(&tid, NULL, srvfunc, (void*)query);
        }
    }
}

void readcfg(void)
{}

int main()
{}
