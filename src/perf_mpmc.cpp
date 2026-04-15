#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <numeric>
#include <immintrin.h>
#include "mpmc_queue.hpp"

const size_t TOTAL_OPERATIONS = 10000000; 
const int PRODUCERS = 4;               
const int CONSUMERS = 4;          
const size_t QUEUE_CAPACITY = 65536;   

void print_result(double duration_sec, size_t count) {
    std::cout << "------------------------------------\n";
    std::cout << "Test Environment:";
    std::cout << "  Producers: " << PRODUCERS << " | Consumers: " << CONSUMERS << '\n';
    std::cout << "  Total Operations: " << count << '\n';
    std::cout << "Result:\n";
    std::cout << "  Time elapsed: " << duration_sec << " secs\n";
    std::cout << "  Throughput: " << (count / duration_sec) / 1e6 << " Mops/s\n";
    std::cout << "------------------------------------\n";
}

int main() {
    MPMCQueue<size_t, QUEUE_CAPACITY> queue;
    std::atomic<bool> start_flag{false};
    std::atomic<int> ready_count{0};

    auto producer_func = [&]() {
        ready_count++;
        while (!start_flag.load(std::memory_order_relaxed)) _mm_pause();

        for (size_t i = 0; i < TOTAL_OPERATIONS / PRODUCERS; ++i) {
            while (!queue.push(i)) {
                _mm_pause();
            }
        }
    };

    auto consumer_func = [&]() {
        ready_count++;
        while (!start_flag.load(std::memory_order_relaxed)) _mm_pause();

        size_t val;
        for (size_t i = 0; i < TOTAL_OPERATIONS / CONSUMERS; ++i) {
            while (!queue.pop(val)) {
                _mm_pause();
            }
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < PRODUCERS; ++i) threads.emplace_back(producer_func);
    for (int i = 0; i < CONSUMERS; ++i) threads.emplace_back(consumer_func);

    while (ready_count.load() < (PRODUCERS + CONSUMERS)) _mm_pause();

    auto start = std::chrono::high_resolution_clock::now();
    
    start_flag.store(true);

    for (auto& t : threads) t.join();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;

    print_result(diff.count(), TOTAL_OPERATIONS);

    return 0;
}