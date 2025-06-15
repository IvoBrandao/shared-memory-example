#include "common.hpp"
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <semaphore.h>
#include <sys/shm.h>
#include <thread>
#include <vector>

// --- Step 1: Async Initialization ---
bool wait_init_ready_async(SharedQueue *&shared_queue, int retry_interval_ms = 200, int max_retries = -1)
{
    for (int attempt = 0; attempt < max_retries || max_retries == -1; ++attempt)
    {
        sem_t *sem_init = sem_open(SEM_INIT_READY_NAME, 0);
        if (sem_init != SEM_FAILED)
        {
            std::cout << "Consumer: Init semaphore found. Waiting for signal...\n";
            sem_wait(sem_init);
            sem_close(sem_init);

            int shmid = shmget(SHM_KEY, SHM_SIZE, 0666);
            if (shmid != -1)
            {
                void *addr = shmat(shmid, nullptr, 0);
                if (addr != (void *)-1)
                {
                    shared_queue = reinterpret_cast<SharedQueue *>(addr);
                    std::cout << "Consumer: Attached to shared memory.\n";
                    return true;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(retry_interval_ms));
    }
    std::cerr << "Consumer: Initialization failed after retries.\n";
    return false;
}

// --- Step 2: Attach Semaphores ---
bool attach_semaphores(sem_t *&sem_filled, sem_t *&sem_empty, sem_t *&sem_consumed, sem_t *&sem_shutdown)
{
    sem_filled = sem_open(SEM_FILLED_NAME, 0);
    sem_empty = sem_open(SEM_EMPTY_NAME, 0);
    sem_consumed = sem_open(SEM_CONSUMED_NAME, 0);
    sem_shutdown = sem_open(SEM_SHUTDOWN_NAME, 0);
    if (sem_filled == SEM_FAILED || sem_empty == SEM_FAILED || sem_consumed == SEM_FAILED || sem_shutdown == SEM_FAILED)
    {
        perror("sem_open");
        return false;
    }
    std::cout << "Consumer: Semaphores opened successfully.\n";
    return true;
}

// --- Worker Thread ---
void process_data(SharedQueue *queue, uint32_t idx, sem_t *sem_empty)
{
    std::vector<char> buf(ELEMENT_SIZE);
    auto start = std::chrono::high_resolution_clock::now();
    std::memcpy(buf.data(), queue->slots[idx].data, ELEMENT_SIZE);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> d = end - start;

    std::cout << "[Thread " << std::this_thread::get_id() << "] Read slot " << idx << " in " << d.count() << " ms\n";

    sem_post(sem_empty);
    std::cout << "[Thread " << std::this_thread::get_id() << "] Marked slot " << idx << " empty\n";
}

// --- Step 3: Consume Until Producer Finishes ---
void consume_items(SharedQueue *queue, sem_t *sem_filled, sem_t *sem_empty, std::vector<std::thread> &workers)
{
    while (true)
    {
        if (!queue->producer_finished.load(std::memory_order_acquire))
        {
            sem_wait(sem_filled);
        }
        else
        {
            if (sem_trywait(sem_filled) != 0)
            {
                break;
            }
        }

        uint32_t idx = queue->tail.load(std::memory_order_relaxed);
        queue->tail.store((idx + 1) % QUEUE_CAPACITY, std::memory_order_release);

        workers.emplace_back(process_data, queue, idx, sem_empty);
    }
}

// --- Step 4: Cleanup ---
void cleanup(SharedQueue *queue, sem_t *sem_filled, sem_t *sem_empty, sem_t *sem_consumed, sem_t *sem_shutdown)
{
    shmdt(queue);
    sem_close(sem_filled);
    sem_close(sem_empty);
    sem_close(sem_consumed);
    sem_close(sem_shutdown);
    std::cout << "Consumer: IPC cleanup complete.\n";
}

int main()
{
    SharedQueue *queue = nullptr;
    if (!wait_init_ready_async(queue))
        return 1;

    sem_t *sem_filled, *sem_empty, *sem_consumed, *sem_shutdown;
    if (!attach_semaphores(sem_filled, sem_empty, sem_consumed, sem_shutdown))
    {
        shmdt(queue);
        return 1;
    }

    std::vector<std::thread> workers;
    consume_items(queue, sem_filled, sem_empty, workers);

    std::cout << "Consumer: Joining worker threads...\n";
    for (auto &t : workers)
    {
        if (t.joinable())
            t.join();
    }

    std::cout << "Consumer: All items processed, signaling producer...\n";
    sem_post(sem_consumed);

    sem_wait(sem_shutdown);
    cleanup(queue, sem_filled, sem_empty, sem_consumed, sem_shutdown);

    std::cout << "Consumer: Exiting.\n";
    return 0;
}
