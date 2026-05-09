#include "definitions.hpp"
#include "order_book.hpp"

#include <iostream>
#include <vector>

void print_trade(const Trade& trade) {
    std::cout << "Trade "
              << trade.trade_id
              << " | symbol=" << trade.symbol
              << " | price=" << trade.price_ticks
              << " | qty=" << trade.quantity
              << " | buy_order_id=" << trade.buy_order_id
              << " | sell_order_id=" << trade.sell_order_id
              << "\n";
}

void print_levels(const std::vector<std::pair<PriceTicks, Quantity>>& levels) {
    for (const auto& [price, quantity] : levels) {
        std::cout << "  " << price << " -> " << quantity << "\n";
    }
}

int main() {
    try {
        OrderBook book;
        TradeId next_trade_id = 1;

        std::cout << "C++ OrderBook sanity test\n\n";

        Order sell_order{
            .symbol = "AAPL",
            .order_id = 1,
            .side = Side::Sell,
            .order_type = OrderType::Limit,
            .price_ticks = 10000,
            .quantity = 10,
            .owner_id = "seller_1",
        };

        auto trades_1 = book.process_order(sell_order, next_trade_id);

        std::cout << "After resting sell order:\n";
        std::cout << "trades returned: " << trades_1.size() << "\n";

        auto best_ask = book.best_ask();
        if (best_ask) {
            std::cout << "best ask: " << *best_ask << "\n";
        } else {
            std::cout << "best ask: none\n";
        }

        std::cout << "Ask levels:\n";
        print_levels(book.ask_levels(5));
        std::cout << "\n";

        Order buy_order{
            .symbol = "AAPL",
            .order_id = 2,
            .side = Side::Buy,
            .order_type = OrderType::Limit,
            .price_ticks = 10000,
            .quantity = 5,
            .owner_id = "buyer_1",
        };

        auto trades_2 = book.process_order(buy_order, next_trade_id);

        std::cout << "After crossing buy order:\n";
        std::cout << "trades returned: " << trades_2.size() << "\n";

        for (const auto& trade : trades_2) {
            print_trade(trade);
        }

        std::cout << "Ask levels after partial fill:\n";
        print_levels(book.ask_levels(5));
        std::cout << "\n";

        Order buy_resting{
            .symbol = "AAPL",
            .order_id = 3,
            .side = Side::Buy,
            .order_type = OrderType::Limit,
            .price_ticks = 9900,
            .quantity = 20,
            .owner_id = "buyer_2",
        };

        auto trades_3 = book.process_order(buy_resting, next_trade_id);

        std::cout << "After resting buy order:\n";
        std::cout << "trades returned: " << trades_3.size() << "\n";

        auto best_bid = book.best_bid();
        if (best_bid) {
            std::cout << "best bid: " << *best_bid << "\n";
        } else {
            std::cout << "best bid: none\n";
        }

        std::cout << "Bid levels:\n";
        print_levels(book.bid_levels(5));
        std::cout << "\n";

        bool canceled = book.cancel_order(3);

        std::cout << "Cancel order 3 result: "
                  << (canceled ? "true" : "false")
                  << "\n";

        best_bid = book.best_bid();
        if (best_bid) {
            std::cout << "best bid after cancel: " << *best_bid << "\n";
        } else {
            std::cout << "best bid after cancel: none\n";
        }

        std::cout << "\nAll sanity checks ran.\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}