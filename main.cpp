#include <algorithm>
#include <chrono>
#include <iostream>
#include <vector>
#include <execution>
#include <random>
#include <thread>

template <typename Callable>
void timeit(Callable&& callable) {
    const auto begin = std::chrono::high_resolution_clock::now();
    volatile auto value = callable();
    const auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - begin;
    std::cout << elapsed_seconds.count() << " seconds, value = " << value << std::endl;
}

float f(int i) {
    for (int j = 0; j < 100; j++) {
        i = (17 * i + 13)% 101;
    }
    return i;
}

template<typename Iterator, typename Predicate>
int parallel_count_if(Iterator first, Iterator last, int num_threads, Predicate&& pred) {
    std::vector<int> results(num_threads);
    std::vector<std::thread> threads;
    const auto length = std::distance(first, last);
    const auto block_size = length / num_threads;
    const auto rem = length % num_threads;
    for (int i = 0; i < num_threads; i++) {
        auto block_end = std::next(first, block_size + (i < rem));
        threads.emplace_back([=, &results]() {
            results[i] = std::count_if(first, block_end, pred);
        });
        first = block_end;
    }
    for (auto& thread : threads) {thread.join();}
    return std::accumulate(results.cbegin(), results.cend(), 0);
}

int main () {
    std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<int> dist{0,1000};
    std::vector<int> vec(100'000'00);
    for (auto& i : vec) {
        i = dist(rng);
    }

    std::cout << "seq: ";
    timeit([&vec](){
        return std::count_if(std::execution::seq, vec.begin(), vec.end(),
            [](int i) { return f(i) < 7; });
    });

    std::cout << "par: ";
    timeit([&vec](){
        return std::count_if(std::execution::par, vec.begin(), vec.end(),
            [](int i) { return f(i) < 7; });
    });

    std::cout << "unseq: ";
    timeit([&vec](){
        return std::count_if(std::execution::unseq, vec.begin(), vec.end(),
            [](int i) { return f(i) < 7; });
    });

    std::cout << "par_unseq: ";
    timeit([&vec](){
        return std::count_if(std::execution::par_unseq, vec.begin(), vec.end(),
            [](int i) { return f(i) < 7; });
    });

    for (int i = 1; i <= 32; i++) {
        std::cout << "parallel_count_if K=" << i << ": ";
        timeit([&vec, i]() {
            return parallel_count_if(vec.begin(), vec.end(), i,[](int x) { return f(x) < 7; });
        });
    }
}
