#pragma once

//boost libraries
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio/steady_timer.hpp>

//standard libraries
#include <cstdint>
#include <atomic>
#include <chrono>
#include <string>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <tuple>
#include <list>
#include <vector>
#include <array>
#include <utility>
#include <map>
#include <stdexcept>
#include <type_traits>

namespace ArkNet
{

using tcp = boost::asio::ip::tcp;
using io_service_t = boost::asio::io_service;
using deadline_timer_t = boost::asio::deadline_timer;
using lock_t = std::unique_lock<std::mutex>;
using steady_timer_t = boost::asio::steady_timer;
using duration_t = boost::asio::steady_timer::duration;

class connection;
class server;

static const auto asio_error = boost::asio::placeholders::error;

}