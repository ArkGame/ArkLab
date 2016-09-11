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

	void Connection::Start()
	{

	}

	void Connection::Stop()
	{
		//TODO:disconnect event
		
		mbConnected = false;
		boost::system::error_code ec;
		mxSocket.close(ec);
	}

	void Connection::WriteMsg(const std::string& msg, const WriteCallback& cb /*= WriteCallback()*/)
	{

	}

	void Connection::Close()
	{
		mxConnectionMgr->Stop(shared_from_this());
	}

	void Connection::HandleRead(const boost::system::error_code& error, std::size_t nTransferedBytes)
	{

	}

	void Connection::HandleWrite(const boost::system::error_code& error)
	{

	}

	void Connection::DoWrite(std::string msg, WriteCallback cb)
	{

	}

}