#pragma once

#include <event2/event-config.h>
#include <event2/bufferevent.h>
#include <event2/bufferevent_compat.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>
#include <event2/buffer_compat.h>
#include <event2/thread.h>
#include <thread>
#include <vector>
#include <functional>
#include <memory>
#include "NFCPacket.h"
#include <list>
#include "common/NFQueue.h"
#include <map>
#include "common/NFLockFreeQueue.h"

struct Connection;
struct Worker;
class NFIServer;

enum ENetDefine
{
    eMaxBuffLen     = 4 * 1024,
    eMaxPackageType = 65535,
};

struct Connection
{
public:
    Connection()
    {
        pBuffEvent = NULL;
        nFD = 0;
        nIndex = -1;
        xInBuff.resize(eMaxBuffLen);
        nInBuffLen = 0;
        xOutBuff.resize(eMaxBuffLen);
        nOutBuffLen = 0;
        pOwner = NULL;
        pNext = NULL;
    }

    struct bufferevent* pBuffEvent;
    evutil_socket_t nFD;
    int nIndex;
    std::vector<char>xInBuff;
    short nInBuffLen;
    std::vector<char> xOutBuff;
    short nOutBuffLen;
    Worker* pOwner;
    Connection* pNext;
};

struct ConnectionList
{
    ConnectionList()
    {
        pHead = NULL;
        pTail = NULL;
        pListConn = NULL;
    }

    Connection* pHead;
    Connection* pTail;
    Connection* pListConn;
};

struct EventData
{
    EventData()
    {
        nSockIndex = 0;
        nEvent = 0;
    }

    int         nSockIndex;
    int         nEvent;
};

struct SendData
{
    SendData()
    {
        nSockIndex = 0;
    }

    int         nSockIndex;
    std::string strData;
};

struct ReceiveData
{
    ReceiveData()
    {
        nSockIndex = 0;
        nMsgID = 0;
    }

    int         nSockIndex;
    int         nMsgID;
    std::string strMsg;
};

struct Worker
{
    Worker()
    {
        pBase = NULL;
        pThread = NULL;
        mnHeadLen = 0;
        bExit = false;
    }

    inline Connection* GetFreeConn()
    {
        Connection* pConn = NULL;
        int nIndex = xCanUseConnList.front();
        xCanUseConnList.pop_front();
        if(nIndex < 0 || nIndex >= pConnList.size())
        {
            return NULL;
        }

        pConn = pConnList[nIndex];
        return pConn;
    }

    inline void FreeConn(Connection* pConn)
    {
        xCanUseConnList.push_back(pConn->nIndex);
    }

    struct event_base* pBase;
    std::thread* pThread;
    int mnHeadLen;
    bool bExit;

    std::vector<Connection*> pConnList; // nIndex 《-- 》;
    std::list<int> xCanUseConnList; //  可用的connection nIndex
    std::map<int, int> mmFDIndex; //  fd<--> nIndex; //

    NFLockFreeQueue<ReceiveData> mReceivemsgList;
    NFLockFreeQueue<SendData> mSendmsgList;
    NFLockFreeQueue<EventData> mEventDataList;
    NFLockFreeQueue<int> mAccpetSokcetList;
};

struct Server
{
    Server()
    {
        bStart = false;
        nPort = 0;
        nWorkerNum = 0;
        nConnectNum = 0;
        nCurWorker = 0;
        nReadTimeout = 60; //默认给60s
        nWriteTimeout = 60;//默认给60s
        pListener = NULL;
        pBase = NULL;
        pWorker = NULL;
        nHeadLen = 0;
        pThread = NULL;
        bNeedCloseListen = false;
    }

    bool bStart;
    short nPort;
    short nWorkerNum;
    unsigned int nConnectNum;
    volatile int nCurWorker;
    int nReadTimeout;
    int nWriteTimeout;
    struct evconnlistener* pListener;
    struct event_base* pBase;
    Worker* pWorker;
    std::thread* pThread;
    size_t nHeadLen;
    bool bNeedCloseListen;
};

enum NF_NET_EVENT
{
    NF_NET_EVENT_EOF = 0x10, /**< eof file reached */
    NF_NET_EVENT_ERROR = 0x20, /**< unrecoverable error encountered */
    NF_NET_EVENT_TIMEOUT = 0x40, /**< user-specified timeout reached */
    NF_NET_EVENT_CONNECTED = 0x80, /**< connect operation finished. */
};

