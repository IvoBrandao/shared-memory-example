#include "common.hpp"
#include <cerrno>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <semaphore.h>
#include <sys/mman.h> // mlock, madvise
#include <sys/shm.h>
#include <unistd.h>

bool init_ipc(sem_t *&sem_init, sem_t *&sem_filled, sem_t *&sem_empty, sem_t *&sem_consumed, sem_t *&sem_shutdown)
{
    sem_init = sem_open(SEM_INIT_READY_NAME, O_CREAT, 0666, 0);
    sem_filled = sem_open(SEM_FILLED_NAME, O_CREAT, 0666, 0);
    sem_empty = sem_open(SEM_EMPTY_NAME, O_CREAT, 0666, QUEUE_CAPACITY);
    sem_consumed = sem_open(SEM_CONSUMED_NAME, O_CREAT, 0666, 0);
    sem_shutdown = sem_open(SEM_SHUTDOWN_NAME, O_CREAT, 0666, 0);

    if (sem_init == SEM_FAILED || sem_filled == SEM_FAILED || sem_empty == SEM_FAILED || sem_consumed == SEM_FAILED ||
        sem_shutdown == SEM_FAILED)
    {
        perror("sem_open");
        return false;
    }
    std::cout << "Producer: IPC semaphores initialized.\n";
    return true;
}

SharedQueue *init_shm(int &shmid)
{
    int flags = IPC_CREAT | 0666;
#if USE_HUGEPAGES
    flags |= SHM_HUGETLB;
#endif

    shmid = shmget(SHM_KEY, SHM_SIZE, flags);
#if USE_HUGEPAGES
    if (shmid < 0 && errno == EPERM)
    {
        std::cerr << "Warning: cannot allocate huge pages (EPERM). Falling back to "
                     "normal pages.\n";
        // Retry without huge pages
        shmid = shmget(SHM_KEY, SHM_SIZE, IPC_CREAT | 0666);
    }
#endif
    if (shmid < 0)
    {
        perror("shmget");
        return nullptr;
    }

    void *ptr = shmat(shmid, nullptr, 0);
    if (ptr == (void *)-1)
    {
        perror("shmat");
        return nullptr;
    }

    auto *queue = static_cast<SharedQueue *>(ptr);
    queue->head.store(0);
    queue->tail.store(0);
    queue->producer_finished.store(false);

#if USE_PRETOUCH
    std::cout << "Producer: Pre-touching pages...\n";
    for (std::size_t s = 0; s < QUEUE_CAPACITY; ++s)
    {
        char *base = queue->slots[s].data;
        for (std::size_t off = 0; off < ELEMENT_SIZE; off += 4096)
        {
            base[off] = 0;
        }
    }
#endif

#if USE_MLOCK
    if (mlock(queue, SHM_SIZE) != 0)
    {
        perror("mlock");
    }
    else
    {
        std::cout << "Producer: Locked shared memory into RAM.\n";
    }
#endif

#if USE_MADV
    if (madvise(queue, SHM_SIZE, MADV_WILLNEED) != 0)
    {
        perror("madvise");
    }
    else
    {
        std::cout << "Producer: Advised kernel to prefetch pages.\n";
    }
#endif

    std::cout << "Producer: Shared memory initialized.\n";
    return queue;
}

void produce(SharedQueue *queue, sem_t *sem_filled, sem_t *sem_empty, int count)
{
    for (int i = 0; i < count; ++i)
    {
        sem_wait(sem_empty);
        uint32_t idx = queue->head.load(std::memory_order_relaxed);

        std::cout << "Producer: Writing item " << (i + 1) << " to slot " << idx << "\n";

        auto start = std::chrono::high_resolution_clock::now();
        std::memset(queue->slots[idx].data, 'A' + i, ELEMENT_SIZE);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double, std::milli> d = end - start;
        std::cout << "Producer: Wrote " << std::to_string(ELEMENT_SIZE_MB) << " MB in " << d.count() << " ms.\n";

        queue->head.store((idx + 1) % QUEUE_CAPACITY, std::memory_order_release);
        sem_post(sem_filled);
        sleep(1);
    }

    queue->producer_finished.store(true, std::memory_order_release);
    sem_post(sem_filled);
}

void cleanup(int shmid, SharedQueue *queue, sem_t *sem_init, sem_t *sem_filled, sem_t *sem_empty, sem_t *sem_consumed,
             sem_t *sem_shutdown)
{
    shmdt(queue);
    shmctl(shmid, IPC_RMID, nullptr);

    sem_close(sem_init);
    sem_close(sem_filled);
    sem_close(sem_empty);
    sem_close(sem_consumed);
    sem_close(sem_shutdown);

    sem_unlink(SEM_INIT_READY_NAME);
    sem_unlink(SEM_FILLED_NAME);
    sem_unlink(SEM_EMPTY_NAME);
    sem_unlink(SEM_CONSUMED_NAME);
    sem_unlink(SEM_SHUTDOWN_NAME);

    std::cout << "Producer: IPC cleanup complete.\n";
}

int main()
{
    sem_t *sem_init, *sem_filled, *sem_empty;
    sem_t *sem_consumed, *sem_shutdown;

    if (!init_ipc(sem_init, sem_filled, sem_empty, sem_consumed, sem_shutdown))
        return 1;

    int shmid;
    SharedQueue *queue = init_shm(shmid);
    if (!queue)
        return 1;

    sem_post(sem_init);
    std::cout << "Producer: Init signal sent.\n";

    produce(queue, sem_filled, sem_empty, ELEMENTS_PRODUCED);

    std::cout << "Producer: Waiting for consumer...\n";
    sem_wait(sem_consumed);

    sem_post(sem_shutdown);
    std::cout << "Producer: Shutdown signal sent.\n";

    cleanup(shmid, queue, sem_init, sem_filled, sem_empty, sem_consumed, sem_shutdown);

    std::cout << "Producer: Exiting.\n";
    return 0;
}
