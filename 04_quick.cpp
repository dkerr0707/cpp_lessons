// Quick exercise: squares of the even numbers, in order.
// Themes: views::filter + views::transform, materializing back to a vector.
// Build:  ./build.sh 04_quick -r

#include <iostream>
#include <ranges>
#include <vector>

// Return a vector containing the squares of the even elements of v,
// preserving order.
//   squares_of_even({1, 2, 3, 4, 5}) -> {4, 16}
//   squares_of_even({7, 7, 7})       -> {}
//   squares_of_even({})              -> {}
//   squares_of_even({-2, -3, 6})     -> {4, 36}
std::vector<int> squares_of_even(const std::vector<int>& v) {

    std::vector<int> result;
    for (int e: v)
        if (e % 2 == 0)
            result.push_back(e*e);
        
    return result;
}

namespace {
void print_vec(const std::vector<int>& v) {
    std::cout << '{';
    for (std::size_t i = 0; i < v.size(); ++i) std::cout << (i ? "," : "") << v[i];
    std::cout << '}';
}
}

int main() {
    std::cout << "{1,2,3,4,5}  -> "; print_vec(squares_of_even({1,2,3,4,5})); std::cout << "  (expect {4,16})\n";
    std::cout << "{7,7,7}      -> "; print_vec(squares_of_even({7,7,7}));     std::cout << "  (expect {})\n";
    std::cout << "{}           -> "; print_vec(squares_of_even({}));          std::cout << "  (expect {})\n";
    std::cout << "{-2,-3,6}    -> "; print_vec(squares_of_even({-2,-3,6}));   std::cout << "  (expect {4,36})\n";
}
