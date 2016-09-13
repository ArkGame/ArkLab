#pragma once

#include <unordered_map>

#include "boost/noncopyable.hpp"
#include "Connection.h"
#include "ServicePool.h"
#include "boost/thread.hpp"

namespace ArkNet
{
    typedef std::function<bool(const int, const std::string&, ConnectionPtr&)> MsgCallback;
    typedef std::unordered_map<const int, MsgCallback> MsgCallbackMap;

    typedef std::function<void(ConnectionPtr, ConnectionManager&, int)> ConnectionCallback;
    typedef std::unordered_map<std::string, ConnectionCallback> ConnectionCallbackMap;

    class TCPServer : public boost::noncopyable
    {
        friend class Connection;

    public:
        explicit TCPServer(ServicePool& ios, std::string address, unsigned short port);
        ~TCPServer();

        void Start();
        void Stop();

        bool AddConnectionProcessCallback(const std::string& name, ConnectionCallback cb);
        bool AddMsgProcessCallback(const int nMsgID, ConnectionCallback cb);

        bool ProcessMsg(const int nMsgID, const std::string& strMsg, ConnectionPtr conn);
        bool DoConnectionEvent(int nEvent, ConnectionPtr connection);

    protected:
        void HandleAccept(const boost::system::error_code& error);

    private:
        ServicePool& mxServicePool;
        boost::asio::io_service& mxIOS;
        boost::asio::ip::tcp::acceptor mxAcceptor;
        ConnectionPtr mxConnection;
        ConnectionManager mxConnectionManager;
        boost::shared_mutex mxConnectionLock;
        boost::shared_mutex mxMsgCBLock;
        MsgCallbackMap mxMsgCallbacks;
        ConnectionCallbackMap mxConnectionCallbacks;
    };

}