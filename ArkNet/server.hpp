#pragma once

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


protected:
private:
};


}