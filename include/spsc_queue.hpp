#include <vector>
#include <atomic>

template <typename T, size_t Capacity>
class SPSCQueue {
    // 1. 在這裡加入 static_assert 確保 Capacity 是 2 的冪次方
    // 2. 加入私人成員變數的定義
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of 2");

public:
    SPSCQueue() : data_(Capacity),head(0),tail(0){};
    
    // producer
    bool push(const T& val){
        size_t cur_tail = tail.load(std::memory_order_relaxed);
        size_t cur_head = head.load(std::memory_order_acquire);

        // size_t cur_tail = tail.load();
        // size_t cur_head = head.load();

        // ring array is full
        if(cur_tail - cur_head == Capacity){
            return false;
        }

        data_[cur_tail & (Capacity - 1)] = val;
        
        size_t next_tail = (cur_tail + 1);
        // tail.store(next_tail);
        tail.store(next_tail,std::memory_order_release);
        return true;
    }

    // consumer
    bool pop(T& val){
        size_t cur_head = head.load(std::memory_order_relaxed);
        size_t cur_tail = tail.load(std::memory_order_acquire);

        // size_t cur_head = head.load();
        // size_t cur_tail = tail.load();
        
        // array is empty
        if(cur_head == cur_tail){
            return false;
        }

        val = data_[cur_head & (Capacity - 1)];

        size_t next_head = (cur_head + 1);
        head.store(next_head,std::memory_order_release);
        // head.store(next_head);
        return true;
    }

private:
    std::vector<T> data_;
    alignas(64) std::atomic<size_t> head;
    alignas(64) std::atomic<size_t> tail;

};