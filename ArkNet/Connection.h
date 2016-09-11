#pragma once
#include "boost/noncopyable.hpp"
#include "boost/enable_shared_from_this.hpp"
#include "boost/asio.hpp"

namespace ArkNet
{

	class TCPServer;
	class ConnectionManager;
	class Connection : boost::noncopyable, boost::enable_shared_from_this<Connection>
	{
	public:
		explicit Connection(boost::asio::io_service& io, TCPServer& server, ConnectionManager* connMgr);
		~Connection();

		void Start();
		void Stop();

		boost::asio::ip::tcp::socket& GetSocket() { return mxSocket; }
		TCPServer& GetServer() { return mxServer; }

		typedef std::function<void(const boost::system::error_code&)> WriteCallback;
		void WriteMsg(const std::string& msg, const WriteCallback& cb = WriteCallback());

	protected:
		void Close();
		void HandleRead(const boost::system::error_code& error, std::size_t nTransferedBytes);
		void HandleWrite(const boost::system::error_code& error);

		void DoWrite(std::string msg, WriteCallback cb);

	private:
		boost::asio::io_service& mxIOS;
		TCPServer& mxServer;
		boost::asio::ip::tcp::socket mxSocket;
		ConnectionManager* mxConnectionMgr;
		bool mbConnected;
	};

}