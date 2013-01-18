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

#endif
