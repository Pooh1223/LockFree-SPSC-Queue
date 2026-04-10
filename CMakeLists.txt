# 1. 規定 CMake 的最低版本要求
cmake_minimum_required(VERSION 3.10)

# 2. 定義專案名稱
project(SPSC_RingBuffer)

# 3. 指定 C++ 標準 (你剛才用了 C++20，我們就設定 20)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 4. 【關鍵！】告訴編譯器：去 include 資料夾找標頭檔 (.hpp)
# 這行就相當於手動編譯時的 -I ../include
include_directories(include)

# 5. 尋找系統的執行緒函式庫 (Thread library)
# 因為 SPSC 最終會跑在多執行緒環境，這行是必須的
find_package(Threads REQUIRED)

# 6. 定義要產出的執行檔名稱，以及它的來源原始碼
add_executable(spsc_test src/main.cpp)

# 7. 連結執行緒函式庫到你的執行檔
target_link_libraries(spsc_test PRIVATE Threads::Threads)