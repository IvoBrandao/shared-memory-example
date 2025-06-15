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
```

## Example execution

### With Huge Pages Disabled

Execution of the producer and consumer with the settings:

```c++
#define USE_PRETOUCH 1 // Touch each page to fault it i
#ifndef USE_MLOCK
#define USE_MLOCK 1 // Lock pages into RAM (mlock)
#define USE_MADV 1 // Advise kernel to prefetch pages (MADV_WILLNEED)
#define USE_HUGEPAGES 0 // Allocate shared memory with huge pages
```

```text
./run_producer.sh
Producer: IPC semaphores initialized.
Producer: Pre-touching pages...
Producer: Locked shared memory into RAM.
Producer: Advised kernel to prefetch pages.
Producer: Shared memory initialized.
Producer: Init signal sent.
Producer: Writing item 1 to slot 0
Producer: Wrote 32 MB in 2.51804 ms.
Producer: Writing item 2 to slot 1
Producer: Wrote 32 MB in 2.28402 ms.
Producer: Writing item 3 to slot 2
Producer: Wrote 32 MB in 2.25075 ms.
Producer: Writing item 4 to slot 3
Producer: Wrote 32 MB in 2.23441 ms.
Producer: Writing item 5 to slot 4
Producer: Wrote 32 MB in 2.38323 ms.
Producer: Writing item 6 to slot 0
Producer: Wrote 32 MB in 2.12948 ms.
Producer: Waiting for consumer...
Producer: Shutdown signal sent.
Producer: IPC cleanup complete.
Producer: Exiting.
```

Consumer execution

```txt
./run_consumer.sh
Consumer: Init semaphore found. Waiting for signal...
Consumer: Attached to shared memory.
Consumer: Semaphores opened successfully.
[Thread 140519155951296] Read slot 0 in 13.5326 ms
[Thread 140519155951296] Marked slot 0 empty
[Thread 140519139169984] Read slot 1 in 13.2907 ms
[Thread 140519139169984] Marked slot 1 empty
[Thread 140519049000640] Read slot 2 in 13.4837 ms
[Thread 140519049000640] Marked slot 2 empty
[Thread 140519032219328] Read slot 3 in 13.7078 ms
[Thread 140519032219328] Marked slot 3 empty
[Thread 140519015438016] Read slot 4 in 13.4182 ms
[Thread 140519015438016] Marked slot 4 empty
[Thread 140518998656704] Read slot 0 in 2.51954 ms
[Thread 140518998656704] Marked slot 0 empty
Consumer: Joining worker threads...
[Thread 140518981875392] Read slot 1 in 2.54221 ms
[Thread 140518981875392] Marked slot 1 empty
Consumer: All items processed, signaling producer...
Consumer: IPC cleanup complete.
Consumer: Exiting.
```


### With Huge Pages Enabled

Execution of the producer and consumer with the settings:

```c
#define USE_PRETOUCH 1  // Touch each page to fault it i
#ifndef USE_MLOCK
#define USE_MLOCK 1     // Lock pages into RAM (mlock)
#define USE_MADV 1      // Advise kernel to prefetch pages (MADV_WILLNEED)
#define USE_HUGEPAGES 1 // Allocate shared memory with huge pages
```

Producer execution

```txt
./run_producer.sh
Producer: IPC semaphores initialized.
Producer: Pre-touching pages...
Producer: Locked shared memory into RAM.
Producer: Advised kernel to prefetch pages.
Producer: Shared memory initialized.
Producer: Init signal sent.
Producer: Writing item 1 to slot 0
Producer: Wrote 32 MB in 2.14431 ms.
Producer: Writing item 2 to slot 1
Producer: Wrote 32 MB in 2.33229 ms.
Producer: Writing item 3 to slot 2
Producer: Wrote 32 MB in 1.95667 ms.
Producer: Writing item 4 to slot 3
Producer: Wrote 32 MB in 1.88985 ms.
Producer: Writing item 5 to slot 4
Producer: Wrote 32 MB in 2.02096 ms.
Producer: Writing item 6 to slot 0
Producer: Wrote 32 MB in 2.09838 ms.
Producer: Waiting for consumer...
Producer: Shutdown signal sent.
Producer: IPC cleanup complete.
Producer: Exiting.
```

Consumer Execution

```txt
./run_consumer.sh
Consumer: Init semaphore found. Waiting for signal...
Consumer: Attached to shared memory.
Consumer: Semaphores opened successfully.
[Thread 140086672881344] Read slot 0 in 2.23129 ms
[Thread 140086672881344] Marked slot 0 empty
[Thread 140086656100032] Read slot 1 in 3.47608 ms
[Thread 140086656100032] Marked slot 1 empty
[Thread 140086639318720] Read slot 2 in 3.15369 ms
[Thread 140086639318720] Marked slot 2 empty
[Thread 140086622537408] Read slot 3 in 2.30085 ms
[Thread 140086622537408] Marked slot 3 empty
[Thread 140086532372160] Read slot 4 in 2.50649 ms
[Thread 140086532372160] Marked slot 4 empty
[Thread 140086515590848] Read slot 0 in 2.12986 ms
[Thread 140086515590848] Marked slot 0 empty
Consumer: Joining worker threads...
[Thread 140086498809536] Read slot 1 in 2.21936 ms
[Thread 140086498809536] Marked slot 1 empty
Consumer: All items processed, signaling producer...
Consumer: IPC cleanup complete.
Consumer: Exiting.
```