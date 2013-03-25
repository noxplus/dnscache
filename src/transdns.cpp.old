#include "util.hpp"
#include "transdns.hpp"


void threadfunc(int sock)
{
    TransDNS    cli(sock);
    cli.UDPRecv();
    GetTF(true);// Õ∑≈
    cli.Decode();
    if (cli.LocalHost() < ERR_no && cli.GetTimeout() == false)
    {
        return cli.Answer();
    }

    cli.NetQuery();
    cli.Answer();
}

int main()
{
    NetUDP udpsrv;

    udpsrv.UDPBind(53);

    while(true)
    {
        while (GetTF(false) == false);
        if (udpsrv.WaitNew() > ERR_no) continue;
        if (pthread_ != OK) GetTF(true); // Õ∑≈
    }
}
