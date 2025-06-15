## 🧭 System Overview

The system consists of two independent processes that communicate via **shared memory** and coordinate access using **POSIX semaphores**.

* The **Producer** generates and writes large data blocks.
* The **Consumer** reads and processes these blocks concurrently.
* Synchronization ensures correct ordering and no data loss or overwriting.

---

## ✅ What the Application Does

1. **Initialization:**

   * Producer creates shared memory and semaphores.
   * Producer signals readiness via a semaphore (`sem_init_ready`).
   * Consumer waits until initialization is complete.

2. **Data Transfer Loop:**

   * Producer waits for an empty slot.
   * Producer writes 8MB data block to the slot.
   * Producer signals slot is filled.
   * Consumer waits for filled slot.
   * Consumer reads the data block in a thread.
   * Consumer marks the slot empty.

3. **Shutdown:**

   * After the final write, producer sets a shutdown flag.
   * Consumer reads remaining items and signals shutdown complete.
   * Both sides unlink semaphores and detach from shared memory.

---

## 🧩 Mermaid Diagram

```mermaid
flowchart TD
    A[Producer Start] --> B[Create Semaphores and Shared Memory]
    B --> C[Send Init Ready Signal]
    C --> D[Start Writing Loop]
    D -->|Wait for Empty Slot| E[Write Data Block to Slot]
    E --> F[Signal Slot Filled]

    F --> G[Consumer Waits for Init Ready]
    G --> H[Open Semaphores and Attach Shared Memory]
    H --> I[Start Reading Loop]
    I -->|Wait for Filled Slot| J[Spawn Thread to Read Slot]
    J --> K[Mark Slot Empty]
    K --> L[Signal Empty Slot]

    D -->|Set Shutdown Flag| M[Producer Wait for Shutdown Ack]
    L -->|Queue is Empty + Shutdown Flag| N[Signal Producer Shutdown]
    N --> O[Clean Up]
    M --> O
```

---

## 🧬 PlantUML Sequence Diagram

```plantuml
@startuml
actor Producer
actor Consumer

Producer -> Producer: Create shared memory and semaphores
Producer -> Consumer: sem_init_ready.post()

Consumer -> Consumer: Wait on sem_init_ready
Consumer -> Consumer: Attach to shared memory

loop while (more data)
    Producer -> Producer: Wait on sem_empty
    Producer -> SharedMemory: Write data to slot
    Producer -> Consumer: sem_filled.post()

    Consumer -> Consumer: Wait on sem_filled
    Consumer -> Consumer: Spawn thread
    Consumer -> Thread: Read data block
    Thread -> SharedMemory: Mark slot empty
    Thread -> Producer: sem_empty.post()
end

Producer -> SharedMemory: Set shutdown flag
Consumer -> Consumer: Detect shutdown + empty
Consumer -> Producer: sem_shutdown.post()

Producer -> Producer: Wait on sem_shutdown
Producer -> Producer: Cleanup
Consumer -> Consumer: Cleanup
@enduml
```
