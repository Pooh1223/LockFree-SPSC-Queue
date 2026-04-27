#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <numeric>
#include <functional>
#include <immintrin.h>
#include "mpmc_queue.hpp"

const size_t TOTAL_OPERATIONS = 10000000; 
const int PRODUCERS = 4;               
const int CONSUMERS = 4;          
const size_t QUEUE_CAPACITY = 65536;   
const size_t BATCH_SIZE = 8;

void print_result(const std::string& test_name, double duration_sec, size_t count) {
    std::cout << "==== Test: " << test_name << " ====\n";
    std::cout << "  Result:\n";
    std::cout << "    Time elapsed: " << duration_sec << " secs\n";
    std::cout << "    Throughput:   " << (count / duration_sec) / 1e6 << " Mops/s\n\n";
}

using QueueType = MPMCQueue<size_t, QUEUE_CAPACITY>;
using LogicType = std::function<void(QueueType&, std::atomic<bool>&, std::atomic<int>&)>;

void run_benchmark(const std::string& test_name, 
                   LogicType producer_logic, 
                   LogicType consumer_logic) {
    
    QueueType queue;
    std::atomic<bool> start_flag{false};
    std::atomic<int> ready_count{0};
    std::vector<std::thread> threads;

    for (int i = 0; i < PRODUCERS; ++i) {
        threads.emplace_back(producer_logic, std::ref(queue), std::ref(start_flag), std::ref(ready_count));
    }

    for (int i = 0; i < CONSUMERS; ++i) {
        threads.emplace_back(consumer_logic, std::ref(queue), std::ref(start_flag), std::ref(ready_count));
    }

    while (ready_count.load() < (PRODUCERS + CONSUMERS)) _mm_pause();

    auto start = std::chrono::high_resolution_clock::now();
    
    start_flag.store(true);

    for (auto& t : threads) t.join();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;

    print_result(test_name, diff.count(), TOTAL_OPERATIONS);
}

int main() {
    auto single_push = [](QueueType& q, std::atomic<bool>& start, std::atomic<int>& ready) {
        ready++;
        while (!start.load(std::memory_order_relaxed)) _mm_pause();
        for (size_t i = 0; i < TOTAL_OPERATIONS / PRODUCERS; ++i) {
            while (!q.push(i)) _mm_pause();
        }
    };

    auto single_pop = [](QueueType& q, std::atomic<bool>& start, std::atomic<int>& ready) {
        ready++;
        while (!start.load(std::memory_order_relaxed)) _mm_pause();
        size_t val;
        for (size_t i = 0; i < TOTAL_OPERATIONS / CONSUMERS; ++i) {
            while (!q.pop(val)) _mm_pause();
        }
    };

    auto batch_push = [](QueueType& q, std::atomic<bool>& start, std::atomic<int>& ready) {
        ready++;
        while (!start.load(std::memory_order_relaxed)) _mm_pause();
        std::vector<size_t> batch(BATCH_SIZE, 42);
        for (size_t i = 0; i < TOTAL_OPERATIONS / PRODUCERS; i += BATCH_SIZE) {
            while (q.push_batch(batch.begin(), batch.end()) == 0) _mm_pause();
        }
    };

    auto batch_pop = [](QueueType& q, std::atomic<bool>& start, std::atomic<int>& ready) {
        ready++;
        while (!start.load(std::memory_order_relaxed)) _mm_pause();
        std::vector<size_t> buffer;
        buffer.reserve(BATCH_SIZE);
        for (size_t i = 0; i < TOTAL_OPERATIONS / CONSUMERS; i += BATCH_SIZE) {
            buffer.clear();
            while (q.pop_batch(buffer, BATCH_SIZE) == 0) _mm_pause();
        }
    };

    run_benchmark("Standard MPMC (Single-Single)", single_push, single_pop);

    run_benchmark("Batch-to-Single (P-Batch)", batch_push, single_pop);

    run_benchmark("Full Batch MPMC (Batch-Batch)", batch_push, batch_pop);

    return 0;
}