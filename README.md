# C++ Refresher

Progressive C++ exercises — one self-contained file per topic, ordered roughly
from warmup to intermediate modern-C++ machinery.

## Build

```
./build.sh <name>      # build one (e.g. ./build.sh 03_warmup)
./build.sh <name> -r   # build and run
./build.sh <name> -d   # build with -O0 (for a clean gdb experience)
./build.sh             # build all
```

Standard: **C++23**. Flags: `-Wall -Wextra -Wpedantic -O2 -g`. Compiler: `g++`.

## Exercises

| #  | File              | Topics |
|----|-------------------|--------|
| 01 | `01_warmup.cpp`   | FizzBuzz, digit sum, string reverse — loops, strings, `std::ranges::reverse` vs `views::reverse` |
| 02 | `02_warmup.cpp`   | `std::optional`, `std::span`, `std::string_view`, `ranges::max` / `sort` / `unique` / `fold_left`; when ranges is the wrong tool (tokenization) |
| 03 | `03_warmup.cpp`   | Rule of five for a move-only `Buffer` (RAII, `noexcept` moves, self-move safety, swap-based move-assign) |
| 04 | `04_quick.cpp`    | `views::filter` + `views::transform` materialized back to a `vector` |
| 05 | `05_smartptr.cpp` | `unique_ptr` / `shared_ptr` / `weak_ptr` used each for its purpose (factory, sink, shared ownership, `.lock()`) |
| 06 | `06_cycle.cpp`    | Breaking a `shared_ptr` parent↔child cycle with `weak_ptr`; verified leak-free with ASan |
| 07 | `07_pimpl.cpp`    | pimpl idiom: forward-declared `Impl`, `unique_ptr<Impl>`, special members declared in the class and defined out-of-line after `Impl` is complete |
| 08 | `08_casts.cpp`    | `static_cast`, `dynamic_cast` (pointer→null, ref→`std::bad_cast`), `const_cast` at a legacy boundary, `std::bit_cast` for float↔uint32 |
| 09 | `09_pointers.cpp` | Raw-pointer mechanics: address-of / indirection, pointer arithmetic over arrays, two-pointer reverse, the const-pointer maze, `->` vs `(*p).`, C-string `strlen` |
| 10 | `10_threading.cpp` | Concurrency primitives: `std::thread` spawn/join, `mutex`+`lock_guard`, `std::atomic`, `condition_variable` producer/consumer, `std::async`+`future`, `std::jthread`+`stop_token` |
| 11 | `11_gdb.cpp`       | Driving gdb: breakpoints (line, function, conditional), step/next/finish, `info locals`/`args`, backtrace + frame navigation across a crash and a recursion, watchpoint on a stack location |

## How this was built

I worked through these with [Claude Code](https://claude.com/claude-code) as a
collaborator in **review mode**: Claude scaffolded each exercise file (problem
statement, function signatures, expected-output harness in `main()`, brief
exposition on the relevant language feature) and left the function bodies as
`TODO`s. I filled in the implementations, then Claude reviewed for correctness,
idiom, and subtle traps (e.g. the moved-from `unique_ptr<Impl>` null in
`07_pimpl.cpp`, or the dynamic-cast reference-vs-pointer asymmetry in
`08_casts.cpp`).

A good fit for refreshing a language after a long break: the scaffolding kept
me writing code (not boilerplate) and the reviews surfaced things I'd have
otherwise missed or only half-remembered.

## Side projects

- `tic_tac_toe/` — a console tic-tac-toe with its own `build.sh`.

## Layout notes

- `bin/` is gitignored — `build.sh` puts compiled binaries there.
- Each `.cpp` is its own translation unit; `build.sh` builds one file at a time.
  Multi-TU exercises (e.g. real pimpl with a `.hpp`/`.cpp` split) fake the
  separation inside a single file with header/cpp banners.
