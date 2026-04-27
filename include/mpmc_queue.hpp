#include <atomic>
#include <vector>
#include <stdexcept>
#include <iterator>
#include <immintrin.h>

template <typename T, size_t Capacity>
class MPMCQueue {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of 2");

public:
    MPMCQueue() {
        for (size_t i = 0; i < Capacity; ++i) {
            buffer_[i].sequence.store(i, std::memory_order_relaxed);
        }
    }

    template <typename Iterator>
    size_t push_batch(Iterator begin, Iterator end) {
        size_t n = std::distance(begin, end);
        if (n == 0) return 0;
        if (n > Capacity) return 0;

        size_t pos = tail_.load(std::memory_order_relaxed);

        while (true) {
            size_t head = head_.load(std::memory_order_acquire);
            
            if (pos + n > head + Capacity) {
                return 0; 
            }

            if (tail_.compare_exchange_weak(pos, pos + n, std::memory_order_relaxed)) {
                break;
            }
            
            _mm_pause();
        }

        for (size_t i = 0; i < n; ++i) {
            size_t actual_pos = pos + i;
            Slot* slot = &buffer_[actual_pos & (Capacity - 1)];

            while (slot->sequence.load(std::memory_order_acquire) != actual_pos) {
                _mm_pause();
            }

            slot -> data = std::move(*begin);
            ++begin;

            slot -> sequence.store(actual_pos + 1, std::memory_order_release);
        }

        return n;
    }

    template <typename... Args>
    bool emplace(Args&&... args){
        Slot* slot;
        size_t pos = tail_.load(std::memory_order_relaxed);

        while (true) {
            slot = &buffer_[pos & (Capacity - 1)];
            size_t seq = slot -> sequence.load(std::memory_order_acquire);
            intptr_t diff = (intptr_t)seq - (intptr_t)pos;

            if (diff == 0) {
                if (tail_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
                    break;
                }
                _mm_pause();
            } else if (diff < 0) {
                return false;
            } else {
                pos = tail_.load(std::memory_order_relaxed);
            }
        }

        slot -> data = T(std::forward<Args>(args)...); 
    
        slot -> sequence.store(pos + 1, std::memory_order_release);
        return true;
    }

    template <typename U>
    bool push(U&& data) {
        Slot* slot;
        size_t pos = tail_.load(std::memory_order_relaxed);

        while (true) {
            slot = &buffer_[pos & (Capacity - 1)];
            size_t seq = slot -> sequence.load(std::memory_order_acquire);
            
            intptr_t diff = (intptr_t)seq - (intptr_t)pos;

            if (diff == 0) {
                if (tail_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
                    break;
                }
                _mm_pause();
            } else if (diff < 0) {
                return false; 
            } else {
                _mm_pause();
                pos = tail_.load(std::memory_order_relaxed);
            }
        }

        slot -> data = std::forward<U>(data);
        slot -> sequence.store(pos + 1, std::memory_order_release);
        
        return true;
    }

    bool pop(T& data) {
        Slot* slot;
        size_t pos = head_.load(std::memory_order_relaxed);

        while(true){
            slot = &buffer_[pos & (Capacity - 1)];

            size_t seq = slot -> sequence.load(std::memory_order_acquire);

            intptr_t diff = (intptr_t)seq - (intptr_t)(pos + 1);

            if(diff == 0){
                if(head_.compare_exchange_weak(pos,pos + 1,std::memory_order_relaxed)){
                    break;
                }
                _mm_pause();
            } else if(diff < 0){
                return false;
            } else {
                _mm_pause();
                pos = head_.load(std::memory_order_acquire);
            }
        }

        data = std::move(slot -> data);

        slot -> sequence.store(pos + Capacity,std::memory_order_release);

        return true;
    }

    template <typename Container>
    size_t pop_batch(Container& items, size_t n) {
        if (n == 0) return 0;
        if (n > Capacity) n = Capacity;

        size_t pos = head_.load(std::memory_order_relaxed);

        while (true) {
            size_t tail = tail_.load(std::memory_order_acquire);
            if (pos + n > tail) {
                size_t available = (tail > pos) ? (tail - pos) : 0;
                if (available == 0) return 0;
                n = available;
            }

            if (head_.compare_exchange_weak(pos, pos + n, std::memory_order_relaxed)) {
                break;
            }
            _mm_pause();
        }

        for (size_t i = 0; i < n; ++i) {
            size_t actual_pos = pos + i;
            Slot* slot = &buffer_[actual_pos & (Capacity - 1)];

            while (slot->sequence.load(std::memory_order_acquire) != actual_pos + 1) {
                _mm_pause();
            }

            items.push_back(std::move(slot->data));

            slot->sequence.store(actual_pos + Capacity, std::memory_order_release);
        }

        return n;
    }

private:
    struct Slot {
        std::atomic<size_t> sequence;
        T data;
    };

    Slot buffer_[Capacity]; 
    alignas(64) std::atomic<size_t> head_{0};
    alignas(64) std::atomic<size_t> tail_{0};
};