// Warm-up exercises. Fill in each TODO.
// Build:  g++ -std=c++20 -Wall -Wextra -o 01_warmup 01_warmup.cpp
// Run:    ./01_warmup

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

// 15
// 1, 2, Fizz, 4, Buzz, Fizz, 7, 8, Fizz, Buzz, 11, Fizz, 13, 14, FizzBuzz

// 1. Classic FizzBuzz.
//    For n in [1..limit]:
//      multiple of 3 and 5 -> "FizzBuzz"
//      multiple of 3       -> "Fizz"
//      multiple of 5       -> "Buzz"
//      otherwise           -> the number
//    Print each on its own line.
void fizzbuzz(int limit) {
    for (int i = 1; i <= limit; i++) {
        std::string s;
        if (i % 3 == 0) s += "Fizz";
        if (i % 5 == 0) s += "Buzz";
        if (s.empty()) std::cout << i << "\n";
        else std::cout << s << "\n";
    }
}

// 11
// 11 % 10
// 11 / 10

// 2. Sum the digits of a non-negative integer.
//    sum_of_digits(1729) -> 1+7+2+9 = 19
int sum_of_digits(int n) {

    int sum = 0;
    while (n > 0){
        sum += n % 10;
        n /= 10;
    }
    return sum;
}

// hello
// h ''
// e 'h'

// 3. Return a reversed copy of s.
//    reverse_string("hello") -> "olleh"
std::string reverse_string(const std::string& s) {
    std::string out(s);
    std::ranges::reverse(out);
    return out;
}
int main() {
    std::cout << "--- FizzBuzz to 15 ---\n";
    fizzbuzz(15);

    std::cout << "\n--- sum_of_digits ---\n";
    int sum1 = sum_of_digits(1729);
    std::cout << "1729 -> " << sum1 << "  (expect 19)\n";
    int sum2 = sum_of_digits(0);
    std::cout << "0    -> " << sum2 << "  (expect 0)\n";

    std::cout << "\n--- reverse_string ---\n";
    std::string reversed1 = reverse_string("hello");
    std::cout << "hello -> " << reversed1 << "  (expect olleh)\n";
    std::cout << "\"\"    -> \"" << reverse_string("")  << "\" (expect \"\")\n";
}
