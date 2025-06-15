#ifndef COMMON_H
#define COMMON_H

#include <atomic>
#include <cstdint>
#include <sys/ipc.h>

// --- Compile‑time switches (0=off, 1=on) ---
#ifndef USE_PRETOUCH
#define USE_PRETOUCH   1   // Touch each page to fault it in
#endif

#ifndef USE_MLOCK
#define USE_MLOCK      1   // Lock pages into RAM (mlock)
#endif

#ifndef USE_MADV
#define USE_MADV       1   // Advise kernel to prefetch pages (MADV_WILLNEED)
#endif

#ifndef USE_HUGEPAGES
#define USE_HUGEPAGES  1  // Allocate shared memory with huge pages
#endif

// --- Configuration ---
constexpr std::size_t MEGABYTE          = 1024 * 1024;
// NOTE: HUGE_PAGES only has an effect if each element is at least 16MB
constexpr std::size_t ELEMENT_SIZE      = 16 * MEGABYTE; // per element
constexpr std::size_t ELEMENT_SIZE_MB   = ELEMENT_SIZE / MEGABYTE; // per element

constexpr std::size_t QUEUE_CAPACITY = 10;             // Number of slots available in SHM Queue

// --- IPC Keys and Names ---
const key_t   SHM_KEY             = ftok("shmfile", 65);
const char*   SEM_INIT_READY_NAME = "/my_app_init_sem";
const char*   SEM_FILLED_NAME     = "/my_app_filled_sem";
const char*   SEM_EMPTY_NAME      = "/my_app_empty_sem";
const char*   SEM_CONSUMED_NAME   = "/my_app_consumed_sem";
const char*   SEM_SHUTDOWN_NAME   = "/my_app_shutdown_sem";

// --- Shared Data Structures ---
struct QueueSlot {
    char data[ELEMENT_SIZE];
};

struct SharedQueue {
    std::atomic<uint32_t> head;
    std::atomic<uint32_t> tail;
    std::atomic<bool>     producer_finished;
    QueueSlot slots[QUEUE_CAPACITY];
};

constexpr std::size_t SHM_SIZE = sizeof(SharedQueue);

#endif // COMMON_H
