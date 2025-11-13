#include <algorithm>
#include <chrono>
#include <iostream>
#include <vector>
#include <execution>
#include <random>
#include <thread>
#include <map>

template <typename Callable>
auto timeit(Callable&& callable) {
    const auto begin = std::chrono::high_resolution_clock::now();
    volatile auto value = callable();
    const auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_ms = std::chrono::duration<double, std::milli>(end - begin);
    return elapsed_ms.count();
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

void Part1(const std::vector<int> &vec) {
    std::cout << "No policy: ";
    std::cout << timeit([&vec](){
        return std::count_if(vec.begin(), vec.end(),
            [](int i) { return f(i) < 7; });
    }) << " ms" << std::endl;
}

void Part2(const std::vector<int> &vec) {
    std::cout << "Sequenced policy: ";
    std::cout << timeit([&vec](){
        return std::count_if(std::execution::seq, vec.begin(), vec.end(),
            [](int i) { return f(i) < 7; });
    }) << " ms" << std::endl;

    std::cout << "Parallel policy: ";
    std::cout << timeit([&vec](){
        return std::count_if(std::execution::par, vec.begin(), vec.end(),
            [](int i) { return f(i) < 7; });
    }) << " ms" << std::endl;

    std::cout << "Unsequenced policy: ";
    std::cout << timeit([&vec](){
        return std::count_if(std::execution::unseq, vec.begin(), vec.end(),
            [](int i) { return f(i) < 7; });
    }) << " ms" << std::endl;

    std::cout << "Parallel unsequenced policy: ";
    std::cout <<timeit([&vec](){
        return std::count_if(std::execution::par_unseq, vec.begin(), vec.end(),
            [](int i) { return f(i) < 7; });
    }) << " ms" << std::endl;
}

void Part3(const std::vector<int> &vec) {
    std::cout << "Benchmarking parallel_count_if (K=1 to 32)\n";
    std::cout << std::left << std::setw(10) << "K" << std::setw(20) << "Time (ms)" << '\n';
    std::cout << std::string(30, '-') << '\n';

    std::map<int, double> results;
    double best_time_ms = std::numeric_limits<double>::max();
    int best_k = 0;

    for (int i = 1; i <= 32; i++) {
        auto duration = timeit([&vec, i]() {
             return parallel_count_if(vec.begin(), vec.end(), i,[](int x) { return f(x) < 7; });
         });

        results[i] = duration;

        std::cout << std::left << std::setw(10) << i
                  << std::setw(20) << std::format("{:.4f} ms", duration)
                  << '\n';

        if (duration < best_time_ms) {
            best_time_ms = duration;
            best_k = i;
        }
    }

    std::cout << std::string(30, '-') << '\n';
    std::cout << "Summary:\n";

    const auto hw_threads = std::thread::hardware_concurrency();
    std::cout << "Hardware (logical) threads: " << hw_threads << "\n";

    std::cout << std::format("Best performance at K = {} (achieved {:.4f} ms)\n",
                             best_k, best_time_ms);
}


int main () {
    std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<int> dist{0,1000};
    std::vector<int> vec(1'000'000);
    for (auto& i : vec) {
        i = dist(rng);
    }

    Part1(vec);
    Part2(vec);
    Part3(vec);
}
