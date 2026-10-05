#include <vector>
#include <map>
#include <algorithm>
#include <mutex>
#include <condition_variable>
long long sumBig(const std::vector<int> &v)
{
    long long s = 0;
    for (int x : v)
        if (x >= 128)
            s += x;
    return s;
}

// v \in [0,256)
long long not_branch_sumBig(const std::vector<int> &v)
{
    long long count[256] = {};
    for (auto &x : v)
    {
        ++count[x];
    }
    long long sum = 0;
    for (int i = 128; i < 256; ++i)
        sum += static_cast<long long>(i) * count[i];
    return sum;
}

std::mutex m;
std::condition_variable lock;
long long share_sum;
#include <omp.h>

void atomic_parallel_histogramm(const std::vector<int> &v)
{
    long long tmp[256];
#pragma omp parallel for private(tmp)
    for (auto x : v)
    {
        ++tmp[x];
    }
#pragma omp parallel for reduction(+ : sum)
    for (int i = 128; i < 256; ++i)
        share_sum += static_cast<long long>(i) * tmp[i];
}

#include <cstddef>
#include <thread>
#include <vector>

long long parallel_sum(const std::vector<int> &v, int T)
{
    const std::size_t n = v.size();
    std::vector<long long> partial(T); // one result slot per thread
    std::vector<std::thread> threads;
    threads.reserve(T);

    for (int t = 0; t < T; ++t)
    {
        const std::size_t begin = t * n / T;
        const std::size_t end = (t + 1) * n / T;

        threads.emplace_back([&v, &partial, t, begin, end]
                             {
                                 long long s = 0; // local accumulator
                                 for (std::size_t i = begin; i < end; ++i)
                                     s += v[i];
                                 partial[t] = s; // write once, at the end
                             });
    }

    for (auto &th : threads)
        th.join(); // wait for all threads

    long long total = 0;
    for (long long s : partial)
        total += s;
    return total;
}