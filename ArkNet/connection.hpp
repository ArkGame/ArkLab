#pragma once

#include <memory>
#include <functional>
#include "header.hpp"

namespace ArkNet
{

class ios_wrapper;

class connection : public std::enable_shared_from_this<connection>
{
public:

    friend class serever;
    friend class router;
    friend class ios_wrapper;

    using connection_ptr = std::shared_ptr<connection>;
    using context_ptr = std::shared_ptr<context_t>;

public:
    connection(io_service_t& ios, router_base& router, duration_t time_out);
    void close();
    io_service_t& get_io_service();

protected:
    tcp::socket& socket();
    void start();
    void send(context_ptr& ctx);
    void on_error(const boost::system::error_code& error);
    blob_t get_read_buffer() const;
    const req_header& get_read_header() const;

private:
    void set_no_delay();
    void expires_timer();
    void cancel_timer();
    void read_head();
    void read_body();

    void handle_read_head(const boost::system::error_code& error);
    void handle_read_body(const boost::system::error_code& error);
    void handle_time_out(const boost::system::error_code& error);

private:
    ios_worker ios_wrapper_;
    router_base& router_;
    tcp::socket socket_;
    req_header head_;
    std::vector<char> read_buffer_;
    steady_timer_t timer_;
    duration_t time_out_;
};

}