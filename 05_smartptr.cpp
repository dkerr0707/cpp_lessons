// Exercise: use each smart pointer for the job it's meant for.
// Build:  ./build.sh 05_smartptr -r
//
//   std::unique_ptr<T> — sole owner. Zero overhead over a raw pointer.
//                        Move to transfer ownership; cannot copy.
//   std::shared_ptr<T> — shared ownership, reference-counted. Object dies
//                        when the last shared_ptr goes away.
//   std::weak_ptr<T>   — non-owning observer of a shared_ptr's object.
//                        Doesn't keep it alive; .lock() to use it safely.
//
// Fill in the four functions marked TODO. main() exercises each.

#include <iostream>
#include <memory>

namespace {
struct Widget {
    int id;
    explicit Widget(int i) : id(i) { std::cout << "  Widget(" << id << ") born\n"; }
    ~Widget()                       { std::cout << "  Widget(" << id << ") died\n"; }
    int value() const { return id * 10; }
};
}

// 1. unique_ptr as a factory: create a Widget and hand back sole ownership.
//    (Prefer std::make_unique over `new`.)
std::unique_ptr<Widget> make_widget(int id) {

    return std::make_unique<Widget>(id);

}

// 2. unique_ptr as a sink: take ownership by value, use it, and let it die
//    when this function returns. Return the widget's value().
int consume(std::unique_ptr<Widget> w) {
    return w->value();
}

// 3. shared_ptr: make a SECOND owner of the same object that w points to,
//    then return how many owners currently exist (use_count). The second
//    owner is local, so it should be released again when this returns.
long owner_count_with_extra(const std::shared_ptr<Widget>& w) {

    auto second_ptr(w);
    
    return second_ptr.use_count();
}

// 4. weak_ptr: observe without owning. If the object is still alive, return
//    its value(); if it has been destroyed, return -1. (Hint: .lock())
int observe(const std::weak_ptr<Widget>& w) {
    if (auto shared_locked = w.lock())
        return shared_locked->value();

    return -1;
}

int main() {
    std::cout << "--- unique_ptr: factory + sink ---\n";
    std::unique_ptr<Widget> u = make_widget(1);            // Widget(1) born
    std::cout << "u->value() = " << u->value() << "  (expect 10)\n";
    int v = consume(std::move(u));                         // Widget(1) dies inside consume
    std::cout << "consume returned " << v << "  (expect 10)\n";
    std::cout << "u is now " << (u ? "non-null" : "null") << "  (expect null)\n";

    std::cout << "\n--- shared_ptr: shared ownership ---\n";
    std::shared_ptr<Widget> s = std::make_shared<Widget>(2);   // Widget(2) born
    std::cout << "use_count           = " << s.use_count()              << "  (expect 1)\n";
    std::cout << "count with extra    = " << owner_count_with_extra(s)  << "  (expect 2)\n";
    std::cout << "use_count after     = " << s.use_count()              << "  (expect 1)\n";

    std::cout << "\n--- weak_ptr: non-owning observer ---\n";
    std::weak_ptr<Widget> w = s;
    std::cout << "observe (alive)   = " << observe(w)   << "  (expect 20)\n";
    std::cout << "expired?          = " << w.expired()  << "  (expect 0)\n";
    s.reset();                                              // last owner gone -> Widget(2) dies
    std::cout << "after s.reset():\n";
    std::cout << "observe (expired) = " << observe(w)   << "  (expect -1)\n";
    std::cout << "expired?          = " << w.expired()  << "  (expect 1)\n";
}
