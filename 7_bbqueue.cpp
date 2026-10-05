#include <condition_variable>
#include <cstddef>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

template <typename T>
class BoundedQueue
{
public:
    explicit BoundedQueue(std::size_t capacity) : capacity_(capacity) {}

    // Blocks while the queue is full.
    void push(T item)
    {
        std::unique_lock<std::mutex> lock(m_);
        not_full_.wait(lock, [&]
                       { return q_.size() < capacity_; });
        q_.push(std::move(item));
        lock.unlock();           // don't wake a consumer into a mutex we still hold
        not_empty_.notify_one(); // there is now at least one item
    }

    // Blocks while the queue is empty.
    T pop()
    {
        std::unique_lock<std::mutex> lock(m_);
        not_empty_.wait(lock, [&]
                        { return !q_.empty(); });
        T item = std::move(q_.front());
        q_.pop();
        lock.unlock();
        not_full_.notify_one(); // there is now at least one free slot
        return item;
    }

private:
    std::queue<T> q_;
    std::size_t capacity_;
    std::mutex m_;                      // protects q_
    std::condition_variable not_full_;  // producers wait here
    std::condition_variable not_empty_; // consumers wait here
};

int main()
{
    constexpr int kProducers = 4, kConsumers = 4, kPerProducer = 100'000;
    constexpr long long kTotalItems = 1LL * kProducers * kPerProducer;
    constexpr long long kExpected = kProducers * (1LL * kPerProducer * (kPerProducer + 1) / 2);

    BoundedQueue<int> q(16);                       // small capacity so producers really block
    std::vector<long long> partial(kConsumers, 0); // written only after each consumer finishes

    std::vector<std::thread> threads;
    for (int p = 0; p < kProducers; ++p)
        threads.emplace_back([&]
                             {
            for (int i = 1; i <= kPerProducer; ++i) q.push(i); });
    for (int c = 0; c < kConsumers; ++c)
        threads.emplace_back([&, c]
                             {
            long long local = 0;  // accumulate locally, no sharing in the hot loop
            for (long long i = 0; i < kTotalItems / kConsumers; ++i) local += q.pop();
            partial[c] = local; });
    for (auto &t : threads)
        t.join();

    long long sum = 0;
    for (long long s : partial)
        sum += s;
    std::cout << "sum = " << sum << ", expected = " << kExpected
              << (sum == kExpected ? "  OK\n" : "  MISMATCH\n");
    return sum == kExpected ? 0 : 1;
}