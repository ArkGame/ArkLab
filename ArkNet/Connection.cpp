#include "Connection.h"

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
		//TODO:server disconnected event
		
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

        //TODO:server connected event

        //TODO:4是包头长度
        boost::asio::async_read(mxSocket, mxResponseBuffer, boost::asio::transfer_exactly(4),
            std::bind(&Connection::HandleReadHeader,
                shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred)
        );

    }

    void Connection::HandleReadHeader(const boost::system::error_code& error, std::size_t nTransferedBytes)
    {
        if (error || !mbConnected)
        {
            Close();
            return;
        }

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

        boost::asio::async_read(mxSocket, mxResponseBuffer, boost::asio::transfer_at_least(nPacketLength), 
            std::bind(&Connection::HandleReadBody, 
                shared_from_this(), 
                boost::asio::placeholders::error, 
                boost::asio::placeholders::bytes_transferred)
        );
    }

    void Connection::HandleReadBody(const boost::system::error_code& error, std::size_t nTransferedBytes)
    {
        if (error || !mbConnected)
        {
            Close();
            return;
        }

        int nPacketLength = static_cast<int>(nTransferedBytes + 4); //???
        std::string strMsg;
        strMsg.resize(nPacketLength);
        mxResponseBuffer.sgetn(&strMsg[0], nPacketLength);
        mxResponseBuffer.consume(nPacketLength);

        mxServer.ProcessMsg(strMsg, shared_from_this());

        boost::asio::async_read(mxSocket, mxResponseBuffer, 
            std::bind(&Connection::HandleReadHeader, 
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
                    std::bind(&Connection::HandleWrite,
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
                std::bind(&Connection::HandleWrite, 
                    shared_from_this(), 
                    boost::asio::placeholders::error)
            );
        }
    }

    boost::any Connection::GetProperty(const std::string& property)
    {
        return mxConnectPropertyMap[property];
    }

    void Connection::SetProperty(const std::string& property, const boost::any& value)
    {
        mxConnectPropertyMap[property] = value;
    }

}