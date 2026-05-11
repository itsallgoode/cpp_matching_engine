#include "order_book.hpp"
#include "definitions.hpp"
#include <algorithm>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <utility>

std::vector<Trade> OrderBook::process_order(Order order, TradeId&  next_trade_id) {
    if (order.is_limit() && !order.price_ticks) {
        throw std::invalid_argument("Limit order must have a price");
    }

    std::vector<Trade> trades;

    if (order.is_buy()) {
        trades = match_buy(order, next_trade_id);
    }
    else if (order.is_sell()) {
        trades = match_sell(order, next_trade_id);
    }
    else {
        throw std::invalid_argument("Invalid order side");
    }

    if (order.is_market() && order.is_active()) {
        order.status = OrderStatus::Canceled;
    }

    switch (order.time_in_force) {
        case TimeInForce::Day:

            break;
        
        case TimeInForce::FOK:

            break;
        
        case TimeInForce::GTC:

            break;
        
        case TimeInForce::IOC:

            break;
    }


    if (order.is_limit() && order.is_active()) {
        add_to_book(order);
    }



    return trades;
}  

bool OrderBook::cancel_order(OrderId order_id) {
    auto order_node = orders_by_id_.extract(order_id);

    if (order_node.empty()) {
        return false;
    }

    Order* order = order_node.mapped();
    order->status = OrderStatus::Canceled;

    return true;
}   

std::optional<PriceTicks> OrderBook::best_bid() {
    while (!bids_.empty()) {
        auto it = std::prev(bids_.end());
        PriceTicks price = it->first;
        auto& queue = it->second;

        while ((!queue.empty() && !queue.front().is_active()) || queue.front().is_expired()) {
            queue.pop_front();
        }

        if (queue.empty()) {
            bids_.erase(it);
            continue;
        }

        return price;
    }

    return std::nullopt;
}

std::optional<PriceTicks> OrderBook::best_ask() {
    while (!asks_.empty()) {
        auto it = asks_.begin();
        PriceTicks price = it->first;
        auto& queue = it->second;

        while ((!queue.empty() && !queue.front().is_active()) || (queue.front().is_expired())) {
            queue.pop_front();
        }

        if (queue.empty()) {
            asks_.erase(it);
            continue;
        }

        return price;
    }

    return std::nullopt;
}

std::vector<Trade> OrderBook::match_buy(Order& order, TradeId& next_trade_id) {
    std::vector<Trade> trades;

    while (order.is_active()) {
        auto best_price = best_ask();

        if (!best_price || (order.is_limit() && *best_price > *order.price_ticks)) {
            break;
        }

        Order& oldest_sell_order = asks_.at(*best_price).front();
        Quantity trade_quantity = std::min(
            order.remaining_quantity,
            oldest_sell_order.remaining_quantity);

        oldest_sell_order.fill(trade_quantity);
        order.fill(trade_quantity);

        Trade trade = {
            .symbol = order.symbol,
            .trade_id = next_trade_id++,
            .buy_owner_id = order.owner_id,
            .sell_owner_id = oldest_sell_order.owner_id,
            .buy_order_id = order.order_id,
            .sell_order_id = oldest_sell_order.order_id,
            .aggressor_order_id = order.order_id,
            .aggressor_side = order.side,
            .resting_order_id = oldest_sell_order.order_id,
            .price_ticks = *best_price,
            .quantity = trade_quantity
        };

        if (!oldest_sell_order.is_active()) {
            orders_by_id_.erase(oldest_sell_order.order_id);
        }

        trades.push_back(trade);

    } 
    
    return trades;
}

std::vector<Trade> OrderBook::match_sell(Order& order, TradeId& next_trade_id) {
    std::vector<Trade> trades;

    while (order.is_active()) {
        auto best_price = best_bid();

        if (!best_price || (order.is_limit() && *best_price < *order.price_ticks)) {
            break;
        }

        Order& oldest_buy_order = bids_.at(*best_price).front();
        Quantity trade_quantity = std::min(
            order.remaining_quantity,
            oldest_buy_order.remaining_quantity
        );

        oldest_buy_order.fill(trade_quantity);
        order.fill(trade_quantity);

        Trade trade = {
            .symbol = order.symbol,
            .trade_id = next_trade_id++,
            .buy_owner_id = oldest_buy_order.owner_id,
            .sell_owner_id = order.owner_id,
            .buy_order_id = oldest_buy_order.order_id,
            .sell_order_id = order.order_id,
            .aggressor_order_id = order.order_id,
            .aggressor_side = order.side,
            .resting_order_id = oldest_buy_order.order_id,
            .price_ticks = *best_price,
            .quantity = trade_quantity
        };

        if (!oldest_buy_order.is_active()) {
            orders_by_id_.erase(oldest_buy_order.order_id);
        }

        trades.push_back(trade);

    }

    return trades;
}

void OrderBook::add_to_book(Order order) {
    if (order.is_buy()) {
        auto& deque = bids_[*order.price_ticks];
        deque.push_back(std::move(order));
        orders_by_id_.try_emplace(deque.back().order_id, &deque.back());
    }
    else if (order.is_sell()) {
        auto& deque = asks_[*order.price_ticks];
        deque.push_back(std::move(order));
        orders_by_id_.try_emplace(deque.back().order_id, &deque.back());
    }
}

std::vector<std::pair<PriceTicks, Quantity>> OrderBook::bid_levels(std::size_t levels) const {
    std::vector<std::pair<PriceTicks, Quantity>> bids;

    for (auto it = bids_.rbegin(); it != bids_.rend(); ++it) {
        PriceTicks price = it->first;
        auto& deque = it->second;
        if (bids.size() == levels) {
            break;
        }

        Quantity quantity = 0;

        for (const auto& order : deque) {
            if (order.is_active() && !order.is_expired()) {
                quantity += order.remaining_quantity;
            }
        }

        if (quantity) bids.push_back(std::make_pair(price, quantity));
    }
    return bids;
}

std::vector<std::pair<PriceTicks, Quantity>> OrderBook::ask_levels(std::size_t levels) const {
    std::vector<std::pair<PriceTicks, Quantity>> asks;

    for (const auto& [price, deque] : asks_) {
        if (asks.size() == levels) {
            break;
        }

        Quantity quantity = 0;

        for (const auto& order : deque) {
            if (order.is_active() && !order.is_expired()) {
                quantity += order.remaining_quantity;
            }
        }

        if (quantity) asks.push_back((std::make_pair(price, quantity)));
    }
    return asks;
}

std::optional<PriceTicks> OrderBook::spread() {
    auto ask = best_ask();
    auto bid = best_bid();

    if (!ask || !bid) {
        return std::nullopt;
    }

    return *ask - *bid;
}

std::optional<double> OrderBook::mid_price() {
    auto ask = best_ask();
    auto bid = best_bid();

    if (!ask || !bid) {
        return std::nullopt;
    }

    return static_cast<double>(*ask + *bid) / 2.0;
}
