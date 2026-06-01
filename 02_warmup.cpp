// Warm-up exercises, round 2.
// Themes: std::optional, std::span, std::string_view, ranges/algorithms.
// Build:  ./build.sh 02_warmup -r

#include <iostream>
#include <optional>
#include <span>
#include <string_view>
#include <vector>
#include <algorithm>
#include <ranges>

// 1. Return the largest value in v, or std::nullopt if v is empty.
//    max_in({3, 1, 4, 1, 5, 9, 2, 6}) -> 9
//    max_in({})                       -> nullopt
std::optional<int> max_in(const std::vector<int>& v) {
    if (v.empty()) return std::nullopt;
    return std::ranges::max(v);
}

// 2. Return a sorted copy of v with duplicates removed.
//    Parameter is taken by value on purpose — feel free to mutate and
//    return it (move-in, mutate, move-out).
//    unique_sorted({4, 1, 4, 2, 1, 3}) -> {1, 2, 3, 4}
//    unique_sorted({})                 -> {}
std::vector<int> unique_sorted(std::vector<int> v) {
    std::ranges::sort(v);
    auto [newEnd, last] = std::ranges::unique(v);
    v.erase(newEnd, last);
    return v;
}

// 3. Arithmetic mean, or std::nullopt for an empty range.
//    Parameter is std::span<const double> — binds to any contiguous range
//    of doubles (vector, std::array, C array). No allocation, no copy.
//    average({1.0, 2.0, 3.0, 4.0}) -> 2.5
//    average({})                   -> nullopt
std::optional<double> average(std::span<const double> values) {
    if (values.empty()) return std::nullopt;

    double sum = std::ranges::fold_left(values, 0.0, std::plus<>{});
    return sum / values.size(); 
}

// 4. Count whitespace-separated words in s.
//    Whitespace: any run of ' ', '\t', '\n'. Leading/trailing/extra
//    whitespace is fine. Empty / all-whitespace input -> 0.
//    count_words("the quick brown fox") -> 4
//    count_words("  hello   world  ")   -> 2
//    count_words("")                    -> 0
//    count_words("   \t\n  ")           -> 0
std::size_t count_words(std::string_view s) {

    std::size_t count = 0;
    bool in_word = false;
    for (char c: s) {
        bool ws = (c == ' ' || c == '\t' || c== '\n');
        if (!ws && !in_word) count++;
        in_word = !ws;
    }

    return count;
}

namespace {
void print_opt(std::optional<int> o)    { if (o) std::cout << *o; else std::cout << "nullopt"; }
void print_opt(std::optional<double> o) { if (o) std::cout << *o; else std::cout << "nullopt"; }

void print_vec(const std::vector<int>& v) {
    std::cout << '{';
    for (std::size_t i = 0; i < v.size(); ++i) std::cout << (i ? "," : "") << v[i];
    std::cout << '}';
}
}

int main() {
    std::cout << "--- max_in ---\n";
    std::cout << "{3,1,4,1,5,9,2,6} -> "; print_opt(max_in({3,1,4,1,5,9,2,6})); std::cout << "  (expect 9)\n";
    std::cout << "{}                -> "; print_opt(max_in({}));                std::cout << "  (expect nullopt)\n";

    std::cout << "\n--- unique_sorted ---\n";
    std::cout << "{4,1,4,2,1,3} -> "; print_vec(unique_sorted({4,1,4,2,1,3})); std::cout << "  (expect {1,2,3,4})\n";
    std::cout << "{}            -> "; print_vec(unique_sorted({}));            std::cout << "  (expect {})\n";

    std::cout << "\n--- average ---\n";
    std::vector<double> v = {1.0, 2.0, 3.0, 4.0};
    std::vector<double> empty;
    std::cout << "{1,2,3,4} -> "; print_opt(average(v));     std::cout << "  (expect 2.5)\n";
    std::cout << "{}        -> "; print_opt(average(empty)); std::cout << "  (expect nullopt)\n";

    std::cout << "\n--- count_words ---\n";
    std::cout << "\"the quick brown fox\" -> " << count_words("the quick brown fox") << "  (expect 4)\n";
    std::cout << "\"  hello   world  \"   -> " << count_words("  hello   world  ")   << "  (expect 2)\n";
    std::cout << "\"\"                    -> " << count_words("")                    << "  (expect 0)\n";
    std::cout << "\"   \\t\\n  \"            -> " << count_words("   \t\n  ")           << "  (expect 0)\n";
}
