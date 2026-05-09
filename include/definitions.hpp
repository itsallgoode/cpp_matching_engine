#pragma once

#include <chrono>
#include <optional>
#include <stdexcept>
#include <cstdint>
#include <string>

using Timestamp = std::chrono::time_point<std::chrono::system_clock>;
using OrderId = std::uint64_t;
using TradeId = std::uint64_t;
using OwnerId = std::string;
using PriceTicks = std::uint64_t;
using Quantity = std::uint64_t;

enum class Side {
    Buy,
    Sell
};

enum class OrderType {
    Limit,
    Market
};

enum class OrderStatus {
    Open,
    PartiallyFilled,
    Filled,
    Canceled
};

struct Order {
    std::string symbol;
    OrderId order_id;

    Side side;
    OrderType order_type;
    std::optional<PriceTicks> price_ticks;
    Quantity quantity;

    OwnerId owner_id;

    OrderStatus status = OrderStatus::Open;
    Quantity remaining_quantity = quantity;

    Timestamp timestamp = std::chrono::system_clock::now();

    bool is_buy() const {
        return side == Side::Buy;
    }

    bool is_sell() const {
        return side == Side::Sell;
    }

    bool is_limit() const {
        return order_type == OrderType::Limit;
    }

    bool is_market() const {
        return order_type == OrderType::Market;
    }

    bool is_active() const {
        return status == OrderStatus::Open ||
        status == OrderStatus::PartiallyFilled;
    }

    bool is_filled() const {
        return remaining_quantity == 0;
    }

    void fill(Quantity fill_quantity) {
        if (fill_quantity == 0) {
            throw std::invalid_argument("fill quantity must be positive");
        }

        if (fill_quantity > remaining_quantity) {
            throw std::invalid_argument("cannot fill more than remaining quantity");
        }

        remaining_quantity -= fill_quantity;

        if (is_filled()) {
            status = OrderStatus::Filled;
        } else {
            status = OrderStatus::PartiallyFilled;
        }
    }
};

struct Trade {
    std::string symbol;
    TradeId trade_id;

    OwnerId buy_owner_id;
    OwnerId sell_owner_id;

    OrderId buy_order_id;
    OrderId sell_order_id;
    OrderId aggressor_order_id;
    Side aggressor_side;
    OrderId resting_order_id;

    PriceTicks price_ticks;
    Quantity quantity;

    Timestamp timestamp = std::chrono::system_clock::now();
};

struct OrderAck {
    bool accepted;
    OrderId order_id;
    std::vector<Trade> trades;
};