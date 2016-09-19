#pragma once

#include <deque>
#include <set>
#include <algorithm>
#include <memory>
#include <iostream>
#include "boost/noncopyable.hpp"
#include "boost/asio.hpp"
#include "boost/any.hpp"
#include "boost/thread.hpp"
#include <functional>

namespace ArkNet
{

class TCPServer;
class ConnectionManager;
class Connection : boost::noncopyable, std::enable_shared_from_this<Connection>
{
public:
    enum ConnectionState
    {
        NONE            = 0,
        CONNECTED       = 1,
        DISCONNECTED    = 2,
    };

    explicit Connection(boost::asio::io_service& io, TCPServer& server, ConnectionManager* connMgr);
    ~Connection();

    void Start();
    void Stop();

    boost::asio::ip::tcp::socket& GetSocket()
    {
        return mxSocket;
    }

    TCPServer& GetServer()
    {
        return mxServer;
    }

    typedef std::function<void(const boost::system::error_code&)> WriteCallback;
    void WriteMsg(const std::string& msg, const WriteCallback& cb = WriteCallback());

protected:
    void Close();
    void HandleReadHeader(const boost::system::error_code& error, std::size_t bytes_transferred);
    void HandleReadBody(const boost::system::error_code& error, std::size_t bytes_transferred);
    void HandleWrite(const boost::system::error_code& error);

    void DoWrite(std::string msg, WriteCallback cb);

private:
    boost::asio::io_service& mxIOS;
    TCPServer& mxServer;
    boost::asio::ip::tcp::socket mxSocket;
    ConnectionManager* mxConnectionMgr;
    bool mbConnected;
    boost::asio::streambuf mxRequestBuffer;
    boost::asio::streambuf mxResponseBuffer;

    struct InternelMsg
    {
        std::string msg;
        WriteCallback handler;
    };

    typedef std::deque<InternelMsg> WriteQueue;
    WriteQueue mxWriteQueue;
    std::map<std::string, boost::any> mxConnectPropertyMap;
};


typedef std::shared_ptr<Connection> ConnectionPtr;
class ConnectionManager : public boost::noncopyable
{
public:
    void Start(ConnectionPtr connect)
    {
        boost::mutex::scoped_lock lock(mxMutex);
        mxConnections.insert(connect);
        connect->Start();
    }

    void Stop(ConnectionPtr connect)
    {
        boost::mutex::scoped_lock lock(mxMutex);
        if (mxConnections.find(connect) != mxConnections.end())
        {
            mxConnections.erase(connect);
        }

        connect->Stop();
    }

    void StopAll()
    {
        boost::mutex::scoped_lock lock(mxMutex);
        std::for_each(mxConnections.begin(), mxConnections.end(), std::bind(&Connection::Stop, std::placeholders::_1));
        mxConnections.clear();
    }

private:
    boost::mutex mxMutex;
    std::set<ConnectionPtr> mxConnections;
};

}