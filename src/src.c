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

static RBRoot Srv_addr;
static RBRoot Srv_domain;
static RBRoot Host_addr;
static RBRoot Host_domain;

//线程函数
//1、解析查询内容。
//2、在缓存中查找。//两棵树上都要找。按需反馈
//3、如果不存在，或者ttl超时，在srv树上找dns服务器，网络查询。
//4、解析查询结果，更新/插入缓存。按需反馈。
void* srvfunc(void* arg)
{
    int iQue, iAns;
    DNSRecode   rec, *search;
    LocalQuery* query = (LocalQuery*)arg;
    QueryRec*   queryrec = NULL;
    AnswerRec*  answerrec = NULL;

    iQue = unpackQuery(query->data, &queryrec);//解析报文，获取查询名称
    if (iQue == 0) return NULL; //暂时只处理一个结果，即iQue == 1
    bzero(&rec, sizeof(rec));
    memcpy(rec.uname.cname, queryrec->name, queryrec->namelen);//复制名称
    free(queryrec);
    queryrec = NULL;

    GenIndex(&rec);
    search = RBTreeSearch(&Host_addr, &rec);//在树上查找是否存在
    if (search != NULL)//找到结果，发送回去
    {
        //packAnswer(search);
        if (search->ttl > time(NULL))//未超时，查询结束
        {
            return NULL;
        }
    }

    //修改流程，udpquery返回字符串。再在这里调用unpackAnswer
    udpquery(query->data, (int*)&(query->dlen));//网络查询，结果反馈客户端
    unpackAnswer(query->data, &answerrec);//结果上树。

    Notify(__FILE__, __LINE__, PRT_INFO, "");

    return NULL;
}

//初始化 & 监听端口 & 收取数据，新建线程处理数据
void startwork(void)
{
    int     skfd, iret;
    socklen_t    ilen;
    fd_set  fdread;
    struct timeval  timeout;
    struct sockaddr_in lsrv;

    LocalQuery  *query;

    if ((skfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        Notify(__FILE__, __LINE__, PRT_INFO, "socket error[%d:%s]", errno, strerror(errno));
        return;
    }
    bzero(&lsrv, sizeof(lsrv));
    lsrv.sin_family = AF_INET;
    lsrv.sin_port = htons(53);
    lsrv.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (bind(skfd, (struct sockaddr*)&lsrv, sizeof(lsrv)) == -1)
    {
        Notify(__FILE__, __LINE__, PRT_INFO, "bind error[%d:%s]", errno, strerror(errno));
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
            Notify(__FILE__, __LINE__, PRT_INFO, "select error[%d:%s]", errno, strerror(errno));
            return;
        }
        if (iret == 0) continue; //timeout
        if (FD_ISSET(skfd, &fdread) == 0) continue;//other

        //这里只负责申请空间。使用完毕之后，线程内释放空间！
        query = (LocalQuery*) malloc (sizeof(LocalQuery));
        bzero(query, sizeof(query));
        ilen = sizeof(query->localadd);
        iret = recvfrom(skfd, query->data, 512, 0, (struct sockaddr*)&(query->localadd), &ilen);

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

void init(void)
{
    Srv_addr.Root = NULL;
    Srv_domain.Root = NULL;
    Host_addr.Root = NULL;
    Host_domain.Root = NULL;
}

int main()
{
    init();
    startwork();

    return 0;
}
