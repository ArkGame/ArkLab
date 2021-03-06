
#include <thread>
#include <string>
//#include <processthreadsapi.h>
#include "NFComm/NFPluginModule/NFPlatform.h"
#include "NFComm/NFCore/NFTimer.h"
#include "NFNet/NFServer.h"
#include "NFNet/NFIPacket.h"
#ifdef NF_DEBUG_MODE
#pragma comment(lib,"NFNet_d.lib")
#pragma comment(lib,"NFCore_d.lib")

#else
#pragma comment(lib,"NFNet.lib")
#pragma comment(lib,"NFCore.lib")
#endif

#pragma comment(lib,"ws2_32.lib")
#pragma  comment(lib,"libevent.lib")
#pragma  comment(lib,"libevent_core.lib")

class TestServerClass
{
public:
    TestServerClass()
    {

        //pNet = new NFCMulNet(this, &TestServerClass::ReciveHandler, &TestServerClass::EventHandler);
        //pNet->Initialization(10000, 8088, 2, 4);
        //nSendMsgCount = 0;
        //nReciveMsgCount = 0;
        //nStartTime = NFTime::GetNowTime();
        //nLastTime = nStartTime;
        //nLastSendCount = 0;
        //nLasterReciveCount = 0;

        pNet = new NFServer(IMsgHead::NF_MIN_HEAD_LENGTH, this, &TestServerClass::ReciveHandler, &TestServerClass::EventHandler);
        pNet->StartServer(8088, 4, 8000, 300, 300);
    }

    //void ReciveHandler(const int nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen, const NFGUID& xClientID)
    //{
    //    std::string str;
    //    str.assign(msg, nLen);

    //    nReciveMsgCount++;
    //    nSendMsgCount++;
    //    pNet->SendMsgWithOutHead(nMsgID, msg, nLen, nSockIndex);
    //    //std::cout << " nSendMsgCount: " << nSendMsgCount << "nReciveMsgCount" << nReciveMsgCount << " fd: " << nSockIndex << " msg_id: " << nMsgID /*<<  " data: " << str*/ << " thread_id: " << GetCurrentThreadId() << std::endl;
    //}
    int ReciveHandler(const NFIPacket& msg)
    {
        //std::string str;
        //str.assign(msg, nLen);

        //nReciveMsgCount++;
        //nSendMsgCount++;
        //pNet->SendMsgWithOutHead(nMsgID, msg, nLen, nSockIndex);
        //std::cout << " nSendMsgCount: " << nSendMsgCount << "nReciveMsgCount" << nReciveMsgCount << " fd: " << nSockIndex << " msg_id: " << nMsgID /*<<  " data: " << str*/ << " thread_id: " << GetCurrentThreadId() << std::endl;

        std::string strMsg = "11111111111111111111111111111111111111111111111111112222222222222222222222222222222222222222222222222222222222222222223333333333333333333333444";

        pNet->SendMsgWithOutHead(1, strMsg.c_str(), strMsg.size(), msg.GetFd(), false);
        return 1;
    }

    //void EventHandler(const int nSockIndex, const NF_NET_EVENT e, const NFGUID& xClientID, const int nServerID)
    //{
    //    std::cout << " fd: " << nSockIndex << " event_id: " << e << " thread_id: " << std::this_thread::get_id() << std::endl;
    //}

    int EventHandler(const int nSockIndex, const NF_NET_EVENT nEvent, NFIServer* pNet)
    {
        std::cout << " fd: " << nSockIndex << " event_id: " << nEvent << " thread_id: " << std::this_thread::get_id() << std::endl;
        return 1;
    }

    void Execute()
    {
        pNet->Execute();

        int nNowTime = NFTime::GetNowTime();
        int nSpanTime = nNowTime - nLastTime;
        int nAllSpanTime = nNowTime - nStartTime;
        if(nSpanTime > 5 && nAllSpanTime > 0)
        {
            nLastTime = nNowTime;

            const int nLastPerSend = (nSendMsgCount - nLastSendCount) / nSpanTime;
            const int nLastPerReceive = (nReciveMsgCount - nLasterReciveCount) / nSpanTime;


            const int nToltalPerSend = nSendMsgCount / nAllSpanTime;
            const int nToltalPerReceive = nReciveMsgCount / nAllSpanTime;

            nLastSendCount = nSendMsgCount;
            nLasterReciveCount = nReciveMsgCount;

            std::cout << " All Send: [" << nSendMsgCount << "] All Receive: [" << nReciveMsgCount << "] All Per Send per second : [" << nToltalPerSend << "] All Per Receive per second : [" << nToltalPerReceive <<
                      "] Last Second Per Send :[" << nLastPerSend << "] Last Second Per Received [" << nLastPerReceive /*<<  " data: " << str*/ << "] thread_id: " << GetCurrentThreadId() << std::endl;
        }
    }

protected:
    NFServer* pNet;
    int nSendMsgCount;
    int nReciveMsgCount;
    int nStartTime;
    int nLastTime;

    int nLasterReciveCount;
    int nLastSendCount;
};

int main(int argc, char** argv)
{
    TestServerClass x;

    while(1)
    {
        x.Execute();

        NFSLEEP(1);
    }

    return 0;
}
