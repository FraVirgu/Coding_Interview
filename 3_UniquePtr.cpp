#include <algorithm> // std::copy
#include <cstddef>   // std::size_t
#include <iostream>
#include <utility> // std::exchange

class IntBuffer
{
    int *data_ = nullptr;
    std::size_t size_ = 0;

public:
    // Default constructor: empty buffer (members already have default values)
    IntBuffer() = default;

    // Allocate n ints, all set to 0 thanks to the "()"
    explicit IntBuffer(std::size_t n) : data_(new int[n]()), size_(n) {}

    // Destructor: new[] pairs with delete[]. delete[] on nullptr is safe.
    ~IntBuffer() { delete[] data_; }

    // ---------- COPY: allocates, other keeps its own array ----------

    // Copy constructor: deep copy
    IntBuffer(const IntBuffer &other)
        : data_(new int[other.size_]), size_(other.size_)
    {
        std::copy(other.data_, other.data_ + other.size_, data_);
    }

    // Copy assignment: exception-safe order
    IntBuffer &operator=(const IntBuffer &other)
    {
        if (this != &other)
        {
            // 1. Allocate and copy FIRST. If new throws, *this is untouched.
            int *newData = new int[other.size_];
            std::copy(other.data_, other.data_ + other.size_, newData);
            // 2. Only now free the old array and commit.
            delete[] data_;
            data_ = newData;
            size_ = other.size_;
        }
        return *this;
    }

    // ---------- MOVE: never allocates, steals and empties other ----------

    // Move constructor
    IntBuffer(IntBuffer &&other) noexcept
        : data_(std::exchange(other.data_, nullptr)),
          size_(std::exchange(other.size_, 0)) {}

    // Move assignment
    IntBuffer &operator=(IntBuffer &&other) noexcept
    {
        if (this != &other)
        {
            delete[] data_;                              // free what I own
            data_ = std::exchange(other.data_, nullptr); // take + empty other
            size_ = std::exchange(other.size_, 0);
        }
        return *this;
    }

    // ---------- Accessors ----------

    std::size_t size() const noexcept { return size_; }

    int &operator[](std::size_t i) { return data_[i]; }
    const int &operator[](std::size_t i) const { return data_[i]; }
};

int main()
{
    IntBuffer a(3);
    a[0] = 10;

    IntBuffer b = a; // copy constructor
    b[0] = 99;
    std::cout << a[0] << '\n'; // 10: a and b are independent

    IntBuffer c = std::move(a);                   // move constructor
    std::cout << c[0] << ' ' << a.size() << '\n'; // "10 0"

    IntBuffer d(5);
    d = b;                                        // copy assignment
    std::cout << d[0] << ' ' << d.size() << '\n'; // "99 3"

    d = std::move(c);                             // move assignment
    std::cout << d[0] << ' ' << c.size() << '\n'; // "10 0"

    d = d;                     // self-assignment: must be safe
    std::cout << d[0] << '\n'; // 10
}