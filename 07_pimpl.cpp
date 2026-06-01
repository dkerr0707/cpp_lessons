// Exercise: the pimpl idiom (pointer to implementation).
// Build:  ./build.sh 07_pimpl -r
//
// Pimpl moves a class's private members into an opaque struct defined in the
// .cpp, leaving the header with only a forward declaration and a
// std::unique_ptr<Impl>. The payoff:
//
//   - Consumers don't recompile when the private layout changes.
//   - Private members' headers (here: <vector>, <string>) stop leaking into
//     every translation unit that includes Logger's interface.
//   - Strong insulation: callers literally cannot see Impl.
//
// Cost: one heap allocation per object, one indirection per access, and you
// have to be careful about where special member functions are *defined* (the
// "incomplete type" trap — see below).
//
// We can't actually split this into two TUs because build.sh builds one file
// at a time, so we'll fake it: pretend everything above the "==== logger.cpp"
// banner lives in logger.hpp and only that part is visible to callers.

#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

// ========================= logger.hpp =====================================
// What a consumer sees. Note: NO <vector>, NO <string> here in real life —
// those would live in the .cpp only. We include them above for the demo.

class Logger {
public:
    Logger();

    // The "incomplete type" trap: ~Logger(), the move ops, and any other
    // special member that touches Impl (even implicitly, via unique_ptr's
    // destructor) MUST be declared here and defined out-of-line below, AFTER
    // struct Impl is complete. If you write `= default` in the class body,
    // the compiler will instantiate unique_ptr<Impl>'s destructor here, where
    // Impl is still incomplete -> "deletion of incomplete type" error.

    // TODO: declare the destructor (just the declaration; definition goes below).
    ~Logger();

    // Copy is deleted: unique_ptr<Impl> makes it move-only anyway, but being
    // explicit documents the intent.
    Logger(const Logger&)            = delete;
    Logger& operator=(const Logger&) = delete;

    // TODO: declare the move constructor and move assignment (noexcept).
    //       Defined out-of-line below as = default.
    Logger(Logger&&) noexcept;
    Logger& operator=(Logger&&) noexcept;


    // Public API. (These you can define here OR below — your choice. They
    // touch impl_->..., so they need Impl to be complete; if you put them
    // here you'll get the same incomplete-type error.)
    void append(std::string_view line);
    void dump(std::ostream& os) const;
    std::size_t size() const;

private:
    // TODO: forward-declare the implementation type and hold one by unique_ptr.
    //       The whole point: this header does NOT see Impl's members.
    struct Impl;
    std::unique_ptr<Impl> impl_;

};

// ========================= logger.cpp =====================================
// What only the implementation TU sees. Impl's full definition lives here.

// TODO: define struct Logger::Impl with a single member: a std::vector of
//       std::string (one entry per appended line).
struct Logger::Impl {
    std::vector<std::string> member;
};

// TODO: define Logger::Logger() — allocate the Impl with std::make_unique.
Logger::Logger() :
    impl_(std::make_unique<Logger::Impl>()) {}

// TODO: define ~Logger() = default, move ctor = default, move assign = default.
//       These work HERE because Impl is now a complete type.
Logger::~Logger() = default;
Logger::Logger(Logger&&) noexcept = default;
Logger& Logger::operator=(Logger&&) noexcept = default;


// TODO: define append, dump, size to forward to impl_->lines.
//       (dump: print each line followed by '\n'.)
void Logger::append(std::string_view line) {
    impl_->member.emplace_back(line); 
}

void Logger::dump(std::ostream& os) const {
    for(auto& e: impl_->member)
        os << e << '\n';
}

std::size_t Logger::size() const {
    return impl_ ? impl_->member.size() : 0; 
}

// ========================= main ===========================================
// Note: main() only uses Logger's PUBLIC API. It has no idea Logger holds a
// vector of strings. If you later changed Impl to a std::deque, a file, a
// ring buffer — none of this code below would need to change OR recompile
// (in the real two-TU version).

int main() {
    std::cout << "--- construct & append ---\n";
    Logger lg;
    lg.append("alpha");
    lg.append("beta");
    lg.append("gamma");
    std::cout << "size: " << lg.size() << "  (expect 3)\n";
    lg.dump(std::cout);

    std::cout << "\n--- move-construct ---\n";
    Logger lg2 = std::move(lg);
    std::cout << "lg2.size: " << lg2.size() << "  (expect 3)\n";
    std::cout << "lg.size:  " << lg.size()  << "  (expect 0 — moved-from)\n";
    lg2.dump(std::cout);

    std::cout << "\n--- move-assign ---\n";
    Logger lg3;
    lg3.append("placeholder");
    lg3 = std::move(lg2);
    std::cout << "lg3.size: " << lg3.size() << "  (expect 3)\n";
    lg3.dump(std::cout);

    std::cout << "\n--- copy is deleted (uncomment to confirm) ---\n";
    // Logger bad = lg3;   // <- should fail to compile
    // Logger bad2; bad2 = lg3; // <- should fail to compile
    std::cout << "ok\n";

}
