#ifndef _TESTGG_H_
#define _TESTGG_H_

#include <string>
#include <list>
#include "netsock.hpp"


typedef struct
{
    uint32         ConnTimeout; //connetc的超时 默认500ms
    uint32         SSLTimeout;  //发送ssl数据的超时 默认500ms
    uint32         HostIPCnt;   //保留的IP地址个数。默认30
    const char*    BakFile;     //IP地址存储文件。默认test.txt
    const char*    IPBlocks;    //google IP网段。
    uint32         ChkInter;    //检测保存数据间隔。默认600s
    uint32         TestInter;   //测试时间间隔。默认2秒
}ggHostCFG;

//一组记录
class ggRec
{
    private:
        IPv4    ipaddr;     //IP地址
        uint32  timeout;    //超时
        std::string  cert;  //证书 - 暂未使用

    public:
        ggRec(uint32 ip, uint32 time = (uint32)-1)
        {
            ipaddr.ipv4 = ip;
            timeout = time;
        }
        ~ggRec(void){}
        bool operator>(const ggRec&);
        bool operator<(const ggRec&);
        bool operator==(const ggRec&);
        void print(FILE* fw = stdout);  //输出
        void SetIPAddr(uint32);
        void SetTimeout(uint32);
        uint32 GetIPAddr(void);
        uint32 GetTimeout(void);
        void tostr(char*, int);         //数据转字符串
};

typedef std::list<ggRec>            ggList;
typedef std::list<ggRec>::iterator  ggListIter;

class ggTest : public SSLTest
{
    private:
        ggList      m_list;         //IP地址列表
        ggHostCFG   m_cfg;          //测试配置
        IPBlock     m_ipnet;        //IP段
        uint32      m_next_test;    //下一次测试时间
        uint32      m_next_check;   //下一次检测时间

    private:
        void InitCfg(void);         //初始化为默认参数
        void Load2Mem(void);        //从文件读取历史数据
        void Save2File(void);       //保存数据到文件
        void CheckFunc(void);       //检测所有IP的超时
        void TestFunc(void);        //测试IP地址

    public:
        ggTest(void);
        ~ggTest(void);
        void ParseArg(int, char**); //从命令行设置参数
        void InitTest(void);        //初始化测试数据
        void LoopFunc(void);        //根据超时，一个小循环
};

typedef struct _IPVal
{
    IPv4    ipaddr;
    uint32  timeout;
    //char    addon[1024];
}IPVal;

class ggStore
{
    private:
#define DefaultSizeStore        20
#define DefaultSizeHistory      1024
        IPVal    *g_Store;//存储可用IP及超时信息
        IPv4     *g_History;//存储被刷下的IP信息
        uint32   g_SizeStore;
        uint32   g_SizeHistory;

        //以下三个变量会在Insert成功之后自动刷新
        uint32   g_mintime;//最快时间，做阈值
        uint32   g_maxtime;//最慢时间，加速插入
        uint32   g_maxindex;//最慢序号，加速插入

        pthread_mutex_t m_Lock_Store;//对Store的操作都要加锁

        int MoveSort(int);      //对只有一个元素错位进行排序

    public:
        ggStore(void);
        ggStore(int, int);
        ~ggStore(void);

        int Reset(int, int);    //重新设置大小
        void Load2Mem(void);    //从文件读取历史数据
        void Save2File(void);   //保存数据到文件

        uint32 CheckGet(void);  //
        int CheckSet(uint32);   //0x00: 开始
        int Insert(IPVal*);     //
}

#endif
