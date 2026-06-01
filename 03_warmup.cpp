// Short exercise: Rule of five for a move-only owning Buffer.
// Goal: refresh manual move semantics and resource management.
// Build:  ./build.sh 03_warmup -r

#include <cstddef>
#include <iostream>
#include <utility>

// Buffer owns a heap-allocated array of ints. Move-only.
//
// Implement the four function bodies marked TODO below.
// Hints encoded in the declarations:
//   - Move ops are noexcept (matters: std::vector picks the fast move-path
//     for elements only if the move ops are noexcept).
//   - Copy ops are deleted — this is a move-only type.
class Buffer {
public:
    explicit Buffer(std::size_t n);
    ~Buffer();

    Buffer(const Buffer&)            = delete;
    Buffer& operator=(const Buffer&) = delete;

    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;

    std::size_t size() const { return size_; }
    int&  operator[](std::size_t i)       { return data_[i]; }
    int   operator[](std::size_t i) const { return data_[i]; }

private:
    int*        data_;
    std::size_t size_;
};

// 1. Construct: allocate n ints, zero-initialized; track size.
//    Refresher: `new int[n]` leaves values indeterminate.
//                `new int[n]()` (or `new int[n]{}`) value-initializes to 0.
Buffer::Buffer(std::size_t n)
    : data_(new int[n]()), size_(n) {
}

// 2. Release the heap allocation.
Buffer::~Buffer() {
    delete[] data_;
}

// 3. Move-construct: take ownership of other's data;
//    leave other in a valid empty state.
Buffer::Buffer(Buffer&& other) noexcept
    : data_(other.data_), size_(other.size_) {
    other.data_ = nullptr;
    other.size_ = 0;
}

// 4. Move-assign: release current state, take ownership of other's data,
//    leave other in a valid empty state. Self-assignment must be safe.
Buffer& Buffer::operator=(Buffer&& other) noexcept {

    std::swap(data_, other.data_);
    std::swap(size_, other.size_);

    return *this;
}

namespace {
void print(const Buffer& b, const char* name) {
    std::cout << name << ": ";
    for (std::size_t i = 0; i < b.size(); ++i) std::cout << b[i] << ' ';
}
}

int main() {
    std::cout << "--- construct & fill ---\n";
    Buffer a(4);
    a[0] = 1; a[1] = 2; a[2] = 3; a[3] = 4;
    print(a, "a"); std::cout << "  (expect 1 2 3 4)\n";

    std::cout << "\n--- move-construct ---\n";
    Buffer b = std::move(a);
    print(b, "b"); std::cout << "  (expect 1 2 3 4)\n";
    std::cout << "a.size() after move: " << a.size() << "  (expect 0)\n";

    std::cout << "\n--- move-assign ---\n";
    Buffer c(2);
    c[0] = 99; c[1] = 99;
    c = std::move(b);
    print(c, "c"); std::cout << "  (expect 1 2 3 4)\n";
    std::cout << "b.size() after move: " << b.size() << "  (expect 0)\n";

    std::cout << "\n--- self move-assign (must not crash or corrupt) ---\n";
    c = std::move(c);
    std::cout << "c.size() after self-move: " << c.size() << "  (expect 4)\n";

    std::cout << "\n--- zero-initialization ---\n";
    Buffer d(3);
    print(d, "d"); std::cout << "  (expect 0 0 0)\n";
}
