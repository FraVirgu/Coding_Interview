#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

constexpr int kIterations = 1'000'000;
constexpr int kThreads = 2;

// ---------- 1. mutex: lock on every increment ----------
int mutex_counter = 0;
std::mutex m;

void work_mutex()
{
    for (int i = 0; i < kIterations; ++i)
    {
        std::lock_guard<std::mutex> lock(m); // named guard: unlocks at end of each iteration
        ++mutex_counter;
    }
}

// ---------- 2. atomic: one indivisible read-modify-write per increment ----------
std::atomic<int> atomic_counter{0};

void work_atomic()
{
    for (int i = 0; i < kIterations; ++i)
        atomic_counter.fetch_add(1, std::memory_order_relaxed); // only the count matters, no ordering needed
}

// ---------- 3. local accumulation: no sharing in the loop ----------
std::atomic<int> local_counter{0};

void work_local()
{
    int local = 0; // must live INSIDE the function: a global `local` would be shared again
    for (int i = 0; i < kIterations; ++i)
        ++local;
    local_counter.fetch_add(local, std::memory_order_relaxed); // one synchronized write per thread
}

// Runs `fn` on kThreads threads and returns elapsed milliseconds.
template <typename F>
double run(F fn)
{
    auto start = std::chrono::steady_clock::now();
    std::thread t1(fn), t2(fn);
    t1.join();
    t2.join();
    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

int main()
{
    const int expected = kThreads * kIterations;

    double t_mutex = run(work_mutex);
    double t_atomic = run(work_atomic);
    double t_local = run(work_local);

    // join() synchronizes with the finished threads, so reading the plain int here is safe.
    std::cout << "mutex : " << mutex_counter << "  (" << t_mutex << " ms)\n";
    std::cout << "atomic: " << atomic_counter.load() << "  (" << t_atomic << " ms)\n";
    std::cout << "local : " << local_counter.load() << "  (" << t_local << " ms)\n";

    bool ok = mutex_counter == expected && atomic_counter == expected && local_counter == expected;
    std::cout << (ok ? "All counters correct\n" : "MISMATCH\n");
    return ok ? 0 : 1;
}