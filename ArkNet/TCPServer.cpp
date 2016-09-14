#include "TCPServer.h"

namespace ArkNet
{

TCPServer::TCPServer(ServicePool& ios, std::string address, unsigned short port)
    : mxServicePool(ios)
    , mxIOS(ios.GetService())
    , mxAcceptor(mxIOS, boost::asio::ip::tcp::v4(), port)
{
    //session* new_session = new session(io_service_);
    //acceptor_.async_accept(new_session->socket(),
    //    boost::bind(&server::handle_accept, this, new_session,
    //        boost::asio::placeholders::error));
}

TCPServer::~TCPServer()
{

}

void TCPServer::Start()
{
    mxConnection = std::make_shared<Connection>(std::ref(mxServicePool.GetService()), boost::ref(*this), &mxConnectionManager);
    mxAcceptor.async_accept(mxConnection->GetSocket(), boost::bind(&TCPServer::HandleAccept, this, boost::asio::placeholders::error));
}

void TCPServer::Stop()
{
    //boost::system::error_code error;
    mxAcceptor.close();
    mxConnectionManager.StopAll();
}

bool TCPServer::AddConnectionProcessCallback(const std::string& name, ConnectionCallback cb)
{
    boost::shared_lock<boost::shared_mutex> lock(mxConnectionCBLock);
    if (mxConnectionCallbacks.find(name) == mxConnectionCallbacks.end())
    {
        return false;
    }

    mxConnectionCallbacks[name] = cb;
    return true;
}

bool TCPServer::AddMsgProcessCallback(const int nMsgID, MsgCallback cb)
{
    boost::shared_lock<boost::shared_mutex> lock(mxMsgCBLock);
    if (mxMsgCallbacks.find(nMsgID) != mxMsgCallbacks.end())
    {
        return false;
    }

    mxMsgCallbacks[nMsgID] = cb;
    return true;
}

bool TCPServer::ProcessMsg(const int nMsgID, const std::string& strMsg, ConnectionPtr conn)
{
    boost::shared_lock<boost::shared_mutex> lock(mxMsgCBLock);
    MsgCallbackMap::iterator iter = mxMsgCallbacks.find(nMsgID);
    if (iter == mxMsgCallbacks.end())
    {
        return false;
    }

    return iter->second(nMsgID, strMsg, std::ref(conn));
}

bool TCPServer::DoConnectionEvent(const int nEvent, ConnectionPtr connection)
{
    boost::shared_lock<boost::shared_mutex> lock(mxConnectionCBLock);
    for (const auto& iter : mxConnectionCallbacks)
    {
        iter.second(nEvent, connection);
    }
}

void TCPServer::HandleAccept(const boost::system::error_code& error)
{
    if (error || !mxAcceptor.is_open())
    {
        //TOOD:log
        return;
    }

    mxConnectionManager.Start(mxConnection);
    Start();
}

}