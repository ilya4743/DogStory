#include "ticker.h"
#include <boost/beast/core.hpp>

Ticker::Ticker(std::shared_ptr<Strand> strand, std::chrono::milliseconds period, Handler handler)
    : strand_{strand}, period_{period}, handler_{std::move(handler)},timer_{*strand_} {
}

Ticker::Ticker(net::io_context& ioc, std::chrono::milliseconds period, Handler handler):
    period_{period},
    handler_{handler},
    timer_(ioc){
}

void Ticker::Start() {
    auto last_tick_ = std::chrono::steady_clock::now();
    /* Выполнить SchedulTick внутри strand_ */
    timer_.expires_after(period_);
    if(strand_) {
        timer_.async_wait(net::bind_executor(*strand_, [self = shared_from_this()](sys::error_code ec) {
            self->OnTick(ec);
        }));
    } else {
        timer_.async_wait([self = shared_from_this()](sys::error_code ec) {
            self->OnTick(ec);
        });
    }
}

void Ticker::ScheduleTick() {
    /* выполнить OnTick через промежуток времени period_ */
    timer_.expires_after(period_);
    timer_.async_wait([self = shared_from_this()](sys::error_code ec) {
        self->OnTick(ec);
    });
}
void Ticker::OnTick(sys::error_code ec) {
    if (!ec) {
        auto current_tick = std::chrono::steady_clock::now();
        auto delta = duration_cast<std::chrono::milliseconds>(current_tick - last_tick_);
        last_tick_ = current_tick;
        handler_(delta);
        ScheduleTick();
    }
}