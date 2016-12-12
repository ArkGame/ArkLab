#pragma once

#include <memory>
#include "header.hpp"

namespace ArkNet
{

using ios_worker_ptr = std::unique_ptr<io_service_t::work>;

class ios_worker
{
public:
    ios_worker()
        : ios_()
        , work_(std::make_unique<io_service_t::work>(ios_))
    {}

    void start()
    {
        worker_thread_ = std::move(std::thread{ boost::bind(&io_service_t::run, &ios_) });
    }

    void stop()
    {
        work_.reset();
        if(!ios_.stopped())
        {
            ios_.stop();
        }
    }

    void wait()
    {
        if(worker_thread_.joinable())
        {
            worker_thread_.join();
        }
    }

    auto& get_io_service()
    {
        return ios_;
    }

private:
    io_service_t ios_;
    ios_worker_ptr work_;
    std::thread worker_thread_;
};


class io_service_pool : boost::noncopyable
{
public:
    explicit io_service_pool(size_t pool_size)
        : ios_workers_(pool_size)
        , next_service_(ios_workers_.begin())
    {}

    ~io_service_pool()
    {
        stop();
    }

    void start()
    {
        for(auto& worker : ios_workers_)
        {
            worker.start();
        }
    }

    void stop()
    {
        for(auto& worker : ios_workers_)
        {
            worker.stop();
        }

        for(auto& worker : ios_workers_)
        {
            worker.wait();
        }
    }

    auto& get_io_service()
    {
        auto current_iter = next_service_++;
        if(ios_workers_.end() != next_service_)
        {
            next_service_ = ios_workers_.begin();
        }

        return current_iter->get_io_service();
    }

private:
    std::vector<ios_worker> ios_workers_;
    std::vector<ios_worker>::iterator next_service_;
};

}