// Exercise: raw pointer mechanics — old-school style.
// Build:  ./build.sh 09_pointers -r
//
// No std::unique_ptr, std::shared_ptr, std::vector, std::ranges,
// std::string in this exercise (except where main() needs them for I/O).
// The point is to feel the underlying machinery: address-of, indirection,
// pointer arithmetic, the const-pointer maze, array decay, and C-string
// traversal. This is what most of the modern idioms are built on top of.

#include <cstddef>
#include <iostream>

// ============================================================================
// Section 1: address-of and indirection
// ============================================================================
// & takes the address of an lvalue, producing a pointer.
// * dereferences a pointer, yielding the pointee as an lvalue.
// They're inverses: *(&x) is x.

// Given a pointer to an int, add `delta` to the pointed-to value (mutating
// the original). Demonstrates that dereferencing a non-const pointer gives
// you an lvalue you can assign to.
void add_to(int* p, int delta) {
    *p += delta;
}

// Return the address of x. (Trivial — just shows that & yields the matching
// pointer type. Used in main to obtain pointers for the other tests.)
int* address_of(int& x) {
    return &x;
}

// ============================================================================
// Section 2: pointer arithmetic over an array
// ============================================================================
// In an array, p + n advances by n ELEMENTS (not n bytes). arr[i] is exactly
// *(arr + i). Many traditional C algorithms walk a pair of pointers — begin
// and end — instead of indexing.

// Sum the n ints starting at `first` using POINTER ARITHMETIC, not indexing.
// (Idiomatic C-style: walk a pointer from `first` to `first + n`.)
long sum_n(const int* first, std::size_t n) {
    // TODO: walk a pointer from `first` to `first + n`, accumulating *p.
    //       Hint: for (const int* p = first; p != first + n; ++p) total += *p;
    long total = 0;
    for (const int* p = first; p != first + n; p++)
        total += *p;
    return total;
}

// Find the first occurrence of `target` in [first, last). Return a pointer
// to it on success; return `last` on failure. This is the C++/STL convention:
// "one past the end" stands in for "not found".
const int* find_first(const int* first, const int* last, int target) {
    // TODO: walk p from first up to (but not including) last; return p when
    //       *p == target. Return last if you fall off the end.
    for (const int* p = first; p != last; p++)
        if (*p == target) return p;

    return last;
}

// ============================================================================
// Section 3: the two-pointer technique
// ============================================================================
// Reverse the elements in [first, last) in place by walking one pointer in
// from each end, swapping, and meeting in the middle. Stop when the two
// pointers cross or meet.

void reverse_in_place(int* first, int* last) {
    // TODO: while (first < last) { swap *first and *(last - 1);
    //       move first forward and last backward. }
    //       Note: `last` points ONE PAST the end, so the rightmost element
    //       is *(last - 1).

    while (first < last) {
        std::swap(*first, *(last - 1));
        //int tmp = *first;
        //*first = *(last - 1);
        //*(last - 1) = tmp;

        first++;
        last--;
    }
}

// ============================================================================
// Section 4: the const-pointer maze
// ============================================================================
// Four distinct things to keep straight. Read right-to-left:
//
//   int*             p — mutable pointer, mutable int
//   const int*       p — mutable pointer, const int      ("pointer to const int")
//   int* const       p — const pointer,   mutable int    ("const pointer to int")
//   const int* const p — const pointer,   const int
//
// Implement each of the following with the signature you're given. The point
// is to notice which assignments compile and which don't. You don't need to
// write any control flow — these are one-liners.

// "Pointer to const int": you may move p around, but you may NOT write *p.
// Return *p.
int read_through(const int* p) {
    // TODO: return *p;
    //*p = 8;  // build error 
    p++; p--;
    return *p;
}

// "Const pointer to int": you may NOT reassign p, but you MAY write *p.
// Write `value` through the pointer.
void write_through(int* const p, int value) {

    //p++; // build error
    *p = value;
}

// "Const pointer to const int": you can only read. Return *p.
int read_only(const int* const p) {
    //p++; // build error
    //*p = 9; // build error
    
    return *p;
}

// (For grins, try uncommenting one of these to see the compiler error:
//   void try_mutate(const int* p)        { *p = 0; }          // can't write *p
//   void try_rebind(int* const p, int* q){ p = q; }            // can't rebind p
// )

// ============================================================================
// Section 5: pointer to struct, and -> vs (*p).
// ============================================================================
// p->member is exactly (*p).member. Use whichever reads cleaner; -> is
// universal for pointer access to members.

struct Point { int x; int y; };

// Move a point by (dx, dy) via its pointer. Use -> for one and (*p). for
// the other, just to feel the equivalence.
void translate(Point* p, int dx, int dy) {
    // TODO: p->x += dx;
    //       (*p).y += dy;     // equivalent spelling

    p->x += dx;
    (*p).y += dy;
}

// ============================================================================
// Section 6: C-string traversal
// ============================================================================
// A C-string is a sequence of chars terminated by '\0'. There is no stored
// length — you find the end by walking until you see the null terminator.
// This is the C-pointer idiom in its purest form.

// Compute the length of a C-string by walking pointers. Do NOT use std::strlen.
std::size_t my_strlen(const char* s) {
    // TODO: walk a pointer p from s until *p is '\0'; return p - s.
    //std::size_t size = 0;
    //for (const char* p = s; *p != '\0'; p++)
    //    size++;

    //return size;

    const char* p = s;
    while (*p != '\0') p++;
    return p - s;
}

// ============================================================================
// main: harness
// ============================================================================
int main() {
    std::cout << "--- Section 1: address-of & indirection ---\n";
    int x = 10;
    int* px = address_of(x);
    std::cout << "*px before add_to: " << *px << "   (expect 10)\n";
    add_to(px, 5);
    std::cout << "x after add_to(+5): " << x << "    (expect 15)\n";

    std::cout << "\n--- Section 2: pointer arithmetic ---\n";
    int arr[] = {3, 1, 4, 1, 5, 9, 2, 6};
    constexpr std::size_t N = sizeof(arr) / sizeof(arr[0]);  // old-school size
    std::cout << "N = " << N << "  (expect 8)\n";
    std::cout << "sum_n(arr, N) = " << sum_n(arr, N) << "   (expect 31)\n";

    const int* hit  = find_first(arr, arr + N, 5);
    const int* miss = find_first(arr, arr + N, 99);
    std::cout << "find 5:  index " << (hit  - arr) << "   (expect 4)\n";
    std::cout << "find 99: index " << (miss - arr) << "   (expect 8 — i.e. end)\n";

    std::cout << "\n--- Section 3: two-pointer reverse ---\n";
    int rev[] = {1, 2, 3, 4, 5};
    reverse_in_place(rev, rev + 5);
    std::cout << "reversed: ";
    for (std::size_t i = 0; i < 5; ++i) std::cout << rev[i] << ' ';
    std::cout << "  (expect 5 4 3 2 1)\n";

    std::cout << "\n--- Section 4: const-pointer maze ---\n";
    int v = 7;
    std::cout << "read_through(&v)         = " << read_through(&v) << "  (expect 7)\n";
    write_through(&v, 99);
    std::cout << "v after write_through    = " << v << "  (expect 99)\n";
    const int kv = 42;
    std::cout << "read_only(&kv)           = " << read_only(&kv) << "  (expect 42)\n";

    std::cout << "\n--- Section 5: -> and (*p). ---\n";
    Point pt{0, 0};
    translate(&pt, 3, 4);
    std::cout << "pt = (" << pt.x << "," << pt.y << ")  (expect (3,4))\n";

    std::cout << "\n--- Section 6: my_strlen ---\n";
    const char* greeting = "hello, world";
    std::cout << "my_strlen(\"hello, world\") = " << my_strlen(greeting)
              << "  (expect 12)\n";
    std::cout << "my_strlen(\"\")              = " << my_strlen("")
              << "  (expect 0)\n";
}
