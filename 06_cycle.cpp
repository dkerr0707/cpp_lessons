// Exercise: break a reference cycle with weak_ptr.
// Build:  ./build.sh 06_cycle -r
//
// Two shared_ptrs pointing at each other form a cycle: each keeps the other
// alive, so neither reference count ever reaches 0 -> the objects leak. This
// is THE classic failure mode of shared_ptr, and the reason weak_ptr exists.
//
// As written, this program LEAKS (you'll see no "died" lines, and the
// observer reports the Parent is still alive after its scope). Your job:
// change exactly ONE pointer so the cycle breaks and both destructors run.
// Do not change main().
//
// After you think it's fixed, also confirm with the sanitizer:
//   g++ -std=c++23 -g -fsanitize=address 06_cycle.cpp -o bin/06_cycle_asan && ./bin/06_cycle_asan

#include <iostream>
#include <memory>

struct Child;   // forward declaration

struct Parent {
    std::shared_ptr<Child> child;     // Parent owns its Child
    ~Parent() { std::cout << "  Parent died\n"; }
};

struct Child {
    // TODO: this back-pointer to Parent is the edge that closes the cycle.
    //       A "back-edge" / non-owning reference should not be an OWNING
    //       pointer. Change its type so it observes the Parent without
    //       keeping it alive. (The assignment `c->parent = p;` in main keeps
    //       working unchanged after the right edit.)
    std::weak_ptr<Parent> parent;
    ~Child() { std::cout << "  Child died\n"; }
};

int main() {
    std::weak_ptr<Parent> observer;   // watches whether Parent outlives the scope

    std::cout << "--- entering scope ---\n";
    {
        auto p = std::make_shared<Parent>();
        auto c = std::make_shared<Child>();
        p->child  = c;     // Parent -> Child
        c->parent = p;     // Child  -> Parent  (the back-edge)
        observer  = p;

        std::cout << "  inside:  Parent use_count   = " << p.use_count()
                  << "   (1 = no cycle, 2 = cycle)\n";
        std::cout << "  inside:  observer.expired() = " << observer.expired()
                  << "   (expect 0 — Parent is alive here)\n";
    }   // p and c leave scope. If the cycle is broken, both objects die now.

    std::cout << "--- left scope ---\n";
    std::cout << "  observer.expired() = " << observer.expired()
              << "   (want 1; if 0, the Parent leaked)\n";

    if (observer.expired())
        std::cout << "RESULT: no leak — the cycle was broken.\n";
    else
        std::cout << "RESULT: LEAK — Parent still alive after its scope.\n";
}
