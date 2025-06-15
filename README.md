# Shared Memory Producer-Consumer System

This project implements a high-performance **Producer-Consumer** model using **POSIX shared memory**, **semaphores**, and **multi-threading**. It is designed to transfer large data blocks efficiently between two separate processes (Producer and Consumer) with support for advanced Linux features like **huge pages**, **mlock**, **madvise**, and **page pre-touching**.

---

## 🧠 Architecture Overview

The application consists of two main programs:

- **Producer:** Writes large blocks of data into shared memory.
- **Consumer:** Reads and processes each block in parallel using threads.

Communication and synchronization are achieved using:

- **POSIX Semaphores** (`sem_open`)
- **POSIX Shared Memory** (`shmget`, `shmat`)
- **Memory Synchronization Primitives** (`std::atomic`)

---

## 🔄 Communication Pipeline

### 1. Shared Memory Buffer

The shared buffer is a circular queue of N fixed-size slots (e.g., 8MB each). The structure (`SharedQueue`) holds:

- A data block array
- Head/tail indices
- Atomic counters for synchronization

### 2. Semaphores

- `sem_filled`: Signals when a slot has data to consume.
- `sem_empty`: Signals when a slot is free to write into.
- `sem_init_ready`: Ensures the consumer waits until the producer is ready.
- `sem_shutdown`: Signals clean termination once all data has been processed.

### 3. Execution Flow

#### Producer

1. Initializes semaphores and shared memory.
2. Sends an "init ready" signal to the consumer.
3. Writes items to the shared queue, signaling `sem_filled` after each.
4. Waits for a shutdown signal from the consumer, then cleans up.

#### Consumer

1. Waits for the init signal.
2. Opens semaphores and attaches to shared memory.
3. Consumes items in parallel threads as they become available.
4. After the producer finishes, drains remaining items and sends shutdown signal.

---

## ⚙️ Compile-Time Options

You can enable/disable advanced memory optimizations using preprocessor flags.

| Option           | Default | Description                                                                 |
|------------------|---------|-----------------------------------------------------------------------------|
| `USE_PRETOUCH`   | 1       | Faults each page at init time to reduce page faults during runtime          |
| `USE_MLOCK`      | 1       | Locks memory to prevent paging out to disk                                 |
| `USE_MADV`       | 1       | Uses `madvise()` to hint the kernel for prefetching and use of huge pages   |
| `USE_HUGEPAGES`  | 1       | Allocates shared memory from the huge page pool (2MB per page)              |

You can control these via `#define` statements or `-D` flags during compilation.

---

## 🚀 Performance Optimizations

- **Threaded Consumer:** Each buffer slot is processed in a separate thread for concurrency.
- **Huge Pages:** Reduces TLB misses and improves throughput for large memory blocks.
- **Pre-touching:** Ensures all memory pages are faulted into RAM before starting processing.
- **`mlock()`:** Prevents OS from swapping the memory to disk.
- **`madvise()`:** Gives the kernel hints about memory usage patterns.

---

## 🛠️ Setup Instructions

### Prerequisites

- Linux system with POSIX support
- GCC or Clang
- Huge Pages enabled (see below)

### Building

```bash
mkdir build && cd build
cmake ..
make
