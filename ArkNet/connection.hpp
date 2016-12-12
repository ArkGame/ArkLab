#pragma once

#include <memory>
#include <functional>
#include "boost/system/error_code.hpp"

namespace ArkNet
{

class ios_wrapper;

class connection : public std::enable_shared_from_this<connection>
{
public:

    template <typename CodecPolicy> friend class serever;
    template <typename CodecPolicy> friend class router;
    friend class ios_wrapper;

    using connection_ptr = std::shared_ptr<connection>;
    using context_ptr = std::shared_ptr<context_t>;
    using conenction_on_error_t = std::function<void(connection_ptr, const boost::system::error_code&)>;


protected:
private:

};

}