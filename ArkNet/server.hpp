#pragma once
#include <stdint.h>
#include "header.hpp"
#include "io_service_pool.hpp"
#include "boost/asio.hpp"
#include "router.hpp"

namespace ArkNet
{

class server
{
public:
    using connection_weak_ptr = std::weak_ptr<connection>;
    using router_t = typename router;
    using invoker_t = typename router_t::invoker_t;
    using sub_container = std::multimap<std::string, connection_weak_ptr>;

public:
    server(uint16_t port, size_t pool_size, duration_t time_out = duration_t::max())
        : ios_pool_(pool_size)
        , acceptor_(ios_pool_.get_io_service(), tcp::endpoint{tcp::v4(), port})
        , time_out_(time_out)
    {
        init_callback_functions();

        //TODO
        register_handler();
    }

    ~server()
    {
        stop();
    }

    void start()
    {
        ios_pool_.start();
        do_accept();
    }

    void stop()
    {
        ios_pool_.stop();
    }

    template<typename Handler, typename PostFunc>
    bool register_invoker(const std::string& name, Handler&& handler, PostFunc&& post_func)
    {
        return router_.register_invoker(name, std::forward<handler>(handler), std::forward<PostFunc>(post_func));
    }

    template<typename Handler, typename PostFunc>
    bool async_register_invoker(const std::string& name, Handler&& handler, PostFunc&& post_func)
    {
        return router_.async_register_invoker(name, std::forward<handler>(handler), std::forward<PostFunc>(post_func));
    }

    template<typename Handler>
    bool register_invoker(const std::string& name, Handler&& handler)
    {
        return router_.register_invoker(name, std::forward<Handler>(handler));
    }

    template<typename Handler>
    bool async_register_invoker(const std::string& name, Handler&& handler)
    {
        return router_.async_register_invoker(name, std::forward<Handler>(handler));
    }

protected:
    void init_callback_functions()
    {
        router_.set_on_read([this](connection_ptr conn)
        {
            auto read_buffer = conn->get_read_buffer();
            router_.apply_invoker(conn, read_buffer.data(), read_buffer.size());
        });

        router_.set_on_error([this](connection_ptr conn, const boost::system::error_code & error)
        {
            remove_sub_conn(conn);
        });
    }

    void do_accept()
    {
        auto new_connection = std::make_shared<connection>(ios_pool_.get_io_service(), router_, time_out_);
        acceptor_.async_accept(new_connection->socket(),
                               [this, new_connection](const boost::system::error_code & error)
        {
            if(!error)
            {
                new_connection->start();
            }
            else
            {
                //log acception error
            }

            do_accept();
        });
    }

private:
    router_t router_;
    io_service_pool ios_pool_;
    boost::asio::ip::tcp::acceptor acceptor_;
    duration_t time_out_;
    mutable std::mutex mutex_;
    sub_container subscribers; //pub/sub for rpc, maybe never be used in game, will be deleted
};


}