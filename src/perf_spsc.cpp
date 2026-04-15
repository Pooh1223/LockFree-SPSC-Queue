#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include "spsc_queue.hpp"

const size_t COUNT = 100000000;

int main() {
    SPSCQueue<size_t, 65536> queue;
    auto start = std::chrono::high_resolution_clock::now();

    std::thread producer([&]() {
        for (size_t i = 0; i < COUNT; ++i) {
            while (!queue.push(i)) {

            }
        }
    });

    std::thread consumer([&]() {
        for (size_t i = 0; i < COUNT; ++i) {
            size_t val;
            while (!queue.pop(val)) {
                
            }
        }
    });

    producer.join();
    consumer.join();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;

    std::cout << COUNT << " data needs : " << diff.count() << " secs\n";
    std::cout << "Average data transfer per second : " << (COUNT / diff.count()) / 1e6 << " (Mops/s)\n";

    return 0;
}