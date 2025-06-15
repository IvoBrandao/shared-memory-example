cmake_minimum_required(VERSION 3.10)
project(IPCQueueApp LANGUAGES CXX)

# -- C++ Standard --
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -- Include directories --
include_directories(${CMAKE_SOURCE_DIR}/include)

# -- Source files --
set(SRC_DIR ${CMAKE_SOURCE_DIR}/source)

# Producer executable
add_executable(producer
    ${SRC_DIR}/producer.cpp
)

# Consumer executable
add_executable(consumer
    ${SRC_DIR}/consumer.cpp
)

# Cleanup executable
add_executable(cleanup_ipc
    ${SRC_DIR}/cleanup.cpp
)

# -- Link libraries --
# POSIX real-time extensions for semaphores, and pthread for threading
find_package(Threads REQUIRED)

target_link_libraries(producer PRIVATE rt)

target_link_libraries(consumer PRIVATE rt Threads::Threads)

# cleanup_ipc uses only sem_unlink and shmctl
target_link_libraries(cleanup_ipc PRIVATE rt)

# -- Installation (optional) --
install(TARGETS producer consumer cleanup_ipc
    RUNTIME DESTINATION bin
)
