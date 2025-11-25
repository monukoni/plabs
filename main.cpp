#include <iostream>
#include <string>
#include <syncstream>
#include <chrono>
#include <future>

using namespace std;

void work_func(const string& name, int seconds) {
    this_thread::sleep_for(chrono::seconds(seconds));
    osyncstream(cout) << name << endl;
}

void work() {
    auto start_time = chrono::high_resolution_clock::now();

    auto f_A2 = std::async(std::launch::async, work_func, "A2", 7);

    work_func("A1", 7);

    f_A2.wait();

    auto f_B2 = std::async(std::launch::async, work_func, "B2", 1);
    auto f_B3 = std::async(std::launch::async, work_func, "B3", 1);

    work_func("B1", 1);

    f_B2.wait();
    f_B3.wait();

    auto f_D = std::async(std::launch::async, work_func, "D", 1);

    work_func("C", 1);

    f_D.wait();

    auto end_time = chrono::high_resolution_clock::now();
    auto duration = chrono::duration<double>(end_time - start_time).count();

    cout << duration << endl;
    cout << "Work is done!" << endl;
}

int main() {
    work();
    return 0;
}


