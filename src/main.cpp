#include "definitions.hpp"
#include "matching_engine.hpp"

#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

struct BenchmarkResult {
    std::string name;
    std::size_t orders;
    std::size_t trades;
    double seconds;

    double orders_per_sec() const {
        return static_cast<double>(orders) / seconds;
    }

    double trades_per_sec() const {
        if (trades == 0) {
            return 0.0;
        }

        return static_cast<double>(trades) / seconds;
    }
};

Order make_order(
    std::string symbol,
    Side side,
    OrderType order_type,
    std::optional<PriceTicks> price_ticks,
    Quantity quantity,
    OwnerId owner_id
) {
    return Order{
        .symbol = std::move(symbol),
        .order_id = 0, // MatchingEngine overwrites this
        .side = side,
        .order_type = order_type,
        .price_ticks = price_ticks,
        .quantity = quantity,
        .owner_id = std::move(owner_id),
    };
}

template <typename Func>
BenchmarkResult run_benchmark(
    const std::string& name,
    std::size_t orders,
    Func benchmark_func
) {
    auto start = std::chrono::steady_clock::now();

    std::size_t trades = benchmark_func();

    auto end = std::chrono::steady_clock::now();

    std::chrono::duration<double> elapsed = end - start;

    return BenchmarkResult{
        .name = name,
        .orders = orders,
        .trades = trades,
        .seconds = elapsed.count(),
    };
}

BenchmarkResult bench_resting_limit_adds(std::size_t n) {
    MatchingEngine engine;

    auto result = run_benchmark(
        "Resting limit adds",
        n,
        [&engine, n]() -> std::size_t {
            for (std::size_t i = 0; i < n; ++i) {
                Side side = (i % 2 == 0) ? Side::Buy : Side::Sell;
                PriceTicks price = (side == Side::Buy) ? 9900 : 10100;

                auto ack = engine.process_order(
                    make_order(
                        "AAPL",
                        side,
                        OrderType::Limit,
                        price,
                        1,
                        "bench"
                    )
                );

                if (!ack.accepted) {
                    throw std::runtime_error("Order rejected unexpectedly");
                }
            }

            return 0;
        }
    );

    return result;
}

BenchmarkResult bench_single_level_crosses(std::size_t n) {
    MatchingEngine engine;

    // Seed resting asks outside timed section.
    for (std::size_t i = 0; i < n; ++i) {
        engine.process_order(
            make_order(
                "AAPL",
                Side::Sell,
                OrderType::Limit,
                10000,
                1,
                "maker"
            )
        );
    }

    auto result = run_benchmark(
        "Single-level crosses",
        n,
        [&engine, n]() -> std::size_t {
            std::size_t trade_count = 0;

            for (std::size_t i = 0; i < n; ++i) {
                auto ack = engine.process_order(
                    make_order(
                        "AAPL",
                        Side::Buy,
                        OrderType::Market,
                        std::nullopt,
                        1,
                        "taker"
                    )
                );

                trade_count += ack.trades.size();
            }

            return trade_count;
        }
    );

    return result;
}

BenchmarkResult bench_market_orders_walk_levels(
    std::size_t n_market_orders,
    std::size_t levels_per_order
) {
    MatchingEngine engine;

    std::size_t total_resting_orders = n_market_orders * levels_per_order;

    // Seed asks outside timed section.
    for (std::size_t i = 0; i < total_resting_orders; ++i) {
        PriceTicks price = 10000 + static_cast<PriceTicks>(i % levels_per_order);

        engine.process_order(
            make_order(
                "AAPL",
                Side::Sell,
                OrderType::Limit,
                price,
                1,
                "maker"
            )
        );
    }

    auto result = run_benchmark(
        "Market orders walk 10 levels",
        n_market_orders,
        [&engine, n_market_orders, levels_per_order]() -> std::size_t {
            std::size_t trade_count = 0;

            for (std::size_t i = 0; i < n_market_orders; ++i) {
                auto ack = engine.process_order(
                    make_order(
                        "AAPL",
                        Side::Buy,
                        OrderType::Market,
                        std::nullopt,
                        static_cast<Quantity>(levels_per_order),
                        "taker"
                    )
                );

                trade_count += ack.trades.size();
            }

            return trade_count;
        }
    );

    return result;
}

BenchmarkResult bench_mixed_steady_state(std::size_t n) {
    MatchingEngine engine;

    constexpr PriceTicks bid_price = 9999;
    constexpr PriceTicks ask_price = 10001;

    constexpr std::size_t seed_liquidity = 10000;

    // Seed outside timed section.
    for (std::size_t i = 0; i < seed_liquidity; ++i) {
        engine.process_order(
            make_order(
                "AAPL",
                Side::Buy,
                OrderType::Limit,
                bid_price,
                1,
                "seed"
            )
        );

        engine.process_order(
            make_order(
                "AAPL",
                Side::Sell,
                OrderType::Limit,
                ask_price,
                1,
                "seed"
            )
        );
    }

    auto result = run_benchmark(
        "Mixed steady-state",
        n,
        [&engine, n]() -> std::size_t {
            std::size_t trade_count = 0;

            for (std::size_t i = 0; i < n; ++i) {
                Order order = [&]() {
                    switch (i % 4) {
                        case 0:
                            return make_order(
                                "AAPL",
                                Side::Buy,
                                OrderType::Market,
                                std::nullopt,
                                1,
                                "taker"
                            );

                        case 1:
                            return make_order(
                                "AAPL",
                                Side::Sell,
                                OrderType::Limit,
                                10001,
                                1,
                                "maker"
                            );

                        case 2:
                            return make_order(
                                "AAPL",
                                Side::Sell,
                                OrderType::Market,
                                std::nullopt,
                                1,
                                "taker"
                            );

                        default:
                            return make_order(
                                "AAPL",
                                Side::Buy,
                                OrderType::Limit,
                                9999,
                                1,
                                "maker"
                            );
                    }
                }();

                auto ack = engine.process_order(std::move(order));
                trade_count += ack.trades.size();
            }

            return trade_count;
        }
    );

    return result;
}

void print_results(const std::vector<BenchmarkResult>& results) {
    std::cout << "\n";

    std::cout << std::left << std::setw(34) << "Benchmark"
              << std::right << std::setw(14) << "Orders"
              << std::setw(14) << "Trades"
              << std::setw(12) << "Seconds"
              << std::setw(18) << "Orders/sec"
              << std::setw(18) << "Trades/sec"
              << "\n";

    std::cout << std::string(110, '-') << "\n";

    for (const auto& result : results) {
        std::cout << std::left << std::setw(34) << result.name
                  << std::right << std::setw(14) << result.orders
                  << std::setw(14) << result.trades
                  << std::setw(12) << std::fixed << std::setprecision(3) << result.seconds
                  << std::setw(18) << std::fixed << std::setprecision(0) << result.orders_per_sec()
                  << std::setw(18) << std::fixed << std::setprecision(0) << result.trades_per_sec()
                  << "\n";
    }

    std::cout << "\n";
}

int main() {
    constexpr std::size_t n = 10'000'000;

    try {
        std::vector<BenchmarkResult> results;

        results.push_back(bench_resting_limit_adds(n));
        results.push_back(bench_single_level_crosses(n));

        results.push_back(
            bench_market_orders_walk_levels(
                n / 10,
                10
            )
        );

        results.push_back(bench_mixed_steady_state(n));

        print_results(results);

    } catch (const std::exception& e) {
        std::cerr << "Benchmark failed: " << e.what() << "\n";
        return 1;
    }

    return 0;
}