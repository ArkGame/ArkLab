#pragma once

#include <vector>
#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

namespace ArkNet
{

class ServicePool : public boost::noncopyable
{
public:
    explicit ServicePool(std::size_t size);
    void Run();
    void Stop();
    boost::asio::io_service& GetService();

private:
    typedef std::shared_ptr<boost::asio::io_service> ServicePtr;
    typedef std::shared_ptr<boost::asio::io_service::work> WorkPtr;

    std::vector<ServicePtr> mxServices;
    std::vector<WorkPtr> mxWorks;
    std::size_t mnNextService;
};

}