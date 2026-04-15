#include <iostream>
#include <cassert>
#include "spsc_queue.hpp"

int main() {
    SPSCQueue<int, 4> queue;

    assert(queue.push(1));
    assert(queue.push(2));
    assert(queue.push(3));
    assert(queue.push(4)); 
    
    assert(queue.push(5) == false);

    std::cout << "Absolute indexing test passed! (Zero slots wasted)" << std::endl;
    return 0;
}