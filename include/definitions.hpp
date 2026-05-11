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
    Sell,
};

enum class OrderType {
    Limit,
    Market,
};

enum class OrderStatus {
    Open,
    PartiallyFilled,
    Filled,
    Canceled,
};

enum class TimeInForce {
    Day,
    GTC, // good-till-canceled
    IOC, // immediate-or-cancel
    FOK, // fill-or-kill
};

struct Order {
    std::string symbol;
    OrderId order_id;

    Side side;
    OrderType order_type;
    TimeInForce time_in_force = TimeInForce::Day;
    std::optional<PriceTicks> price_ticks;
    Quantity quantity;

    OwnerId owner_id;

    OrderStatus status = OrderStatus::Open;
    Quantity remaining_quantity = quantity;
    std::optional<Timestamp> TTL;

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

    bool is_expired() const {
        Timestamp now = std::chrono::system_clock::now();
        return now > TTL;
    };

    bool is_filled() const {
        return remaining_quantity == 0;
    }

    bool is_GTC() const {
        return time_in_force == TimeInForce::Day;
    }

    bool is_IOC() const {
        return time_in_force == TimeInForce::IOC;
    } 

    bool is_FOK() const {
        return time_in_force == TimeInForce::FOK;
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