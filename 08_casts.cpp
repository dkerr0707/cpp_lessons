// Exercise: pick the right cast for the job.
// Build:  ./build.sh 08_casts -r
//
// Each section gives a context and asks you to perform a conversion using
// the right named cast. Fill in the TODOs. The goal is choosing — and being
// able to justify — the cast, not just making it compile.
//
// Forbidden in every section: the C-style cast (T)x. It silently picks
// among static/const/reinterpret and obscures intent.

#include <bit>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <vector>

// ============================================================================
// Section 1: static_cast — numeric narrowing
// ============================================================================
// Truncate a double to int. The conversion is implicit, but doing it silently
// triggers -Wconversion / -Wnarrowing warnings in real codebases and (more
// importantly) hides the intent. A static_cast is the audit-trail: "yes, I
// know I'm narrowing, this is deliberate."
int truncate_to_int(double d) {
    // TODO: return d cast to int using static_cast.
    return static_cast<int>(d);
}

// ============================================================================
// Section 2: static_cast — void* round-trip
// ============================================================================
// Old C-style callback APIs pass user data as void*. You store a pointer to
// your own type as void*, and recover it on the way back. static_cast handles
// both directions; the round-trip is well-defined as long as the type matches.
struct Context { int counter; };

void* stash(Context& ctx) {
    // TODO: return &ctx cast to void* via static_cast.
    return static_cast<void*>(&ctx);
}

int read_counter(void* opaque) {
    // TODO: recover the Context* from opaque using static_cast and return
    //       the counter. (Assume opaque is non-null and really points to a Context.)
    return static_cast<Context*>(opaque)->counter;
}

// ============================================================================
// Section 3: dynamic_cast — polymorphic downcast
// ============================================================================
// Shape is a polymorphic base (has a virtual function). A Shape* might point
// at any concrete shape. dynamic_cast<Derived*>(base_ptr) returns nullptr if
// the dynamic type isn't Derived (or a class derived from it).
//
// Why not static_cast here? static_cast<Circle*>(shape) would compile even if
// shape actually points to a Square — no runtime check, and you'd dereference
// garbage. The whole point of dynamic_cast is the check.
struct Shape {
    virtual ~Shape() = default;
    virtual double area() const = 0;
};

struct Circle : Shape {
    double r;
    explicit Circle(double r_) : r(r_) {}
    double area() const override { return 3.14159265 * r * r; }
    double radius() const { return r; }
};

struct Square : Shape {
    double s;
    explicit Square(double s_) : s(s_) {}
    double area() const override { return s * s; }
};

// If shape really is a Circle, return its radius. Otherwise return -1.0.
double radius_if_circle(const Shape* shape) {
    // TODO: use dynamic_cast<const Circle*>(shape). If the result is non-null,
    //       return its radius(). Otherwise return -1.0.

    if(auto *c = dynamic_cast<const Circle*>(shape))
        return c->radius();

    return -1.0;
}

// Same idea, but with REFERENCES. dynamic_cast on a reference throws
// std::bad_cast on failure (it can't return "null" because references can't be).
// Wrap the cast in try/catch and return -1.0 on failure.
double radius_if_circle_ref(const Shape& shape) {
    // TODO: try { auto& c = dynamic_cast<const Circle&>(shape); return c.radius(); }
    //       catch (const std::bad_cast&) { return -1.0; }

    try {
        return dynamic_cast<const Circle&>(shape).radius();
    }
    catch (const std::bad_cast&) { return -1.0; }
}

// ============================================================================
// Section 4: const_cast — bridging to a const-incorrect API
// ============================================================================
// Pretend this is a 1980s C function we can't modify. It takes char* but
// (we know from the docs) doesn't actually mutate the buffer.
extern "C" inline std::size_t legacy_strlen(char* s) {
    return std::strlen(s);
}

// We have a std::string (which gives us a const char* via .c_str()). We need
// to call legacy_strlen. The ONLY legitimate use of const_cast is at an
// interface boundary like this — and only because we trust the callee not to
// mutate. (If it did mutate, that'd be UB because the underlying buffer is
// genuinely const-qualified.)
std::size_t safe_strlen(const std::string& s) {
    // TODO: take s.c_str() (which is const char*), const_cast away the const
    //       to get a char*, and pass it to legacy_strlen.

    auto c = const_cast<char*>(s.c_str());
    return legacy_strlen(c);
}

// ============================================================================
// Section 5: std::bit_cast — viewing the bits of a float
// ============================================================================
// IEEE-754 float and uint32_t have the same size and are both trivially
// copyable. std::bit_cast<uint32_t>(f) gives you f's bit pattern as an
// unsigned integer — well-defined, constexpr-friendly, no UB.
//
// The old way was reinterpret_cast<uint32_t&>(f) or a union or memcpy.
// reinterpret_cast violates the strict aliasing rule (float and uint32_t
// don't alias under [basic.lval]) and is UB; the union trick is UB in C++;
// memcpy works but is verbose. bit_cast is the modern answer.
std::uint32_t float_bits(float f) {
    // TODO: return std::bit_cast<std::uint32_t>(f).
    return std::bit_cast<std::uint32_t>(f);
}

// And back the other way. Given a bit pattern, view it as a float.
float bits_as_float(std::uint32_t bits) {
    // TODO: return std::bit_cast<float>(bits).
    return std::bit_cast<float>(bits);
}

// ============================================================================
// main: harness
// ============================================================================
int main() {
    std::cout << "--- Section 1: static_cast numeric narrowing ---\n";
    std::cout << "truncate_to_int(3.7)   = " << truncate_to_int(3.7)
              << "   (expect 3)\n";
    std::cout << "truncate_to_int(-2.9)  = " << truncate_to_int(-2.9)
              << "  (expect -2; truncation is toward zero)\n";

    std::cout << "\n--- Section 2: static_cast void* round-trip ---\n";
    Context ctx{42};
    void* opaque = stash(ctx);
    std::cout << "read_counter(opaque)   = " << read_counter(opaque)
              << "  (expect 42)\n";

    std::cout << "\n--- Section 3: dynamic_cast polymorphic downcast ---\n";
    std::unique_ptr<Shape> a = std::make_unique<Circle>(2.0);
    std::unique_ptr<Shape> b = std::make_unique<Square>(3.0);
    std::cout << "radius_if_circle(circle) = " << radius_if_circle(a.get())
              << "  (expect 2)\n";
    std::cout << "radius_if_circle(square) = " << radius_if_circle(b.get())
              << "  (expect -1)\n";
    std::cout << "radius_if_circle_ref(circle) = " << radius_if_circle_ref(*a)
              << "  (expect 2)\n";
    std::cout << "radius_if_circle_ref(square) = " << radius_if_circle_ref(*b)
              << "  (expect -1)\n";

    std::cout << "\n--- Section 4: const_cast at legacy boundary ---\n";
    std::string s = "hello";
    std::cout << "safe_strlen(\"hello\") = " << safe_strlen(s)
              << "  (expect 5)\n";

    std::cout << "\n--- Section 5: bit_cast float<->uint32 ---\n";
    std::uint32_t one_bits = float_bits(1.0f);
    std::cout << "float_bits(1.0f)        = 0x" << std::hex << one_bits << std::dec
              << "  (expect 0x3f800000 — IEEE-754 for 1.0)\n";
    std::cout << "bits_as_float(0x40490fdb) = " << bits_as_float(0x40490fdbu)
              << "  (expect ~3.14159)\n";
    float roundtrip = bits_as_float(float_bits(2.5f));
    std::cout << "roundtrip 2.5f          = " << roundtrip
              << "  (expect 2.5)\n";
}
