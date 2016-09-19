#include "Connection.h"
#include "TCPServer.h"

namespace ArkNet
{

Connection::Connection(boost::asio::io_service& io, TCPServer& server, ConnectionManager* connMgr)
    : mxIOS(io)
    , mxServer(server)
    , mxSocket(io)
    , mxConnectionMgr(connMgr)
    , mbConnected(false)
{

}

Connection::~Connection()
{
    Stop();
}

void Connection::Stop()
{
    mxServer.DoConnectionEvent(Connection::DISCONNECTED, shared_from_this());
    mbConnected = false;
    boost::system::error_code ec;
    mxSocket.close(ec);
}

void Connection::Close()
{
    mxConnectionMgr->Stop(shared_from_this());
}

void Connection::Start()
{
    mxResponseBuffer.consume(mxResponseBuffer.size());
    mbConnected = true;

    boost::system::error_code error;
    mxSocket.set_option(boost::asio::ip::tcp::no_delay(true), error);
    if (error)
    {
        std::cout << "set tcp::no_delay failed, error info : " << error.message();
    }

    mxServer.DoConnectionEvent(Connection::CONNECTED, shared_from_this());

    //4是包头长度
    boost::asio::async_read(mxSocket, mxResponseBuffer, boost::asio::transfer_exactly(4),
                            boost::bind(&Connection::HandleReadHeader,
                                        shared_from_this(),
                                        boost::asio::placeholders::error,
                                        boost::asio::placeholders::bytes_transferred)
                           );
}

void Connection::HandleReadHeader(const boost::system::error_code& error, std::size_t bytes_transferred)
{
    if (error || !mbConnected)
    {
        Close();
        return;
    }

    //拷贝包头出来(4个字节的包体长度)
    boost::asio::streambuf tempbuf;
    boost::asio::buffer_copy(tempbuf.prepare(mxResponseBuffer.size()), mxResponseBuffer.data());
    tempbuf.commit(mxResponseBuffer.size());

    int32_t nPacketLength = 0;
    tempbuf.sgetn((char*)(&nPacketLength), sizeof(nPacketLength));
    nPacketLength = ntohl(nPacketLength);

    if (nPacketLength > 512 * 1024)
    {
        Close();
        return;
    }

    boost::asio::async_read(mxSocket, mxResponseBuffer, boost::asio::transfer_exactly(nPacketLength),
                            boost::bind(&Connection::HandleReadBody,
                                        shared_from_this(),
                                        boost::asio::placeholders::error,
                                        boost::asio::placeholders::bytes_transferred)
                           );
}

void Connection::HandleReadBody(const boost::system::error_code& error, std::size_t bytes_transferred)
{
    if (error || !mbConnected)
    {
        Close();
        return;
    }

    int nPacketLength = static_cast<int>(bytes_transferred + 4); //???
    std::string strMsg;
    strMsg.resize(nPacketLength);
    mxResponseBuffer.sgetn(&strMsg[0], nPacketLength);
    mxResponseBuffer.consume(nPacketLength);

    //TODO:此处处理数据解析

    int nMsgID = 0;
    mxServer.ProcessMsg(nMsgID, strMsg, shared_from_this());

    boost::asio::async_read(mxSocket, mxResponseBuffer, boost::asio::transfer_exactly(4), //包头4个字节
                            boost::bind(&Connection::HandleReadHeader,
                                        shared_from_this(),
                                        boost::asio::placeholders::error,
                                        boost::asio::placeholders::bytes_transferred)
                           );
}

void Connection::WriteMsg(const std::string& msg, const WriteCallback& cb /*= WriteCallback()*/)
{
    mxIOS.post(std::bind(&Connection::DoWrite, shared_from_this(), msg, cb));
}

void Connection::HandleWrite(const boost::system::error_code& error)
{
    WriteCallback& cb = mxWriteQueue.front().handler;
    if (cb)
    {
        cb(error);
    }

    if (error)
    {
        Close();
    }
    else
    {
        mxWriteQueue.pop_front();
        if (!mxWriteQueue.empty())
        {
            boost::asio::async_write(mxSocket,
                                     boost::asio::buffer(mxWriteQueue.front().msg.data(), mxWriteQueue.front().msg.length()),
                                     boost::bind(&Connection::HandleWrite,
                                                 shared_from_this(),
                                                 boost::asio::placeholders::error)
                                    );
        }
    }
}

void Connection::DoWrite(std::string msg, WriteCallback cb)
{
    bool bWriting = !mxWriteQueue.empty();
    InternelMsg item = { msg, cb };
    mxWriteQueue.push_back(item);
    if (!bWriting)
    {
        boost::asio::async_write(mxSocket,
                                 boost::asio::buffer(mxWriteQueue.front().msg.data(), mxWriteQueue.front().msg.length()),
                                 boost::bind(&Connection::HandleWrite,
                                             shared_from_this(),
                                             boost::asio::placeholders::error)
                                );
    }
}

}