// common.hpp
#ifndef COMMON_H
#define COMMON_H

#include <atomic>
#include <cstdint>
#include <sys/ipc.h>

// --- Configuration ---
constexpr std::size_t ELEMENT_SIZE   = 8 * 1024 * 1024; // 8MB per element
constexpr std::size_t QUEUE_CAPACITY = 2;               // 2 slots in the queue

// --- IPC Keys and Names ---
const key_t   SHM_KEY             = ftok("shmfile", 65);
const char*   SEM_INIT_READY_NAME = "/my_app_init_sem";    // Signal consumer to start
const char*   SEM_FILLED_NAME     = "/my_app_filled_sem";  // Counts filled slots
const char*   SEM_EMPTY_NAME      = "/my_app_empty_sem";   // Counts empty slots

// Graceful shutdown semaphores
const char*   SEM_CONSUMED_NAME   = "/my_app_consumed_sem"; // Consumer done
const char*   SEM_SHUTDOWN_NAME   = "/my_app_shutdown_sem"; // Producer cleanup done

// --- Shared Data Structures ---
struct QueueSlot {
    char data[ELEMENT_SIZE];
};

struct SharedQueue {
    std::atomic<uint32_t> head;               // Next write index
    std::atomic<uint32_t> tail;               // Next read index
    std::atomic<bool>     producer_finished;  // Flag flipped when producer is done
    QueueSlot slots[QUEUE_CAPACITY];
};

constexpr std::size_t SHM_SIZE = sizeof(SharedQueue);

#endif // COMMON_H
