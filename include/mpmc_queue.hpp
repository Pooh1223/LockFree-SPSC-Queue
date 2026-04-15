#include <atomic>
#include <vector>
#include <stdexcept>

template <typename T, size_t Capacity>
class MPMCQueue {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of 2");

public:
    MPMCQueue() {
        for (size_t i = 0; i < Capacity; ++i) {
            buffer_[i].sequence.store(i, std::memory_order_relaxed);
        }
    }

    bool push(const T& data) {
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
            } 
            else if (diff < 0) {
                return false; 
            } else {
                pos = tail_.load(std::memory_order_relaxed);
            }
        }

        slot -> data = data;
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

            } else if(diff < 0){
                return false;
            } else {
                pos = head_.load(std::memory_order_acquire);
            }
        }

        data = slot -> data;

        slot -> sequence.store(pos + Capacity,std::memory_order_release);

        return true;
    }

private:
    struct Slot {
        std::atomic<size_t> sequence;
        T data;
    };

    alignas(64) Slot buffer_[Capacity]; 
    alignas(64) std::atomic<size_t> head_{0};
    alignas(64) std::atomic<size_t> tail_{0};
};