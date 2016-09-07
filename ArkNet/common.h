#pragma once
#include "config.h"
#include "error.h"

namespace ArkNet
{

static const uint64_t dbg_none = 0;
static const uint64_t dbg_all = ~(uint64_t)0;
static const uint64_t dbg_hook = 0x1;
static const uint64_t dbg_yield = 0x1 << 1;
static const uint64_t dbg_scheduler = 0x1 << 2;
static const uint64_t dbg_task = 0x1 << 3;
static const uint64_t dbg_switch = 0x1 << 4;
static const uint64_t dbg_ioblock = 0x1 << 5;
static const uint64_t dbg_wait = 0x1 << 6;
static const uint64_t dbg_exception = 0x1 << 7;
static const uint64_t dbg_syncblock = 0x1 << 8;
static const uint64_t dbg_timer = 0x1 << 9;
static const uint64_t dbg_scheduler_sleep = 0x1 << 10;
static const uint64_t dbg_sleepblock = 0x1 << 11;
static const uint64_t dbg_spinlock = 0x1 << 12;
static const uint64_t dbg_fd_ctx = 0x1 << 13;
static const uint64_t dbg_debugger = 0x1 << 14;
static const uint64_t dbg_signal = 0x1 << 15;
static const uint64_t dbg_sys_max = dbg_debugger;

static const uint64_t dbg_accept_error = dbg_sys_max;
static const uint64_t dbg_accept_debug = dbg_sys_max << 1;
static const uint64_t dbg_session_alive = dbg_sys_max << 2;
static const uint64_t dbg_no_delay = dbg_sys_max << 3;
static const uint64_t dbg_network_max = dbg_no_delay;

enum class proto_type
{
    unknown     = 0 ,
    tcp             ,
    udp             ,
    ssl             ,
    http            ,
    https           ,
};

char const* proto_type_string[] = {
    "unknown",
    "tcp",
    "udp",
    "ssl",
    "http",
    "https",
};

proto_type str2proto(const std::string& s)
{
    static int n = sizeof(proto_type_string) / sizeof(char const*);
    for (int i = 0; i < n; ++i)
    {
        if (strcmp(s.c_str(), proto_type_string[i]) == 0)
        {
            return proto_type(i);
        }
    }

    return proto_type::unknown;
}

std::string proto2str(proto_type proto)
{
    static int n = sizeof(proto_type_string) / sizeof(char const*);
    if ((int)proto >= n || (int)proto < 0)
    {
        return proto_type_string[0];
    }

    return proto_type_string[(int)proto];
}

//////////////////////////////////////////////////////////////////////////
struct ext_t
{
    ext_t() : proto_(proto_type::unknown) {}
    explicit ext_t(proto_type proto) : proto_(proto) {}
    ext_t(proto_type proto, std::string const& path) : proto_(proto), path_(path) {}

    proto_type proto_;
    std::string path_;
};


class protocol;

class endpoint : public boost::asio::ip::basic_endpoint<protocol>
{
public:
    typedef boost::asio::ip::basic_endpoint<protocol> base_t;

    endpoint() {}
    endpoint(const endpoint&) = default;
    endpoint(endpoint&&) = default; //move constructor
    endpoint& operator=(const endpoint&) = default;
    endpoint& operator=(endpoint&&) = default;

    template<typename Proto>
    explicit endpoint(const boost::asio::ip::basic_endpoint<Proto>& ep)
        : base_t(ep.address(), ep.port())
    {
    }

    template<typename Proto>
    explicit endpoint(const boost::asio::ip::basic_endpoint<Proto>& ep, proto_type proto)
        : base_t(ep.address(), ep.port()), ext(proto)
    {
    }

    operator boost::asio::ip::tcp::endpoint() const
    {
        return boost::asio::ip::tcp::endpoint(this->address(), this->port());
    }

    operator boost::asio::ip::udp::endpoint() const
    {
        return boost::asio::ip::udp::endpoint(this->address(), this->port());
    }

    proto_type proto() const
    {
        return ext.proto_;
    }

    void set_proto(proto_type proto)
    {
        ext.proto_ = proto;
    }

    const std::string& path() const
    {
        return ext.path_;
    }



private:
    ext_t ext;
};

}