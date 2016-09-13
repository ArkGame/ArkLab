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

    }

    bool TCPServer::AddMsgProcessCallback(const int nMsgID, ConnectionCallback cb)
    {

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

    bool TCPServer::DoConnectionEvent(int nEvent, ConnectionPtr connection)
    {

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