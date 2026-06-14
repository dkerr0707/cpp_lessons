// Exercise: basic concurrency primitives.
// Build:  ./build.sh 10_threading -r
//
// std::thread, mutex/lock_guard, atomic, condition_variable,
// std::async + future, and C++20's std::jthread + stop_token.
//
// The lesson is the *layering*. The bottom layer is raw `std::thread` plus
// manual synchronization (mutex / atomic / condition_variable). The higher
// layers (`std::async`, `std::jthread`) bundle common patterns — return
// values + exception propagation, auto-join + cooperative cancellation —
// on top of that bottom layer. Reach for the higher layer first; drop down
// only when you need to.

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <future>
#include <iostream>
#include <mutex>
#include <optional>
#include <stop_token>
#include <thread>
#include <vector>

// ===========================================================================
// Section 1: std::thread — spawn and join
// ===========================================================================
// std::thread runs a callable in a new OS thread. You MUST either join() or
// detach() before the std::thread object is destroyed; otherwise the dtor
// calls std::terminate(). Joining is almost always right — detach is a
// footgun (the OS thread outlives the std::thread handle, with no way to
// wait for or signal it, and it'll happily reference dead stack variables).
//
// There's no shared mutable state below: each worker writes a *distinct*
// slot in the output buffer. That's the easiest concurrency win — data
// parallelism over disjoint regions needs no synchronization at all.

// Spawn `n` threads. Thread i writes (i * i) into out[i]. Then join them all.
void parallel_squares(std::vector<int>& out, std::size_t n) {
    // TODO: out.resize(n);
    //       std::vector<std::thread> threads;
    //       for i in 0..n: threads.emplace_back([&out, i]{ out[i] = i * i; });
    //       for each t in threads: t.join();

    out.resize(n);
    std::vector<std::thread> threads;
    for (size_t i = 0; i < n; i++)
        threads.emplace_back([&out, i]{ out[i] = i * i; });

    for (std::thread& t: threads) t.join();

}

// ===========================================================================
// Section 2: std::mutex — protecting shared state
// ===========================================================================
// When threads share mutable data, you have to serialize access. The classic
// way is a mutex. std::lock_guard is an RAII wrapper that locks on
// construction and unlocks on destruction — so the lock is held for exactly
// the lifetime of the guard, and exceptions can't leak it.
//
// For multiple mutexes at once, std::scoped_lock acquires all of them in a
// single call using a deadlock-avoidance algorithm. Always prefer it over
// hand-rolled lock/unlock pairs.

struct Counter {
    long value = 0;
    std::mutex m;
};

// Each of `n` threads increments `c.value` `iters` times. Without a lock the
// final value is unpredictable (data race = UB). Fix with a lock_guard.
void contended_increment(Counter& c, std::size_t n, std::size_t iters) {
    // TODO: spawn n threads, each doing iters iterations of:
    //         std::lock_guard<std::mutex> g(c.m);
    //         c.value += 1;
    //       Join all threads.

    std::vector<std::thread> threads;
    for (size_t i = 0; i < n; i++) {
        threads.emplace_back([&c, iters]{ 
            for(size_t j = 0; j < iters; j++){
                std::lock_guard<std::mutex> lock(c.m);
                c.value += 1;
            } 
        });
    }

    for (auto& t: threads) t.join();

}

// ===========================================================================
// Section 3: std::atomic — lock-free for simple operations
// ===========================================================================
// For a single integer counter you don't need a full mutex. std::atomic<T>
// gives you load/store/fetch_add etc. that are well-defined under concurrent
// access (no data race, no UB). On x86 fetch_add on a 64-bit int compiles to
// a single `lock xadd` instruction — one bus-locked cycle, no kernel call.
//
// Rule of thumb: use atomics when the operation is a single read-modify-write
// (counters, flags, sequence numbers). Use a mutex when you need a multi-step
// critical section that touches several variables.

void atomic_increment(std::atomic<long>& counter, std::size_t n, std::size_t iters) {
    // TODO: spawn n threads, each doing `iters` of `++counter` (or
    //       counter.fetch_add(1)). Join all.

    std::vector<std::thread> threads;
    for (size_t i = 0; i < n; i++) {
        threads.emplace_back( [&counter, iters] {
            for (size_t j = 0; j < iters; j++) counter.fetch_add(1);
        });
    }

    for (auto& t: threads) t.join();

}

// ===========================================================================
// Section 4: std::condition_variable — producer/consumer
// ===========================================================================
// A condition_variable lets one thread sleep until another signals a change
// in shared state. You always use it WITH a mutex AND a predicate. The
// predicate form `cv.wait(lock, pred)` handles spurious wakeups for you
// (the wait loop is built in) — never write a bare `cv.wait(lock)`.
//
// Pattern below: a single-slot buffer. Producer writes into it; consumer
// reads and clears it. Empty/full state is held in a std::optional.
//
// Note on notify ordering: it's traditional (and slightly more efficient) to
// unlock the mutex *before* calling notify_one — otherwise the woken thread
// may immediately re-block on the still-held lock. Not a correctness issue,
// just a minor efficiency thing.

struct Channel {
    std::optional<int> slot;
    std::mutex m;
    std::condition_variable cv;
};

// Push `value` into the channel. Wait while it's already full.
void send(Channel& ch, int value) {
    // TODO: std::unique_lock lock(ch.m);
    //       ch.cv.wait(lock, [&]{ return !ch.slot.has_value(); });
    //       ch.slot = value;
    //       lock.unlock();
    //       ch.cv.notify_one();

    std::unique_lock<std::mutex> lock(ch.m);
    ch.cv.wait (lock, [&]{ return !ch.slot.has_value(); });
    ch.slot = value;
    lock.unlock();
    ch.cv.notify_one();
}

// Pop a value from the channel. Wait while it's empty.
int recv(Channel& ch) {
    // TODO: std::unique_lock lock(ch.m);
    //       ch.cv.wait(lock, [&]{ return ch.slot.has_value(); });
    //       int v = *ch.slot;
    //       ch.slot.reset();
    //       lock.unlock();
    //       ch.cv.notify_one();
    //       return v;

    std::unique_lock lock(ch.m);
    ch.cv.wait (lock, [&]{ return ch.slot.has_value(); });
    int v = *ch.slot;
    ch.slot.reset();
    lock.unlock();
    ch.cv.notify_one();

    return v;
}

// ===========================================================================
// Section 5: std::async — task-based parallelism with a return value
// ===========================================================================
// std::async launches a callable and hands you a std::future<T>. Calling
// .get() on the future blocks until the task is done and returns its result.
// If the task threw, .get() rethrows that exception in the calling thread —
// MUCH nicer than std::thread, where an unhandled exception in the worker
// terminates the program.
//
// Pass std::launch::async to force a fresh thread. The default policy is
// implementation-defined and can be "deferred" — the callable runs lazily
// on the calling thread when you call .get(), giving you zero parallelism.
// Always be explicit.

// Launch `compute(input)` asynchronously and return a future for its result.
std::future<int> spawn_compute(int (*compute)(int), int input) {
    // TODO: return std::async(std::launch::async, compute, input);
    return std::async (std::launch::async, compute, input);
}

// ===========================================================================
// Section 6: std::jthread + stop_token (C++20)
// ===========================================================================
// std::jthread is std::thread plus two things:
//   1. it auto-joins in its destructor (no more "forgot to join → terminate"),
//   2. it integrates a std::stop_token for cooperative cancellation.
// The worker's callable takes a std::stop_token by first parameter; it
// periodically checks st.stop_requested() and exits its loop when set. The
// jthread destructor calls request_stop() and then join() — so dropping the
// handle is enough to wind the worker down cleanly.
//
// This is the modern default for "run a long-lived background worker".

// Spawn a jthread that increments `counter` until its stop_token is requested.
// The caller will sleep a bit and then drop the returned jthread — its dtor
// requests stop and joins.
std::jthread spinner(std::atomic<long>& counter) {
    // TODO: return std::jthread([&counter](std::stop_token st){
    //           while (!st.stop_requested()) {
    //               counter.fetch_add(1, std::memory_order_relaxed);
    //           }
    //       });

    return std::jthread ([&counter] (std::stop_token st) {
        while (!st.stop_requested()) {
            counter.fetch_add (1, std::memory_order_relaxed);
        }
    });
}

// ===========================================================================
// main: harness
// ===========================================================================
int main() {
    std::cout << "--- Section 1: spawn & join ---\n";
    std::vector<int> squares;
    parallel_squares(squares, 5);
    std::cout << "squares: ";
    for (int x : squares) std::cout << x << ' ';
    std::cout << "  (expect 0 1 4 9 16)\n";

    std::cout << "\n--- Section 2: contended counter with mutex ---\n";
    Counter c;
    contended_increment(c, 4, 100000);
    std::cout << "c.value = " << c.value << "  (expect 400000)\n";

    std::cout << "\n--- Section 3: atomic counter ---\n";
    std::atomic<long> ac{0};
    atomic_increment(ac, 4, 100000);
    std::cout << "ac = " << ac.load() << "       (expect 400000)\n";

    std::cout << "\n--- Section 4: channel (single-slot producer/consumer) ---\n";
    Channel ch;
    std::thread prod([&]{
            for (int i = 1; i <= 5; ++i) send(ch, i * 10);
            });
    std::thread cons([&]{
            for (int i = 0; i < 5; ++i) {
            int v = recv(ch);
            std::cout << "  got " << v << '\n';
            }
            });
    prod.join();
    cons.join();
    std::cout << "  (expect 10 20 30 40 50 — single-slot channel forces order)\n";

    std::cout << "\n--- Section 5: async + future ---\n";
    auto fut = spawn_compute([](int n){ return n * n; }, 9);
    std::cout << "future.get() = " << fut.get() << "  (expect 81)\n";

    std::cout << "\n--- Section 6: jthread + stop_token ---\n";
    std::atomic<long> spincount{0};
    {
        auto jt = spinner(spincount);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }  // jt's dtor: request_stop() then join()
    std::cout << "spincount > 0: " << (spincount.load() > 0)
        << "  (expect 1 — some work happened before stop was requested)\n";
}
