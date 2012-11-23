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

//线程函数
//1、解析查询内容。
//2、在缓存中查找。//两棵树上都要找。按需反馈
//3、如果不存在，或者ttl超时，在srv树上找dns服务器，网络查询。
//4、解析查询结果，更新/插入缓存。按需反馈。
void* srvfunc(void* arg)
{
    int iQue;
    DNSRecode rec, *sch;
    LocalQuery* query = (LocalQuery*)arg;

    iQue = unpackQuery(query->data, querys);
    if (iQue == 0) return NULL; //暂时只处理一个结果
    bzero(&rec, sizeof(rec));
    memcpy(rec.uname, query->data, query->dlen);
    GenIndex(&rec);
    sch = RBTreeSearch(host_add, rec);
    if (sch != NULL)//找到结果，发送回去
    {
        packAnswer(sch);
        if (sch->ttl > time(NULL))//未超时，查询结束
        {
            return NULL;
        }
    }
    udpquery();

    return NULL;
}

//初始化 & 监听端口 & 收取数据，新建线程处理数据
void startwork(void)
{
    int     skfd, iret;
    fd_set  fdread;
    struct timeval  timeout;
    struct sockaddr_in lsrv;
    socklen_t   len;

    LocalQuery  *query;

    if ((skfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        printf("socket error\n");
        return;
    }
    bzero(&lsrv, sizeof(lsrv));
    lsrv.sin_family = AF_INET;
    lsrv.sin_port = htons(53);
    lsrv.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (bind(skfd, (struct sockaddr*)&lsrv, sizeof(lsrv)) == -1)
    {
        printf("bind error\n");
        return;
    }

    while(1)
    {
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        FD_ZERO(&fdread);
        FD_SET(skfd, &fdread);
        iret = select(skfd+1, &fdread, NULL, NULL, &timeout);
        if (iret < 0)
        {
            printf("select error\n");
            return;
        }
        if (iret == 0) continue; //timeout
        if (FD_ISSET(skfd, &fdread) == 0) continue;//other

        //这里只负责申请空间。使用完毕之后，线程内释放空间！
        query = (LocalQuery*) malloc (sizeof(LocalQuery));
        bzero(&query, sizeof(query));
        iret = recvfrom(skfd, query->data, sizeof(query->data), 0 , (struct sockaddr*)&(query->localadd), &len);
        if (iret > 0)
        {
            pthread_t tid;
            query->dlen = iret;
            pthread_create(&tid, NULL, srvfunc, (void*)query);
        }
    }
}

//读取配置文件，生成全局量初值。
void readcfg(void)
{}

int main()
{}
