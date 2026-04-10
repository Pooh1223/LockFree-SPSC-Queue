#include <iostream>
#include <cassert>
#include "spsc_ring_buffer.hpp"

int main() {
    // 建立一個容量為 4 的 Queue (傳統版實際上只能放 3 個)
    SPSCQueue<int, 4> queue;

    // 1. 測試連續放入資料
    assert(queue.push(1) == true);
    assert(queue.push(2) == true);
    assert(queue.push(3) == true);
    
    // 2. 測試全滿 (因為要留一格，第四個應該會失敗)
    assert(queue.push(4) == false);
    std::cout << "Full-check passed (1 slot wasted as expected)." << std::endl;

    // 3. 測試取出資料
    int val;
    assert(queue.pop(val) == true && val == 1);
    assert(queue.pop(val) == true && val == 2);
    assert(queue.pop(val) == true && val == 3);

    // 4. 測試全空
    assert(queue.pop(val) == false);
    std::cout << "Empty-check passed." << std::endl;

    std::cout << "All basic tests passed!" << std::endl;
    return 0;
}