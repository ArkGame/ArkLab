#pragma once
#include <functional>
#include "boost/noncopyable.hpp"
#include "header.hpp"
#include <unordered_map>
#include <exception>

namespace ArkNet
{

class router_base : boost::noncopyable
{
public:
    using invoker_t = std::function<void(connection_ptr, const char*, size_t)>;
    using invoker_container = std::unordered_map<uint64_t, invoker_t>;
    using on_read_func = std::function<void(connection_ptr)>;
    using on_error_func = std::function<void(connection_ptr, const boost::system::error_code& error)>;

public:
    bool register_invoker(uint64_t id, invoker_t&& invoker)
    {
        return invokers_.emplace(id, std::move(invoker)).second;
    }

    bool has_invoker(uint64_t id) noexcept
    {
        return invokers_.find(id) != invokers_.end();
    }

    void on_read(const connection_ptr& conn_ptr)
    {
        if(on_read_)
        {
            on_read_(conn_ptr);
        }
    }

    void on_error(const connection_ptr& conn_ptr, const boost::system::error_code& error)
    {
        if(on_error_)
        {
            on_error(conn_ptr, error);
        }
    }

    void set_on_read(on_read_func&& on_read)
    {
        on_read_ = std::move(on_read);
    }

    void set_on_error(on_error_func&& on_error)
    {
        on_error_ = std::move(on_error);
    }
private:
    invoker_container invokers_;
    on_read_func on_read_;
    on_error_func on_error_;
};

class router : public router_base
{
public:
    using invoker_t = typename router_base::invoker_t;
    using invoker_container = typename router_base::invoker_container;
    using on_read_func = typename router_base::on_read_func;
    using on_error_func = typename router_base::on_error_func;

public:
    template<typename Handler, typename PostFunc>
    bool register_invoker(const std::string& name, Handler&& handler, PostFunc&& post_func)
    {
        //TODO
        return true;
    }

    template<typename Handler>
    bool register_invoker(const std::string& name, Handler&& handler)
    {
        //TODO
        return true;
    }

    template<typename Handler, typename PostFunc>
    bool async_register_invoker(const std::string& name, Handler&& handler, PostFunc&& post_func)
    {
        //TODO
        return true;
    }

    template<typename Handler>
    bool async_register_invoker(const std::string& name, Handler&& handler)
    {
        //TODO
        return true;
    }

    template<typename Handler>
    bool register_raw_invoker(const std::string& name, Handler&& handler)
    {
        return register_invoker_impl(name, handler);
    }

    bool has_invoker(const std::string& name)
    {
        auto hash = hash_(name);
        return router_base::has_invoker(hash);
    }

    void apply_invoker(connection_ptr conn, const char* data, size_t size) const
    {
        auto& header = conn->get_read_header();
        auto iter = this->invokers_.find(header.hash);
        if(iter == this->invokers_.end())
        {
            //log
            std::cout << "Cannot find invoker, ID = " << header.hash << std::endl;
        }
        else
        {
            auto& invoker = iter->second;
            if(!invoker)
            {
                //log error
                std::cout << "This invoker is invalid, ID = " << header.hash << std::endl;
                return;
            }

            try
            {
                invoker(conn, data, size);
            }
            catch(...)
            {
                //TODO:自己的exception
                //log error
                std::cout << "Execute this invoker failed, ID = " << header.hash << std::endl;
            }
        }
    }

protected:
    //变参模版(TODO:正常的函数应该不需要变参模版，处理的函数都是相同的)
    template<typename InvokerTraits, typename ... Handlers>
    bool register_invoker_impl(const std::string& name, Handlers&& ... handlers)
    {
        auto hash = hash_(name);
        if(has_invoker(hash))
        {
            return false;
        }

        auto invoker = InvokerTraits::get(std::forward<Handlers>(handlers)...);
        this->invokers_.emplace(hash, std::move(invoker));
        return true;
    }

    //定参模版
    template <typename Handler>
    bool register_invoker_impl(const std::string& name, Handler&& handler)
    {
        auto hash = hash_(name);
        if(has_invoker(hash))
        {
            return false;
        }

        auto invoker = std::forward<Handler>(handler);
        this->invokers_.emplace(hash, std::move(invoker));
        return true;
    }

private:
    std::hash<std::string> hash_;
};

}