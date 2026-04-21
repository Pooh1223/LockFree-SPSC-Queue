#include <gtest/gtest.h>
#include <thread>
#include <set>
#include <string>
#include "mpmc_queue.hpp"

struct ComplexItem {
    int id;
    std::string name;

    ComplexItem() : id(0), name("") {}

    ComplexItem(int i, std::string n) : id(i), name(std::move(n)) {}
};

TEST(MPMCQueueTest, EmplaceTest) {
    MPMCQueue<ComplexItem, 16> queue;

    EXPECT_TRUE(queue.emplace(99, "LEOLIN"));

    ComplexItem result;
    EXPECT_TRUE(queue.pop(result));
    
    EXPECT_EQ(result.id, 99);
    EXPECT_EQ(result.name, "LEOLIN");
}

TEST(MPMCQueueTest, ConcurrentPushPop) {
    MPMCQueue<int, 1024> queue;
    const int num_threads = 4;
    const int ops_per_thread = 10000;
    std::atomic<int> total_received{0};

    std::vector<std::thread> producers;
    for (int i = 0; i < num_threads; ++i) {
        producers.emplace_back([&]() {
            for (int j = 0; j < ops_per_thread; ++j) {
                while (!queue.push(j)) std::this_thread::yield();
            }
        });
    }

    std::vector<std::thread> consumers;
    for (int i = 0; i < num_threads; ++i) {
        consumers.emplace_back([&]() {
            int val;
            for (int j = 0; j < ops_per_thread; ++j) {
                while (!queue.pop(val)) std::this_thread::yield();
                total_received++;
            }
        });
    }

    for (auto& t : producers) t.join();
    for (auto& t : consumers) t.join();

    EXPECT_EQ(total_received.load(), num_threads * ops_per_thread);
}