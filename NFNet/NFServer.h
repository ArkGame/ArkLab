#pragma once
#include "NetData.h"

class NFServer
{
public:
    template<typename BaseType>
    NFServer(MsgHead::NF_Head nLength, BaseType* pBaseType, int (BaseType::*HandleRecv)(const NFIPacket&), int (BaseType::HandleSocketEvent)(), const int nResetCount = 30, const float fResetTime = 30.f);

    ~NFServer();

    bool StartServer(int nPort, short nWorkerNum, unsigned int nMaxConnNum, int nReadTimeout, int nWriteTimeout);
    void StopServer();

protected:
    static void listener_cb(struct evconnlistener* listener, evutil_socket_t fd, struct sockaddr* sa, int socklen, void* user_data);
    static void socket_event_cb(struct bufferevent* buffer_event, void* user_data);
    static void read_cb(struct bufferevent* buffer_event, void* user_data);
    static void write_cb(struct bufferevent* buffer_event, void* user_data);
    void CloseConn(Connection* pConn);

    static int ThreadServer(void* user_data);
    static int ThreadWorkers(void* user_data);

private:
    Server mxServer;
};

template<typename BaseType>
NFServer::NFServer(MsgHead::NF_Head nLength, BaseType* pBaseType, int (BaseType::*HandleRecv)(const NFIPacket&), int (BaseType::HandleSocketEvent)(), const int nResetCount /*= 30*/, const float fResetTime /*= 30.f*/)
{
    mxServer.nHeadLen = nLength;

}
