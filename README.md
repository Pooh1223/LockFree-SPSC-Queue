# LockFree-Queue

A high-performance, header-only, lock-free concurrent queue library implemented in modern C++. This project provides optimized components for low-latency and high-throughput data communication between threads, tailored for modern multi-core architectures.

## 🌟 Key Features

  * **SPSCQueue**: A Lock-free Single-Producer Single-Consumer queue designed for extreme low-latency scenarios.
  * **MPMCQueue**: A Lock-free Multi-Producer Multi-Consumer queue based on Dmitry Vyukov's bounded queue algorithm, optimized for high contention.
  * **Cache-Line Optimized**: Prevents **False Sharing** by utilizing `alignas(64)` for core synchronization primitives (Head/Tail).
  * **Modern C++ API**: Full support for **Move Semantics** and **Perfect Forwarding** (`emplace`) to eliminate unnecessary data copies.
  * **Stability**: Verified with **Google Test** (GTest) for correctness and **ThreadSanitizer (TSan)** for rigorous data race detection.
  * **Robust Indexing**: Implements absolute indexing with $2^n$ capacity to handle unsigned integer wrap-around without logic breaks.

-----

## 🛠 Technical point

### 1\. Memory Ordering (Acquire-Release)

To maximize throughput, this library avoids the heavy overhead of default `std::memory_order_seq_cst`. Instead, it uses precise **Acquire-Release semantics**:

  * **Store-Release**: Ensures all data writes are visible to other threads before the index (Head/Tail) is updated.
  * **Load-Acquire**: Ensures the thread observes the most recent data writes once it sees an index update.
  * **Relaxed Order**: Applied to thread-local index increments to minimize hardware cache-coherency traffic.

### 2\. Slot-Level Synchronization (MPMC)

In high-contention MPMC scenarios, global pointers alone aren't enough to prevent reading stale data. Each slot in `MPMCQueue` features an `atomic<size_t> sequence`. This acts as a fine-grained synchronization guard, ensuring that a producer or consumer only accesses a slot when its sequence matches the expected "ticket number."

### 3\. False Sharing Mitigation

Modern CPUs operate on 64-byte cache lines. If the `Head` and `Tail` pointers share a cache line, a write to one invalidates the other in different core caches (Cache Line Ping-pong). We use `alignas(64)` to ensure these hot variables reside in separate cache lines.

-----

## 🚀 Performance Benchmarks

**Environment:**

  * **CPU**: AMD Ryzen 7 7800X3D (8-Core, 16-Thread)
  * **OS**: WSL2 (Ubuntu 22.04)
  * **Compiler**: GCC 11.4 (-O3 Optimization)

| Queue Type | Configuration (P/C) | Throughput (Mops/s) |
| :--- | :--- | :--- |
| **SPSC** | 1 Producer / 1 Consumer | **\~110.2 Mops/s** |
| **MPMC** | 4 Producers / 4 Consumers | **\~4.2 Mops/s** |

### **Analysis: The Contention Bottleneck**

The SPSC queue reaches near-hardware theoretical limits because synchronization is unidirectional. In contrast, the MPMC queue's performance is limited by **Cache Line Bouncing**. When multiple cores perform a `compare_exchange` (CAS) on the same global `tail_`, the **MESI protocol** triggers constant cross-core cache invalidations, creating a physical synchronization delay of approximately 40-60ns per operation.

-----

## 📦 Getting Started

### Prerequisites

  * C++17 compliant compiler (GCC 9+, Clang 10+, MSVC 2019+)
  * CMake 3.14+

### Build and Run Tests

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make

# Run correctness stress tests
./unit_tests

# Run performance benchmarks
./mpmc_perf
```

## 🏗 Future Work: Batching

- [ ] Apply Batching in MPMC
