#pragma once
#include "NetData.h"
#include "NFIPacket.h"
#include "NFIServer.h"
#include <map>

typedef std::function<int(const NFIPacket& msg)> RECIEVE_FUNCTOR;
typedef std::function<int(const int nSockIndex, const NF_NET_EVENT nEvent, NFIServer* pNet)> EVENT_FUNCTOR;

class NFServer : public NFIServer
{
public:
    template<typename BaseType>
    NFServer(MsgHead::NF_Head nLength, BaseType* pBaseType, 
        int (BaseType::*HandleRecv)(const NFIPacket&), int (BaseType::*HandleSocketEvent)(const int, const NF_NET_EVENT, NFIServer*), const int nResetCount = 30, const float fResetTime = 30.f);

    ~NFServer();

    bool StartServer(int nPort, short nWorkerNum, unsigned int nMaxConnNum, int nReadTimeout, int nWriteTimeout);
    bool Excute();
    void StopServer();

    bool SendMsg(const NFCPacket& msg, const int nSockIndex, bool bBroadcast);
    bool SendMsg(const char* msg, const uint32_t nLen, const int nSockIndex, bool bBroadcast = false);


protected:
    static void listener_cb(struct evconnlistener* listener, evutil_socket_t fd, struct sockaddr* sa, int socklen, void* user_data);
    static void socket_event_cb(struct bufferevent* buffer_event, short events, void* user_data);
    static void read_cb(struct bufferevent* buffer_event, void* user_data);
    static void write_cb(struct bufferevent* buffer_event, void* user_data);

    //只能在网络线程调用，才对。
    static void CloseConn(Connection* pConn);

    static int ThreadServer(void* user_data);
    static int ThreadWorkers(void* user_data);

    static Connection* GetConectionInWoker(Worker* pWorker,const int nSocket);

public:

private:
    Server mxServer;
    RECIEVE_FUNCTOR mxRecvFunc;
    EVENT_FUNCTOR mxEventFunc;

};

template<typename BaseType>
NFServer::NFServer(MsgHead::NF_Head nLength, BaseType* pBaseType, int (BaseType::*HandleRecv)(const NFIPacket&), int (BaseType::*HandleSocketEvent)(const int, const NF_NET_EVENT, NFIServer*), const int nResetCount /*= 30*/, const float fResetTime /*= 30.f*/)
{
    mxServer.nHeadLen = nLength;
    mxRecvFunc = std::bind(HandleRecv, pBaseType, std::placeholders::_1);
    mxEventFunc = std::bind(HandleSocketEvent, pBaseType, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}
