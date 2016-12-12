#include "NFServer.h"

#ifdef _MSC_VER
#include <WS2tcpip.h>
#include <winsock2.h>
#else
#include <linux/tcp.h>
#endif

NFServer::~NFServer()
{
#ifdef _MSC_VER
    WSACleanup();
#endif
}

bool NFServer::StartServer(int nPort, short nWorkerNum, unsigned int nMaxConnNum, int nReadTimeout, int nWriteTimeout)
{
    mxServer.bStart = false;
    mxServer.nCurWorker = 0;
    mxServer.nPort = nPort;
    mxServer.nWorkerNum = nWorkerNum;
    mxServer.nConnectNum = nMaxConnNum;
    mxServer.nReadTimeout = nReadTimeout;
    mxServer.nWriteTimeout = nWriteTimeout;

#ifdef _MSC_VER
    WSADATA wsa_data;
    WSAStartup(0x0201, &wsa_data);
#endif

#ifdef _MSC_VER
    evthread_use_windows_threads();
#endif

    mxServer.pBase = event_base_new();
    if(NULL == mxServer.pBase)
    {
        return false;
    }

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(mxServer.nPort);

    mxServer.pListener = evconnlistener_new_bind(mxServer.pBase, listener_cb, (void*)&mxServer, LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, -1, (struct sockaddr*)&sin, sizeof(sin));
    if(NULL == mxServer.pListener)
    {
        std::cout << "Cannot create listener, please check port has been used." << std::endl;
        return false;
    }

    //TODO:一些特殊函数处理

    mxServer.pWorker = new Worker[mxServer.nWorkerNum];
    for(int i = 0; i < mxServer.nWorkerNum; ++i)
    {
        mxServer.pWorker[i].pBase = event_base_new();
        if(NULL == mxServer.pWorker[i].pBase)
        {
            delete[]mxServer.pWorker;
            return false;
        }

        mxServer.pWorker[i].pConnList = new ConnectionList();
        if(NULL == mxServer.pWorker[i].pConnList)
        {
            delete[]mxServer.pWorker;
            return false;
        }

        mxServer.pWorker[i].pConnList->pListConn = new Connection[mxServer.nConnectNum + 1];
        mxServer.pWorker[i].pConnList->pHead = &mxServer.pWorker[i].pConnList->pListConn[0];
        mxServer.pWorker[i].pConnList->pTail = &mxServer.pWorker[i].pConnList->pListConn[mxServer.nConnectNum];
        for(int j = 0; j < mxServer.nConnectNum; ++j)
        {
            mxServer.pWorker[i].pConnList->pListConn[j].nIndex = j;
            mxServer.pWorker[i].pConnList->pListConn[j].pNext = mxServer.pWorker[i].pConnList->pListConn[j + 1];
        }
        mxServer.pWorker[i].pConnList->pListConn[mxServer.nConnectNum].nIndex = mxServer.nConnectNum;
        mxServer.pWorker[i].pConnList->pListConn[mxServer.nConnectNum].pNext = NULL;

        Connection* pConn = mxServer.pWorker[i].pConnList->pHead;
        while(NULL != pConn)
        {
            pConn->pBuffEvent = bufferevent_socket_new(mxServer.pWorker[i].pBase, -1, BEV_OPT_CLOSE_ON_FREE);
            if(NULL == pConn->pBuffEvent)
            {
                //TODO:delete former memory
                return false;
            }

            //set event callbacks
            bufferevent_setcb(pConn->pBuffEvent, read_cb, NULL, socket_event_cb, pConn);
            bufferevent_setwatermark(pConn->pBuffEvent, EV_READ, 0, eMaxBuffLen);
            bufferevent_enable(pConn->pBuffEvent, EV_READ | EV_WRITE);
            struct timeval xDelayReadTimeout;
            xDelayReadTimeout.tv_sec = mxServer.nReadTimeout;
            xDelayReadTimeout.tv_usec = 0;

            struct timeval xDelayWriteTimeout;
            xDelayWriteTimeout.tv_sec = mxServer.nWriteTimeout;
            xDelayWriteTimeout.tv_usec = 0;

            bufferevent_set_timeouts(pConn->pBuffEvent, &xDelayReadTimeout, xDelayWriteTimeout);
            pConn->pOwner = &mxServer.pWorker[i];
            pConn = pConn->pNext;
        }

        mxServer.pWorker[i].xThread(&NFServer::ThreadWorkers, &mxServer.pWorker[i]);
        mxServer.pWorker[i].xThread.detach();
    }

    mxServer.xThread(&NFServer::ThreadServer, &mxServer);
    mxServer.xThread.detach();

    mxServer.bStart = true;
}

void NFServer::StopServer()
{
    if(!mxServer.bStart)
    {
        return;
    }

    struct timeval xDelay = { 2, 0 };
    event_base_loopexit(mxServer.pBase, &xDelay);
    if(NULL != mxServer.pWorker)
    {
        for(int i = 0; i < mxServer.nWorkerNum; ++i)
        {
            event_base_loopexit(mxServer.pWorker[i].pBase, &xDelay);
        }

        for(int i = 0; i < mxServer.nWorkerNum; ++i)
        {
            if(NULL != mxServer.pWorker[i].pConnList)
            {
                delete []mxServer.pWorker[i].pConnList->pListConn;
                delete mxServer.pWorker[i].pConnList;
                mxServer.pWorker[i].pConnList = NULL;
            }

            event_base_free(mxServer.pWorker[i].pBase);
        }

        delete []mxServer.pWorker;
        mxServer.pWorker = NULL;
    }

    evconnlistener_free(mxServer.pListener);
    event_base_free(mxServer.pBase);
    mxServer.bStart = false;
}

void NFServer::listener_cb(struct evconnlistener* listener, evutil_socket_t fd, struct sockaddr* sa, int socklen, void* user_data)
{
    Server* pServer = (Server*)user_data;
    int nCurrent = (pServer->nCurWorker++) % pServer->nWorkerNum;
    Worker& xWorker = pServer->pWorker[nCurrent];
    Connection* pConn = xWorker.GetFreeConn();
    if(NULL == pConn)
    {
        return;
    }

    pConn->nFD = fd;
    evutil_make_socket_nonblocking(pConn->nFD);
    bufferevent_enable(pConn->pBuffEvent, EV_READ | EV_WRITE);
    socket_event_cb(pConn->pBuffEvent, eConnected, pConn);
}

void NFServer::socket_event_cb(struct bufferevent* buffer_event, void* user_data)
{

}

void NFServer::read_cb(struct bufferevent* buffer_event, void* user_data)
{
    struct evbuffer* input_buffer = bufferevent_get_input(buffer_event);
    size_t input_len = evbuffer_get_length(input_buffer);
    if(input_len <= 0)
    {
        return;
    }

    Connection* pConn = (Connection*)user_data;
    while(evbuffer_get_length(input_buffer))
    {
        pConn->nInBuffLen += evbuffer_remove(input_buffer, pConn->xInBuff[0], eMaxBuffLen - pConn->nInBuffLen);
    }

    while(true)
    {
        if(pConn->nInBuffLen < IMsgHead::NF_HEAD_LENGTH)
        {
        }
    }
}

void NFServer::write_cb(struct bufferevent* buffer_event, void* user_data)
{
    //do something
}

void NFServer::CloseConn(Connection* pConn)
{
    pConn->nInBuffLen = 0;
    bufferevent_disable(pConn->pBuffEvent, EV_READ | EV_WRITE);
    evutil_closesocket(pConn->nFD);
    pConn->pOwner->FreeConn(pConn);
}

int NFServer::ThreadServer(void* user_data)
{
    Server* pServer = reinterpret_cast<Server*>(user_data);
    if(NULL == pServer)
    {
        return -1;
    }

    event_base_loop(pServer->pBase, EVLOOP_ONCE | EVLOOP_NONBLOCK);
    return std::this_thread::get_id();
}

int NFServer::ThreadWorkers(void* user_data)
{
    Worker* pWorker = reinterpret_cast<Worker*>(user_data);
    if(NULL == pWorker)
    {
        return -1;
    }

    event_base_loop(pWorker->pBase, EVLOOP_ONCE | EVLOOP_NONBLOCK);
    return std::this_thread::get_id();
}