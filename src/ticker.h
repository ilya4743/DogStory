#pragma once
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <chrono>
#include <functional>
#include <memory>

namespace net = boost::asio;
namespace sys = boost::system;

class Ticker : public std::enable_shared_from_this<Ticker> {
   public:
    using Strand = net::strand<net::io_context::executor_type>;
    using Handler = std::function<void(std::chrono::milliseconds delta)>;

    Ticker(std::shared_ptr<Strand> strand, std::chrono::milliseconds period, Handler handler);
    Ticker(net::io_context& ioc, const std::chrono::milliseconds period, Handler handler);

    ~Ticker() = default;
    void Start();

   private:
    void ScheduleTick();
    void OnTick(sys::error_code ec);

    std::shared_ptr<Strand> strand_;
    net::steady_timer timer_;
    std::chrono::milliseconds period_;
    Handler handler_;
    std::chrono::steady_clock::time_point last_tick_;
};