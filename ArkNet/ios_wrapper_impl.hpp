#pragma once

#include "ios_wrapper.hpp"
#include "connection.hpp"

namespace ArkNet
{

ios_wrapper::ios_wrapper(io_service_t& ios)
    : ios_(ios)
    , write_in_progress_(false)
{

}

void ios_wrapper::write(connection_ptr& conn_ptr, context_ptr& context)
{
    assert(nullptr != context);

    lock_t lock{ mutex_ };
    if(!write_in_progress_)
    {
        write_in_progress_ = true;
        lock.unlock();
        write_progress_entry(conn_ptr, context);
    }
    else
    {
        delay_msgs_.emplace_back(conn_ptr, context);
    }
}

io_service_t& ios_wrapper::get_io_service() noexcept
{
    return ios_;
}

void ios_wrapper::write_progress_entry(connection_ptr& conn_ptr, context_ptr& context)
{
    assert(nullptr != context);
    boost::asio::async_write(conn_ptr->socket(), context->get_msg(),
                             boost::bind(&ios_wrapper::handle_write_entry, this, conn_ptr, context, asio_error));
}

void ios_wrapper::write_progress()
{
    lock_t lock{ mutex_ };
    if(delay_msgs_.empty())
    {
        write_in_progress_ = false;
        return;
    }
    else
    {
        context_container_t delay_msgs = std::move(delay_msgs_);
        lock.unlock();
        write_progress(std::move(delay_msgs));
    }
}

void ios_wrapper::write_progress(context_container_t&& delay_msgs)
{
    auto& conn_ptr = delay_msgs_.front().first;
    auto& ctx_ptr = delay_msgs_.front().second;

    boost::asio::async_write(conn_ptr->socket(), ctx_ptr->get_msg(),
                             boost::bind(&ios_wrapper::handle_write, this, std::move(delay_msgs_), std::placeholders::_1));
}

void ios_wrapper::handle_write_entry(connection_ptr conn_ptr, context_ptr context, const boost::system::error_code& error)
{
    assert(nullptr != context);
    if(!conn_ptr->socket().is_open())
    {
        return;
    }

    if(!error)
    {
        //call the registered function
        context->apply_post_func();
        //release shared_ptr
        context.reset();
        //continue to write
        write_progress();
    }
    else
    {
        conn_ptr->on_error(error);
    }
}

void ios_wrapper::handle_write(context_container_t& delay_msgs, const boost::system::error_code& error)
{
    connection_ptr conn_ptr;
    context_ptr ctx_ptr;
    //会把一个元组数据解包成内部的多个数据(tie返回的是左值引用)
    std::tie(conn_ptr, ctx_ptr) = std::move(delay_msgs.front());
    delay_msgs.pop_front();

    if(!conn_ptr->socket().is_open())
    {
        return;
    }

    if(!error)
    {
        //call the registered function
        ctx_ptr->apply_post_func();
        //release shared_ptr
        ctx_ptr.reset();
        conn_ptr.reset();

        //continue
        if(!delay_msgs.empty())
        {
            write_progress(std::move(delay_msgs));
        }
        else
        {
            write_progress();
        }
    }
    else
    {
        conn_ptr->on_error(error);
    }
}

}
