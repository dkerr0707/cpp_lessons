// Exercise: driving gdb.
// Build:  ./build.sh 11_gdb -d        (the -d flag drops -O2 so locals stay
//                                       visible instead of <optimized out>)
// Then:   gdb bin/11_gdb
//
// Each "mission" below is a question to answer by DRIVING GDB, not by
// reading the code. Report your gdb commands and your findings; I'll review.
//
// =============================================================================
// Quick command cheat sheet (the ones you'll need)
// =============================================================================
//
//   Breakpoints
//     b main                    break at function
//     b 11_gdb.cpp:42           break at file:line
//     b sum_n                   break at function (resolves to first line)
//     b sum_n if i == 3         conditional breakpoint
//     info b                    list breakpoints
//     d 2                       delete breakpoint #2
//
//   Execution
//     r                         run
//     c                         continue
//     n                         next (step OVER calls)
//     s                         step (step INTO calls)
//     fin                       finish (run until current function returns)
//     u <line>                  until that line (good for jumping past a loop)
//
//   Inspection
//     p <expr>                  print     (p x, p arr[3], p *node, p/x 255)
//     p/x  p/d  p/t  p/c        hex, decimal, binary, char formats
//     info locals               all locals in current frame
//     info args                 all function arguments
//     display <expr>            auto-print <expr> every time we stop
//
//   Stack
//     bt                        backtrace
//     up    down                move one frame up / down
//     frame N                   jump to frame N
//     info frame                details about the current frame
//
//   Watchpoints (heavy — break when a value changes)
//     watch <expr>              break when expr changes
//     rwatch <expr>             break when expr is read
//     awatch <expr>             break on read OR write
//
//   Memory
//     x/4xw &arr[0]             4 words, hex
//     x/16cb &s[0]              16 bytes as chars
//
//   TUI / misc
//     layout src                show source pane (Ctrl-X-A toggles)
//     q                         quit
//
// =============================================================================

#include <iostream>

// =============================================================================
// Mission 1: breakpoint + step + inspect a loop
// =============================================================================
// Run gdb on the binary. Set a breakpoint on the line `total += p[i];` inside
// sum_n. `run`. The program will stop there on the first iteration.
//
// (1a) What does `info locals` show? (you should see i, total, and maybe more)
// (1b) Use `c` to advance through iterations. After the THIRD time the
//      breakpoint fires (so just after adding p[2]), what are i and total?
// (1c) Now replace your breakpoint with a CONDITIONAL one that only fires
//      when i == 3:
//          d                       (delete all breakpoints)
//          b 11_gdb.cpp:<line> if i == 3
//          r
//      What is `total` when it stops?
long sum_n(const int* p, int n) {
    long total = 0;
    for (int i = 0; i < n; ++i) {
        total += p[i];
    }
    return total;
}

// =============================================================================
// Mission 2: crash + backtrace + frame navigation
// =============================================================================
// caller_one() calls caller_two() calls inner_deref(nullptr). The deref
// crashes. Run the program in gdb until the crash (just `r` — no breakpoints
// needed). gdb will stop on the SIGSEGV.
//
// (2a) Run `bt`. How many frames deep are you, and what's the function in
//      frame 0?
// (2b) `frame 2` (or `up` twice) to get to caller_two. Run `info locals`.
//      What is the value of `local`?
// (2c) Still in caller_two's frame, run `p nope`. What does it print, and
//      why is that information useful at a crash site?
int inner_deref(int* p) {
    return *p;
}
int caller_two() {
    int local = 42;
    int* nope = nullptr;
    return local + inner_deref(nope);
}
int caller_one() {
    return caller_two() * 2;
}

// =============================================================================
// Mission 3: watchpoint — find the line that writes a wrong value
// =============================================================================
// corrupt_buffer fills `buf` with what looks like a clean pattern, then ONE
// line writes a surprising value into buf[3]. Use a watchpoint to find that
// line without reading the source above.
//
// Recipe:
//     b corrupt_buffer
//     r
//     watch buf[3]
//     c
// Each time buf[3] is written, gdb stops and shows "Old value" / "New value".
//
// (3a) How many times does buf[3] get written, and at which file:line is
//      each write?
// (3b) Which of those writes looks "wrong" (i.e., breaks the pattern set by
//      the surrounding stores)? What value does it write?
//
// (Why this matters: in real code you don't know where a bad write comes
// from. A watchpoint catches the writer red-handed even if it's deep in
// some other function.)
void corrupt_buffer(int* buf) {
    buf[0] = 0;
    buf[1] = 1;
    buf[2] = 4;
    buf[3] = 9;
    buf[4] = 16;
    buf[3] = 999;
    buf[5] = 25;
}

// =============================================================================
// Mission 4: recursion + conditional breakpoint + frame chain
// =============================================================================
// factorial(6) recurses down to factorial(1). Stop at the base case using a
// conditional breakpoint, then walk the call chain.
//
//     b factorial if n == 1
//     c
//
// (4a) `bt` — how many factorial frames are on the stack?
// (4b) `frame 1` (one level up from the base case). What is n there?
// (4c) Walk all the way up with `up` (or `frame N`). What is n in the
//      OUTERMOST factorial frame — i.e., the one called from main?
// (4d) From the outermost factorial frame, run `fin`. What value gets
//      printed as the return value, and where does execution stop?
long factorial(long n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

// =============================================================================
// main: drives all four missions. The output is mostly irrelevant — the
// "real" output is what you observe in gdb. The program will crash inside
// Mission 2's call chain at the end, so don't be alarmed when you see no
// final newline outside gdb.
// =============================================================================
int main() {
    int arr[] = {1, 2, 3, 4, 5};
    std::cout << "sum_n: " << sum_n(arr, 5) << '\n';          // Mission 1

    int buf[6]{};
    corrupt_buffer(buf);                                      // Mission 3
    std::cout << "buf[3] after corrupt = " << buf[3] << '\n';

    std::cout << "6! = " << factorial(6) << '\n';             // Mission 4

    std::cout << "About to crash (Mission 2)...\n";
    caller_one();                                             // Mission 2
    std::cout << "(this line never prints)\n";
}
