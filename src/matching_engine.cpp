#include "matching_engine.hpp"
#include <cstddef>

OrderAck MatchingEngine::process_order(Order order) {
    order.order_id = next_order_id_++;

    OrderBook& book = books_[order.symbol];

    auto trades = book.process_order(order, next_trade_id_);
    trades_.insert(trades_.end(), trades.begin(), trades.end());

    OrderAck order_ack = {
        .accepted = true,
        .order_id = order.order_id,
        .trades = std::move(trades)
    };

    return order_ack;
}

bool MatchingEngine::cancel_order(const std::string& symbol, OrderId order_id) {
    auto it = books_.find(symbol);

    if (it == books_.end()) {
        return false;
    }
    
    return it->second.cancel_order(order_id);
}

std::optional<PriceTicks> MatchingEngine::best_bid(const std::string& symbol) {
    auto it = books_.find(symbol);

    if (it == books_.end()) {
        return std::nullopt;
    }
    
    return it->second.best_bid();
}

std::optional<PriceTicks> MatchingEngine::best_ask(const std::string& symbol) {
    auto it = books_.find(symbol);

    if (it == books_.end()) {
        return std::nullopt;
    }
    
    return it->second.best_ask();
}

std::vector<std::pair<PriceTicks, Quantity>> MatchingEngine::bid_levels(const std::string& symbol, std::size_t levels) const {
    auto it = books_.find(symbol);

    if (it == books_.end()) {
        return {};
    }

    return it->second.bid_levels(levels);
}

std::vector<std::pair<PriceTicks, Quantity>> MatchingEngine::ask_levels(const std::string& symbol, std::size_t levels) const {
    auto it = books_.find(symbol);

    if (it == books_.end()) {
        return {};
    }

    return it->second.ask_levels(levels);
}

std::optional<PriceTicks> MatchingEngine::spread(const std::string& symbol) {
    auto it = books_.find(symbol);

    if (it == books_.end()) {
        return std::nullopt;
    }
    
    return it->second.spread();
}

std::optional<double> MatchingEngine::mid_price(const std::string& symbol) {
    auto it = books_.find(symbol);

    if (it == books_.end()) {
        return std::nullopt;
    }
    
    return it->second.mid_price();
}

const std::vector<Trade>& MatchingEngine::trades() const {
    return trades_;
}