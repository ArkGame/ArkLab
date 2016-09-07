#pragma once
#include "config.h"

namespace ArkNet
{

enum NetworkErrorCode
{
    ec_ok = 0,
    ec_connecting = 1,
    ec_estab = 2,
    ec_shutdown = 3,
    ec_half = 4,
    ec_no_destination = 5,
    ec_send_timeout = 6,
    ec_recv_timeout = 7,
    ec_url_parse_error = 8,
    ec_data_parse_error = 9,
    ec_unsupport_protocol = 10,
    ec_recv_overflow = 11,
    ec_send_overflow = 12,
    ec_dns_not_found = 13,

    // ºÊ»›
    ec_timeout = ec_send_timeout,
};

class network_error_category : public boost::system::error_category
{
public:
    virtual const char* name() const throw()
    {
        return "network_error";
    }

    virtual std::string message(int ec) const
    {
        switch ((NetworkErrorCode)ec)
        {
        case NetworkErrorCode::ec_ok:
            return "(network)ok";

        case NetworkErrorCode::ec_connecting:
            return "(network)client was connecting";

        case NetworkErrorCode::ec_estab:
            return "(network)client was ESTAB";

        case NetworkErrorCode::ec_shutdown:
            return "(network)user shutdown";

        case NetworkErrorCode::ec_half:
            return "(network)send or recv half of package";

        case NetworkErrorCode::ec_no_destination:
            return "(network)udp send must be appoint a destination address";

        case NetworkErrorCode::ec_send_timeout:
            return "(network)send time out";

        case NetworkErrorCode::ec_recv_timeout:
            return "(network)recv time out";

        case NetworkErrorCode::ec_url_parse_error:
            return "(network)URL parse error";

        case NetworkErrorCode::ec_data_parse_error:
            return "(network)data parse error";

        case NetworkErrorCode::ec_unsupport_protocol:
            return "(network)unsupported protocol";

        case NetworkErrorCode::ec_recv_overflow:
            return "(network)recv buf overflow";

        case NetworkErrorCode::ec_dns_not_found:
            return "(network)DNS not found";
        }

        return "";
    }
};

    const boost::system::error_category& GetNetworkErrorCategory()
    {
        static network_error_category obj;
        return obj;
    }

    boost::system::error_code MakeNetworkErrorCode(NetworkErrorCode code)
    {
        return boost::system::error_code((int)code, GetNetworkErrorCategory());
    }

    void ThrowError(NetworkErrorCode code, const char* what)
    {
        if (std::uncaught_exception()) return;
        throw boost::system::system_error(MakeNetworkErrorCode(code), what);
    }

}