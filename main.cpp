#include <iostream>
#include <syncstream>
#include <vector>
#include <map>
#include <string>
#include <thread>
#include <barrier>
#include <atomic>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

struct InitData {
    const int nt = 4;

    const vector<pair<char, int>> totals = {
        {'a', 6}, {'b', 9}, {'c', 8}, {'d', 7},
        {'e', 5}, {'f', 7}, {'g', 5}, {'h', 4},
        {'i', 9}, {'j', 7}
    };

    const vector<string> jobs = {
        "aaaa",
        "aabb",
        "bbbb",
        "bbbd",
        "dddd",
        "ddff",
        "ffff",
        "feee",
        "eegg",
        "gggh",
        "hhhj",
        "jjjj",
        "jjii",
        "iiii",
        "iiic",
        "cccc",
        "ccc "
    };
};

void f(char x, int i) {
    osyncstream(cout) << "З набору " << x << ", виконано дію " << i << "." << endl;
}

map<char, atomic<int>> init_counters(const InitData& data) {
    map<char, atomic<int>> counters;
    for (const auto& [task_name, _] : data.totals) {
        counters[task_name] = 1;
    }
    return counters;
}

void run_simulation(const InitData& data) {
    auto next_idx = init_counters(data);

    barrier sync(data.nt);

    vector<jthread> workers;
    workers.reserve(data.nt);

    for (int tid = 0; tid < data.nt; ++tid) {
        workers.emplace_back([&, tid] {
            for (const auto& job_step : data.jobs) {
                if (tid < job_step.size() && job_step[tid] != ' ') {
                    char task_name = job_step[tid];
                    int i = next_idx[task_name].fetch_add(1);

                    f(task_name, i);
                }
                sync.arrive_and_wait();
            }
        });
    }
}


int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    InitData data;
    osyncstream(cout) << "Обчислення розпочато" << endl;
    run_simulation(data);
    osyncstream(cout) << "Обчислення завершено" << endl;
    return 0;
}