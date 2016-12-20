#include "NFServer.h"
#include "NFCPacket.h"

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
        mxServer.pWorker[i].mnHeadLen = mxServer.nHeadLen;
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
            mxServer.pWorker[i].pConnList->pListConn[j].pNext = &mxServer.pWorker[i].pConnList->pListConn[j + 1];
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

            //bufferevent_set_timeouts(pConn->pBuffEvent, &xDelayReadTimeout, &xDelayWriteTimeout);
            pConn->pOwner = &mxServer.pWorker[i];
            pConn = pConn->pNext;
        }

        mxServer.pWorker[i].pThread = new std::thread(&NFServer::ThreadWorkers, &mxServer.pWorker[i]);
        mxServer.pWorker[i].pThread->detach();
    }

    mxServer.pThread = new std::thread(&NFServer::ThreadServer, &mxServer);
    mxServer.pThread->detach();

    mxServer.bStart = true;

    return true;
}

bool NFServer::Execute()
{
    for(int em = 0; em < mxServer.nWorkerNum; em++)
    {
        ReceiveData xReceiveData;
        NFCPacket xPacket((MsgHead::NF_Head)mxServer.nHeadLen);
        bool bRet = mxServer.pWorker[em].mReceivemsgList.Pop(xReceiveData);
        if(bRet && mxRecvFunc)
        {
            if(xPacket.DeCode(xReceiveData.strMsg.c_str(), xReceiveData.strMsg.size()))
            {
                xPacket.SetFd(xReceiveData.nSockIndex);
                mxRecvFunc(xPacket);
            }
        }
    }

    for(int em = 0; em < mxServer.nWorkerNum; em++)
    {
        EventData xData;
        bool bRet = mxServer.pWorker[em].mEventDataList.Pop(xData);
        if(bRet && mxEventFunc)
        {
            if(xData.nEvent == NF_NET_EVENT_CONNECTED)
            {
                mmFDWorkerIndex[xData.nSockIndex] = em;
            }
            else
            {
                mmFDWorkerIndex[xData.nSockIndex] = -1;
            }

            mxEventFunc(xData.nSockIndex, (NF_NET_EVENT)xData.nEvent, this);
        }
    }

    return true;
}

void NFServer::StopServer()
{
    if(!mxServer.bStart)
    {
        return;
    }

    for(int i = 0; i < mxServer.nWorkerNum; ++i)
    {
        mxServer.pWorker[i].bExit = false;
    }

    for(int i = 0; i < mxServer.nWorkerNum; ++i)
    {
        if(mxServer.pWorker[i].pThread)
        {
            mxServer.pWorker[i].pThread->join();
        }
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

bool NFServer::SendMsg(const NFCPacket& msg, const int nSockIndex, bool bBroadcast)
{
    return SendMsg(msg.GetPacketData(), msg.GetPacketLen(), nSockIndex, bBroadcast);
}

bool NFServer::SendMsg(const char* msg, const uint32_t nLen, const int nSockIndex, bool bBroadcast /*= false*/)
{
    std::map<int, int>::iterator iter = mmFDWorkerIndex.find(nSockIndex);
    if(iter == mmFDWorkerIndex.end())
    {
        return false;
    }

    int nIndex = iter->second;
    if(nIndex < 0 || nIndex >= mxServer.nWorkerNum)
    {
        return false;
    }

    Worker* pWork = &mxServer.pWorker[nIndex];
    if(NULL == pWork)
    {
        return false;
    }

    SendData xData;
    xData.nSockIndex = nSockIndex;
    xData.strData = std::string(msg, nLen);
    pWork->mSendmsgList.Push(xData);

    return true;
}

void NFServer::listener_cb(struct evconnlistener* listener, evutil_socket_t fd, struct sockaddr* sa, int socklen, void* user_data)
{
    Server* pServer = (Server*)user_data;
    int nCurrent = (pServer->nCurWorker++) % pServer->nWorkerNum;
    Worker& xWorker = pServer->pWorker[nCurrent];
    xWorker.mAccpetSokcetList.Push(fd);
}

void NFServer::socket_event_cb(struct bufferevent* buffer_event, short events, void* user_data)
{
    Connection* pConection = (Connection*)user_data;
    if(NULL == pConection)
    {
        return;
    }

    EventData xdata;
    xdata.nEvent = events;
    xdata.nSockIndex = pConection->nFD;

    if(NULL != pConection->pOwner)
    {
        pConection->pOwner->mEventDataList.Push(xdata);

        if(!(events & BEV_EVENT_CONNECTED))
        {
            CloseConn(pConection);
        }
    }
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
    if(!pConn)
    {
        return;
    }

    int nMoveLen = input_len;
    if(input_len > eMaxBuffLen)
    {
        nMoveLen = eMaxBuffLen;
    }

    evbuffer_remove(input_buffer, &pConn->xInBuff[0], nMoveLen);
    pConn->nInBuffLen = nMoveLen;

    if(!pConn->pOwner)
    {
        return;
    }

    NFCPacket packet((MsgHead::NF_Head)(pConn->pOwner->mnHeadLen));
    while(true)
    {
        if(pConn->nInBuffLen < pConn->pOwner->mnHeadLen)
        {
            return;
        }

        packet.Reset((MsgHead::NF_Head)(pConn->pOwner->mnHeadLen));
        const int nUsedLen = packet.DeCode(&pConn->xInBuff[0], pConn->nInBuffLen);
        if(nUsedLen > (eMaxBuffLen - pConn->pOwner->mnHeadLen) || nUsedLen <= 0)
        {
            //TODO worker出错了，怎么关掉conn
            //pConn->pOwner>CloseConn(pConn);
            return;
        }

        packet.SetFd(pConn->nFD);

        //TODO:packet处理

        pConn->nInBuffLen -= packet.GetPacketLen();
        if(pConn->nInBuffLen == 0)
        {
            break;
        }
        else
        {
            assert(pConn->nInBuffLen > 0);
            memmove(&pConn->xInBuff[0], (&pConn->xInBuff[0]) + packet.GetPacketLen(), pConn->nInBuffLen);

            ReceiveData xReceiveData;
            xReceiveData.nMsgID = packet.GetMsgID();
            xReceiveData.nSockIndex = packet.GetFd();
            xReceiveData.strMsg = std::string(packet.GetPacketData(), packet.GetPacketLen());
            pConn->pOwner->mReceivemsgList.Push(xReceiveData);
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
    pConn->nFD = 0;
    pConn->nIndex = -1;
    pConn->pOwner->FreeConn(pConn);
}

int NFServer::ThreadServer(void* user_data)
{
    Server* pServer = reinterpret_cast<Server*>(user_data);
    if(NULL == pServer)
    {
        return -1;
    }

    while(!pServer->bNeedCloseListen)
    {
        event_base_loop(pServer->pBase, EVLOOP_ONCE | EVLOOP_NONBLOCK);

        NFSLEEP(1);
    }

    return 0;//std::this_thread::get_id();
}

int NFServer::ThreadWorkers(void* user_data)
{
    Worker* pWorker = reinterpret_cast<Worker*>(user_data);
    if(NULL == pWorker)
    {
        return -1;
    }

    while(!pWorker->bExit)
    {
        const int nSoketSize = pWorker->mAccpetSokcetList.Count();
        for(int i = 0; i < nSoketSize; i++)
        {
            Connection* pConn = pWorker->GetFreeConn();//new Connection();
            if(NULL == pConn)
            {
                continue;
            }

            int nfd = 0;
            if(!pWorker->mAccpetSokcetList.Pop(nfd))
            {
                continue;
            }

            //pConn->pBuffEvent = bufferevent_socket_new(pWorker->pBase, nfd, BEV_OPT_CLOSE_ON_FREE);
            if(NULL == pConn->pBuffEvent)
            {
                //TODO:delete former memory
                return false;
            }

            ////set event callbacks
            //bufferevent_setcb(pConn->pBuffEvent, read_cb, NULL, socket_event_cb, pConn);
            //bufferevent_setwatermark(pConn->pBuffEvent, EV_READ, 0, eMaxBuffLen);
            //bufferevent_enable(pConn->pBuffEvent, EV_READ | EV_WRITE);

            //TODO 这里是监听线程，在改数据，改fd，线程安全等等。。
            pConn->nFD = nfd;
            bufferevent_setfd(pConn->pBuffEvent, nfd);
            evutil_make_socket_nonblocking(pConn->nFD);
            bufferevent_enable(pConn->pBuffEvent, EV_READ | EV_WRITE);

            socket_event_cb(pConn->pBuffEvent, NF_NET_EVENT_CONNECTED, pConn);
        }

        event_base_loop(pWorker->pBase, EVLOOP_ONCE | EVLOOP_NONBLOCK);

        const int nSize = pWorker->mSendmsgList.Count();
        for(int i = 0; i < nSize; i++)
        {
            SendData xData;
            if(pWorker->mSendmsgList.Pop(xData))
            {
                Connection* pConect = GetConectionInWoker(pWorker, xData.nSockIndex);
                if(pConect)
                {
                    pConect->pBuffEvent;
                    bufferevent_write(pConect->pBuffEvent, xData.strData.c_str(), xData.strData.size());
                }
            }
        }

        NFSLEEP(1);
    }

    return 0;//std::this_thread::get_id();
}

Connection* NFServer::GetConectionInWoker(Worker* pWorker, const int nSocket)
{
    if(NULL == pWorker)
    {
        return NULL;
    }

    for(Connection* pData = pWorker->pConnList->pHead; pData != pWorker->pConnList->pTail; pData = pData->pNext)
    {
        if(NULL != pData && nSocket == pData->nFD)
        {
            return pData;
        }
    }

    return NULL;
}
