// producer.cpp
#include "common.hpp"
#include <iostream>
#include <cstring>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <unistd.h>
#include <chrono>

bool init_ipc(sem_t*& sem_init, sem_t*& sem_filled, sem_t*& sem_empty,
              sem_t*& sem_consumed, sem_t*& sem_shutdown) {
    sem_init     = sem_open(SEM_INIT_READY_NAME, O_CREAT, 0666, 0);
    sem_filled   = sem_open(SEM_FILLED_NAME,    O_CREAT, 0666, 0);
    sem_empty    = sem_open(SEM_EMPTY_NAME,     O_CREAT, 0666, QUEUE_CAPACITY);
    sem_consumed = sem_open(SEM_CONSUMED_NAME,  O_CREAT, 0666, 0);
    sem_shutdown = sem_open(SEM_SHUTDOWN_NAME,  O_CREAT, 0666, 0);

    if (sem_init == SEM_FAILED || sem_filled == SEM_FAILED ||
        sem_empty == SEM_FAILED || sem_consumed == SEM_FAILED ||
        sem_shutdown == SEM_FAILED) {
        perror("sem_open");
        return false;
    }
    std::cout << "Producer: IPC semaphores initialized.\n";
    return true;
}

SharedQueue* init_shm(int& shmid) {
    shmid = shmget(SHM_KEY, SHM_SIZE, IPC_CREAT | 0666);
    if (shmid < 0) { perror("shmget"); return nullptr; }
    void* ptr = shmat(shmid, nullptr, 0);
    if (ptr == (void*)-1) { perror("shmat"); return nullptr; }

    auto* queue = static_cast<SharedQueue*>(ptr);
    queue->head.store(0);
    queue->tail.store(0);
    queue->producer_finished.store(false);
    std::cout << "Producer: Shared memory initialized.\n";
    return queue;
}

void produce(SharedQueue* queue, sem_t* sem_filled, sem_t* sem_empty, int count) {
    for (int i = 0; i < count; ++i) {
        sem_wait(sem_empty);
        uint32_t idx = queue->head.load(std::memory_order_relaxed);
        std::cout << "Producer: Writing item " << (i + 1)
                  << " to slot " << idx << "\n";

        auto start = std::chrono::high_resolution_clock::now();
        std::memset(queue->slots[idx].data, 'A' + i, ELEMENT_SIZE);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> d = end - start;
        std::cout << "Producer: Wrote 8MB in " << d.count() << " ms.\n";

        queue->head.store((idx + 1) % QUEUE_CAPACITY, std::memory_order_release);
        sem_post(sem_filled);
        sleep(1);
    }
    // Signal no more items, then wake consumer
    queue->producer_finished.store(true, std::memory_order_release);
    sem_post(sem_filled);
}

void cleanup(int shmid, SharedQueue* queue,
             sem_t* sem_init, sem_t* sem_filled, sem_t* sem_empty,
             sem_t* sem_consumed, sem_t* sem_shutdown) {
    // Detach & remove shared memory
    shmdt(queue);
    shmctl(shmid, IPC_RMID, nullptr);

    // Close semaphores
    sem_close(sem_init);
    sem_close(sem_filled);
    sem_close(sem_empty);
    sem_close(sem_consumed);
    sem_close(sem_shutdown);

    // Unlink semaphores
    sem_unlink(SEM_INIT_READY_NAME);
    sem_unlink(SEM_FILLED_NAME);
    sem_unlink(SEM_EMPTY_NAME);
    sem_unlink(SEM_CONSUMED_NAME);
    sem_unlink(SEM_SHUTDOWN_NAME);

    std::cout << "Producer: IPC cleanup complete.\n";
}

int main() {
    sem_t *sem_init, *sem_filled, *sem_empty;
    sem_t *sem_consumed, *sem_shutdown;

    if (!init_ipc(sem_init, sem_filled, sem_empty, sem_consumed, sem_shutdown))
        return 1;

    int shmid;
    SharedQueue* queue = init_shm(shmid);
    if (!queue) return 1;

    // Step 3: Signal consumer to start
    sem_post(sem_init);
    std::cout << "Producer: Init signal sent.\n";

    // Step 4: Produce items
    produce(queue, sem_filled, sem_empty, 5);

    // Wait for consumer to finish processing
    std::cout << "Producer: Waiting for consumer...\n";
    sem_wait(sem_consumed);

    // Step 5a: Notify consumer shutdown is about to happen
    sem_post(sem_shutdown);
    std::cout << "Producer: Shutdown signal sent.\n";

    // Step 5b: Cleanup our side
    cleanup(shmid, queue, sem_init, sem_filled, sem_empty, sem_consumed, sem_shutdown);
    std::cout << "Producer: Exiting.\n";
    return 0;
}
