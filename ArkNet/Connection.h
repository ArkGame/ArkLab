#pragma once

#include <deque>
#include <set>
#include <algorithm>
#include "boost/noncopyable.hpp"
#include "boost/enable_shared_from_this.hpp"
#include "boost/asio.hpp"
#include "boost/any.hpp"
#include "boost/thread.hpp"


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

        boost::any GetProperty(const std::string& property);
        void SetProperty(const std::string& property, const boost::any& value);
	protected:
		void Close();
		void HandleReadHeader(const boost::system::error_code& error, std::size_t nTransferedBytes);
        void HandleReadBody(const boost::system::error_code& error, std::size_t nTransferedBytes);
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
    //typedef std::weak_ptr<Connection> ConnenctionWeakPtr;

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