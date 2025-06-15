#include "common.hpp"
#include <iostream>
#include <semaphore.h>
#include <sys/shm.h>

int main()
{
    std::cout << "Cleaning up IPC resources..." << std::endl;

    // Remove shared memory
    int shmid = shmget(SHM_KEY, 0, 0);
    if (shmid != -1)
    {
        if (shmctl(shmid, IPC_RMID, nullptr) == -1)
        {
            perror("shmctl");
        }
        else
        {
            std::cout << "Shared memory segment removed." << std::endl;
        }
    }

    // Unlink named semaphores
    if (sem_unlink(SEM_INIT_READY_NAME) == 0)
    {
        std::cout << "Semaphore " << SEM_INIT_READY_NAME << " unlinked." << std::endl;
    }
    if (sem_unlink(SEM_FILLED_NAME) == 0)
    {
        std::cout << "Semaphore " << SEM_FILLED_NAME << " unlinked." << std::endl;
    }
    if (sem_unlink(SEM_EMPTY_NAME) == 0)
    {
        std::cout << "Semaphore " << SEM_EMPTY_NAME << " unlinked." << std::endl;
    }

    if (sem_unlink(SEM_CONSUMED_NAME) == 0)
    {
        std::cout << "Semaphore " << SEM_CONSUMED_NAME << " unlinked." << std::endl;
    }

    if (sem_unlink(SEM_SHUTDOWN_NAME) == 0)
    {
        std::cout << "Semaphore " << SEM_SHUTDOWN_NAME << " unlinked." << std::endl;
    }

    std::cout << "Cleanup complete." << std::endl;
    return 0;
}
