#include "TCPServer.h"

namespace ArkNet
{

TCPServer::TCPServer(ServicePool& ios, std::string address, unsigned short port)
    : mxServicePool(ios)
    , mxIOS(ios.GetService())
    , mxAcceptor(mxIOS)
{
    boost::system::error_code error;

    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port);
    mxAcceptor.open(endpoint.protocol());
    mxAcceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    mxAcceptor.bind(endpoint, error);
    if (error)
    {
        std::cout << "Server bind failed: " << error.message() << std::endl;
        return;
    }

    mxAcceptor.listen(boost::asio::socket_base::max_connections, error);
    if (error)
    {
        std::cout << "Server listen failed: " << error.message() << std::endl;
        return;
    }
}

TCPServer::~TCPServer()
{

}

void TCPServer::Start()
{
    mxConnection = std::make_shared<Connection>(std::ref(mxServicePool.GetService()), std::ref(*this), &mxConnectionManager);
    mxAcceptor.async_accept(mxConnection->GetSocket(), boost::bind(&TCPServer::HandleAccept, this, boost::asio::placeholders::error));
}

void TCPServer::Stop()
{
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
    const auto& iter = mxMsgCallbacks.find(nMsgID);
    if (iter == mxMsgCallbacks.end())
    {
        return false;
    }

    return iter->second(nMsgID, strMsg, std::ref(conn));
}

void TCPServer::DoConnectionEvent(const int nEvent, ConnectionPtr connection)
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