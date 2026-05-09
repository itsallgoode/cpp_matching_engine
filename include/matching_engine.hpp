#pragma once

#include "definitions.hpp"
#include "order_book.hpp"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

class MatchingEngine {
public:
    OrderAck process_order(Order order);

    bool cancel_order(const std::string& symbol, OrderId order_id);

    std::optional<PriceTicks> best_bid(const std::string& symbol);
    std::optional<PriceTicks> best_ask(const std::string& symbol);

    std::vector<std::pair<PriceTicks, Quantity>>
    bid_levels(const std::string& symbol, std::size_t levels) const;

    std::vector<std::pair<PriceTicks, Quantity>>
    ask_levels(const std::string& symbol, std::size_t levels) const;

    std::optional<PriceTicks> spread(const std::string& symbol);
    std::optional<double> mid_price(const std::string& symbol);

    const std::vector<Trade>& trades() const;

private:
    std::unordered_map<std::string, OrderBook> books_;
    std::vector<Trade> trades_;

    OrderId next_order_id_ = 1;
    TradeId next_trade_id_ = 1;
};