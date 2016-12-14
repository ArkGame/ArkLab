#pragma once

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

struct Connection;
struct Worker;

enum ENetDefine
{
    eMaxBuffLen     = 4096,
    eMaxPackageType = 65535,
};

struct Connection
{
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
    std::vector<char> xInBuff;
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

struct Worker
{
    Worker()
    {
        pBase = NULL;
        pConnList = NULL;
    }

    inline Connection* GetFreeConn()
    {
        Connection* pConn = NULL;
        if(pConnList->pHead != pConnList->pTail)
        {
            pConn = pConnList->pHead;
            pConnList->pHead = pConnList->pHead->pNext;
        }

        return pConn;
    }

    inline void FreeConn(Connection* pConn)
    {
        pConnList->pTail->pNext = pConn;
        pConnList->pTail = pConn;
    }

    struct event_base* pBase;
    std::thread xThread;
    ConnectionList* pConnList;
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
    std::thread xThread;
    size_t nHeadLen;
};

enum NF_NET_EVENT
{
    NF_NET_EVENT_EOF = 0x10, /**< eof file reached */
    NF_NET_EVENT_ERROR = 0x20, /**< unrecoverable error encountered */
    NF_NET_EVENT_TIMEOUT = 0x40, /**< user-specified timeout reached */
    NF_NET_EVENT_CONNECTED = 0x80, /**< connect operation finished. */
};

