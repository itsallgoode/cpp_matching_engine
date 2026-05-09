#pragma once

#include "definitions.hpp"

#include <deque>
#include <map>
#include <optional>
#include <unordered_map>
#include <vector>

class OrderBook {
public:
    std::vector<Trade> process_order(Order order, TradeId& next_trade_id);

    bool cancel_order(OrderId order_id);

    std::optional<PriceTicks> best_bid();
    std::optional<PriceTicks> best_ask();

    std::vector<std::pair<PriceTicks, Quantity>> bid_levels(std::size_t levels) const;
    std::vector<std::pair<PriceTicks, Quantity>> ask_levels(std::size_t levels) const;

    std::optional<PriceTicks> spread();
    std::optional<double> mid_price();

private:
    std::vector<Trade> match_buy(Order& order, TradeId& next_trade_id);
    std::vector<Trade> match_sell(Order& order, TradeId& next_trade_id);

    void add_to_book(Order order);

private:
    std::map<PriceTicks, std::deque<Order>> bids_;
    std::map<PriceTicks, std::deque<Order>> asks_;

    std::unordered_map<OrderId, Order*> orders_by_id_;
};