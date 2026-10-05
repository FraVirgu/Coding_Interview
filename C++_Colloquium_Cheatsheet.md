# C++ Colloquium Cheat Sheet

Built from the 8 exercises in this folder. Each section says which file it comes from.

---

## 1. Containers and complexity

| Container | Lookup | Insert | Notes |
|---|---|---|---|
| `std::vector` | O(1) by index | O(1) amortized `push_back` | contiguous, cache-friendly; `reserve(n)` avoids reallocations |
| `std::unordered_map` | O(1) avg | O(1) avg | hash table; `reserve(n)` avoids rehashing |
| `std::map` / `std::multimap` | O(log n) | O(log n) | red-black tree, sorted by key; `multimap` allows duplicate keys |
| `std::multiset` | O(log n) | O(log n) | sorted, duplicates allowed; `*rbegin()` = max |
| `std::list` | O(n) | O(1) at an iterator | doubly linked; `splice` moves a node in O(1), **iterators stay valid** |
| `std::deque` | O(1) by index | O(1) at both ends | `push_front/back`, `pop_front/back` |
| `std::queue` | | | adapter: `push`, `front`, `pop` (`pop` returns void) |

```cpp
std::unordered_map<int,int> count;
for (int n : nums) count[n]++;              // operator[] default-inserts 0

for (const auto &[num, freq] : count) {}    // structured bindings (C++17)

std::multimap<int,int> byFreq;
byFreq.insert({freq, num});
for (auto it = byFreq.rbegin(); it != byFreq.rend(); ++it) {} // reverse iteration

std::multiset<int> w(nums.begin(), nums.begin() + k);
w.erase(w.find(x));                         // erases ONE copy
w.erase(x);                                 // WARNING: erases ALL copies

std::vector<std::vector<int>> buckets(n + 1);   // bucket sort by frequency
```

### Top-K frequent (1_top_frequent.cpp)
- Sort-based approach with `multimap`: O(n log n).
- Bucket approach: index = frequency, value = list of numbers. O(n). Frequency is at most `n`, so you need `n + 1` buckets.

### Sliding window max (4_slinding_window.cpp)
| Approach | Time | Space |
|---|---|---|
| Brute force | O(n·k) | O(1) |
| `multiset` | O(n log k) | O(k) |
| Monotonic `deque` of indices | **O(n)** | O(k) |

Deque steps for each `i`:
1. Pop the front if it is outside the window.
2. Pop from the back while `nums[back] <= nums[i]`.
3. `push_back(i)`.
4. The front is the max once `i + 1 >= k`.

Each index is pushed and popped at most once, so the total is O(n).

### LRU cache (5_lru_cache.cpp)
- `std::list<pair<key,value>>` plus `unordered_map<key, list::iterator>`. Both `get` and `put` are O(1).
- The list stores the **key** too, so eviction can call `pos_.erase(items_.back().first)`.
- `items_.splice(items_.begin(), items_, it)` moves a node to the front without allocating or invalidating iterators.
- `if (auto it = m.find(k); it != m.end())` is an if-statement with an initializer (C++17).
- `emplace_front(key, value)` constructs in place.

---

## 2. Performance traps (2_find_waste.cpp)

Original code:
```cpp
std::vector<std::string> filter(std::vector<std::string> v, std::string prefix)
{
    std::vector<std::string> out;
    for (auto s : v)
        if (s.substr(0, prefix.size()) == prefix) out.push_back(s);
    return out;
}
```

| Problem | Cost | Fix |
|---|---|---|
| `v` by value | copies the whole vector (N allocations) | `const std::vector<std::string>&` |
| `prefix` by value | copies the string | `std::string_view` (no allocation, no copy) |
| `auto s` in the loop | copies each string | `const auto &s` |
| `s.substr(...)` | creates a temporary string (may allocate) | `s.starts_with(prefix)` (C++20) or `s.compare(0, n, prefix) == 0` |
| `out` grows incrementally | repeated reallocations and moves | `out.reserve(v.size())` |
| `push_back(s)` | copies the string, which is required here because `out` owns its strings | none |

Complexity of the fixed version: O(N·P) time, where P is the prefix length. Heap allocations: one for the vector buffer plus one per matching string (copy), or zero strings if you return indices or views instead.

General rules:
- Pass big objects by `const &`. Pass small trivial types by value.
- Use `string_view` for read-only string parameters. Never return a view of a temporary.
- `reserve` when you know the size in advance.
- `emplace_back` constructs in place. `push_back(std::move(x))` avoids a copy.

---

## 3. Rule of Five / RAII (3_UniquePtr.cpp)

```cpp
class IntBuffer {
    int *data_ = nullptr;           // default member initializers
    std::size_t size_ = 0;
public:
    IntBuffer() = default;
    explicit IntBuffer(std::size_t n) : data_(new int[n]()), size_(n) {} // () zero-initializes
    ~IntBuffer() { delete[] data_; }                                     // new[] <-> delete[]

    IntBuffer(const IntBuffer &o) : data_(new int[o.size_]), size_(o.size_) {
        std::copy(o.data_, o.data_ + o.size_, data_);
    }
    IntBuffer &operator=(const IntBuffer &o) {
        if (this != &o) {
            int *nd = new int[o.size_];                  // 1. allocate first
            std::copy(o.data_, o.data_ + o.size_, nd);   //    copy
            delete[] data_;                              // 2. then release the old one
            data_ = nd; size_ = o.size_;
        }
        return *this;
    }
    IntBuffer(IntBuffer &&o) noexcept
        : data_(std::exchange(o.data_, nullptr)), size_(std::exchange(o.size_, 0)) {}
    IntBuffer &operator=(IntBuffer &&o) noexcept {
        if (this != &o) {
            delete[] data_;
            data_ = std::exchange(o.data_, nullptr);
            size_ = std::exchange(o.size_, 0);
        }
        return *this;
    }
    int &operator[](std::size_t i) { return data_[i]; }
    const int &operator[](std::size_t i) const { return data_[i]; }   // const overload
};
```

Key points:
- **Rule of 0/3/5**: if you write any of destructor, copy ctor, copy assignment, move ctor or move assignment, you probably need all five. Better still, use `std::vector` or `std::unique_ptr` and write none.
- Copy assignment: allocate first, then delete the old buffer (strong exception safety).
- Move: `noexcept` is important, because `std::vector` only moves elements on reallocation if the move constructor is `noexcept`.
- After a move the source must be left valid but empty.
- `std::exchange(obj, new)` returns the old value and assigns the new one.
- `explicit` prevents implicit conversions such as `IntBuffer b = 5;`.
- `std::move` is just a cast to an rvalue reference. It moves nothing by itself.
- `new int[n]` leaves values uninitialized. `new int[n]()` zero-initializes.
- Smart pointers:
  - `unique_ptr`: sole ownership, move-only, no overhead. Create with `make_unique<T>(args)`.
  - `shared_ptr`: reference counted. Create with `make_shared<T>`.
  - `weak_ptr`: non-owning observer, breaks cycles.
  - `unique_ptr<int[]>` for arrays.

---

## 4. Concurrency (6_thread.cpp, 7_bbqueue.cpp, 8_sorter_vs_unsorted.cpp)

### Threads
```cpp
std::thread t(fn, args...);   // starts immediately
t.join();                     // wait. A joinable thread destroyed without join/detach => std::terminate
std::vector<std::thread> ts;  ts.emplace_back([&]{ ... });
```
- Lambdas capture with `[&]` (by reference), `[=]` (by copy), `[&, c]` (all by reference except `c` by copy), or `[&v, t]` (an explicit list).
- Capture the loop variable **by value** if the thread outlives the iteration.
- `join()` synchronizes-with the thread's completion, so reading its results afterward is safe.

### Three ways to count with 2 threads, 1M increments each
| Version | Mechanism | Speed |
|---|---|---|
| `std::lock_guard<std::mutex>` per increment | lock and unlock each time | slowest |
| `std::atomic<int>::fetch_add(1, relaxed)` | one atomic RMW per increment | medium (cache-line contention) |
| Local counter, then **one** `fetch_add` at the end | no sharing in the hot loop | fastest |

- A plain `int` incremented from several threads is a **data race** (undefined behavior).
- `memory_order_relaxed` guarantees atomicity but no ordering. It is enough for pure counters.
- Default is `memory_order_seq_cst` (strongest, slowest).
- False sharing: independent variables on the same cache line slow each other down. Fix by padding with `alignas(64)` or by accumulating locally.
- Per-thread partial results in `vector<long long> partial(T)`, combined after `join` (see `parallel_sum`).

### Mutex helpers
- `std::lock_guard<std::mutex>`: simple RAII lock, cannot be unlocked early.
- `std::unique_lock<std::mutex>`: can `unlock()` and `lock()`, and is required by `condition_variable::wait`.
- `std::scoped_lock(m1, m2)`: locks several mutexes without deadlock (C++17).
- A guard must be **named**. `std::lock_guard<std::mutex>(m);` creates a temporary that unlocks immediately.

### Bounded blocking queue (producer/consumer)
```cpp
void push(T item) {
    std::unique_lock<std::mutex> lock(m_);
    not_full_.wait(lock, [&]{ return q_.size() < capacity_; });  // predicate loop handles spurious wakeups
    q_.push(std::move(item));
    lock.unlock();                 // don't wake a consumer into a mutex we still hold
    not_empty_.notify_one();
}
```
- `wait(lock, pred)` is equivalent to `while(!pred()) wait(lock);`. It releases the mutex while sleeping and reacquires it on wake.
- Always wait with a **predicate** because of spurious wakeups and lost wakeups.
- Two condition variables (`not_full_`, `not_empty_`) so producers and consumers wake the right side.
- `notify_one` wakes one waiter, `notify_all` wakes everyone.
- Class templates: `template <typename T> class BoundedQueue`.

### Timing
```cpp
auto s = std::chrono::steady_clock::now();
...
double ms = std::chrono::duration<double, std::milli>(end - s).count();
```
Use `steady_clock` for measuring intervals because it never goes backwards.

### Other
- Deadlock conditions: mutual exclusion, hold and wait, no preemption, circular wait. Avoid by locking in a consistent order or using `scoped_lock`.
- `std::async` / `std::future` / `std::promise` for return values from tasks.
- OpenMP: `#pragma omp parallel for reduction(+:sum)` gives each thread a private copy and combines them at the end.

---

## 5. Branch prediction and cache (8_sorter_vs_unsorted.cpp)

```cpp
for (int x : v) if (x >= 128) s += x;   // fast on SORTED data, slow on random data (mispredictions)
```
- On random data the branch is unpredictable, so the CPU flushes the pipeline roughly half the time.
- Fixes:
  - Sort first, if you do many passes.
  - Make it branchless: `s += (x >= 128) * x;`, or the **counting/histogram** trick (`count[256]`, then a sum over `i = 128..255`). This is O(n + 256) and only valid when the value range is small.
- Sequential vector access is cache-friendly. Linked lists and trees are not.
- Don't hand-roll what the compiler already does. Measure with `-O2`/`-O3`.

---

## 6. Modern C++ syntax quick reference

```cpp
auto x = ...;  const auto &r = ...;  auto &&fwd = ...;
for (auto &[k, v] : m) {}                  // structured bindings (C++17)
if (auto it = m.find(k); it != m.end()) {} // if with initializer (C++17)
constexpr int kN = 1'000'000;              // digit separators (C++14)
static_cast<int>(sz)                       // preferred over C-style casts
static_cast<long long>(i) * count[i]       // cast before multiplying to avoid overflow
s.starts_with("ab")                        // C++20
std::string_view sv = str;                 // non-owning
template <typename F> double run(F fn)     // generic callable
[&, c](args) { ... }                       // lambda
class X { X() = default; X(const X&) = delete; };
noexcept, explicit, const-correctness, override, final
```

### Pitfalls worth remembering
- Comparing `size_t` to `int` (signed/unsigned mismatch): cast explicitly, as in `i + k <= nums.size()` or `(int)result.size() < k`.
- `unsigned` arithmetic wraps around: `size() - 1` on an empty container is huge.
- `vector<T>::operator[]` does no bounds checking. `at()` does and throws.
- An element access with `vec[vec.size()]` is out of bounds. Use `size() - 1` as the last valid index.
- Iterator invalidation: `vector::push_back` may invalidate every iterator. `list::splice` and `list::erase` of other nodes do not.
- `unordered_map::operator[]` inserts the key if it is missing. Use `find` or `count` to test without modifying.
- A function with a non-void return type must return on every path, or the behavior is undefined.

---

## 7. Bugs spotted in the exercise files (good review practice)

**1_top_frequent.cpp**
- Both `topKFrequent` overloads have **identical signatures** (redefinition error). Rename one.
- The O(n) version has **no `return solution;`**. This is undefined behavior.
- `int f = freq_vect.size()` starts one past the end. It should be `freq_vect.size() - 1`, or `nums.size()`.
- `solution.size() < k` compares `size_t` with `int`.
- Unused variable `k_best` in the first version.
- The comment says "O(n + nlog(n) + k)", which simplifies to O(n log n).

**8_sorter_vs_unsorted.cpp**
- `atomic_parallel_histogramm`:
  - `tmp` is declared uninitialized and `private(tmp)` gives each thread an uninitialized copy.
  - `private` copies are never merged back, so counts are lost. You need `reduction(+ : tmp[:256])` (OpenMP 4.5+) or a local histogram per thread merged under a lock or atomics.
  - `reduction(+ : sum)` refers to an undeclared `sum`. The code writes to `share_sum` instead.
  - OpenMP range-based `for` over a `vector` requires C++20-compatible OpenMP, so use an index loop to be safe.
  - Needs the `-fopenmp` compile flag.
- The `mutex m` and `condition_variable lock` globals are unused (and `lock` is a misleading name).
- Duplicate `#include`s.

**2_find_waste.cpp**: the fixed version is correct. `starts_with` needs `-std=c++20`.

---

## 8. Likely oral questions

1. Why does the LRU list store the key as well as the value?
2. Why is `std::unordered_map` O(1) on average but O(n) worst case?
3. Explain the Rule of Five. Why must the move operations be `noexcept`?
4. What happens to the source object after a move?
5. Mutex vs atomic vs thread-local accumulation: when is each right?
6. Why does `condition_variable::wait` need a predicate, and why `unique_lock` rather than `lock_guard`?
7. What is a data race, and what is the difference between a race condition and a data race?
8. Why is the monotonic deque solution O(n) even though it has a nested `while`?
9. Why is a sorted array faster for the same loop? (Branch prediction.)
10. `vector` vs `list` in practice. (Cache locality usually makes `vector` win.)
11. What does `const` after a member function mean? What does `explicit` do?
12. `shared_ptr` vs `unique_ptr`: cost and when to choose each. What problem does `weak_ptr` solve?
13. Stack vs heap, and what `new`/`delete` do versus `malloc`/`free`.
14. Virtual functions and vtables. Why must a base class destructor be `virtual`?
