#pragma once

namespace ArkNet
{

template<typename CodecPolicy>
class server
{
public:
    using codec_policy = CodecPolicy;
    using connection_ptr = std::shared_ptr<connection>;
    using connection_weak_ptr = std::weak_ptr<connection>;

public:
protected:
private:
};


}