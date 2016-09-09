#include "ServicePool.h"
#include <thread>
#include <functional>
#include <boost/thread.hpp>

namespace ArkNet
{

ServicePool::ServicePool(std::size_t size)
{
    mnNextService = 0;
    if (size == 0)
    {
        size = 1;
    }

    for (std::size_t i = 0; i < size; ++i)
    {
        ServicePtr xService(new boost::asio::io_service);
        WorkPtr xWork(new boost::asio::io_service::work(*xService));
        mxServices.push_back(xService);
        mxWorks.push_back(xWork);
    }
}

void ServicePool::Run()
{
    if (mxServices.size() == 1)
    {
        mxServices[0]->run();
        return;
    }

    typedef std::shared_ptr<std::thread> ThreadPtr;
    std::vector<ThreadPtr> xThreads;
    for (std::size_t i = 0; i < mxServices.size(); ++i)
    {
        ThreadPtr xThread(new std::thread(std::bind(static_cast<std::size_t(boost::asio::io_service::*)()>(&boost::asio::io_service::run), mxServices[i])));
        //CPU core°ó¶¨£¿
        xThreads.push_back(xThread);
    }

    for (size_t i = 0; i < xThreads.size(); ++i)
    {
        xThreads[i]->join();
    }
}

void ServicePool::Stop()
{
    for (std::size_t i = 0; i < mxServices.size(); ++i)
    {
        mxServices[i]->stop();
    }
}

boost::asio::io_service& ServicePool::GetService()
{
    boost::asio::io_service& xService = *(mxServices[mnNextService]);
    mnNextService = (mnNextService + 1) % mxServices.size();
    return xService;
}

}