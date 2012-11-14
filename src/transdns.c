/* 查询指定域名的ip地址
 * 支持udp/tcp协议
 * 后续支持：根据返回数据的ttl过滤掉dns污染
 * 迂回：当udp查询，返回多次结果时，全部https测试。
 * 主要是针对https的，所以兼顾测试一下是否支持https。
 * */

#include <stdlib.h>
#include <stdio.h>

typedef long    int32;
typedef short   int16;
typedef char    int8;
typedef unsigned long   uint32;
typedef unsigned short  uint16;
typedef unsigned char   uint8;
#define PACKED __attribute__((packed))

typedef struct 

char* srv = "8.8.8.8";//默认使用google dns。后续加入配置文件

//参数：目标域名
int main(int argc, char** argv)
{}
