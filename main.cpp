#include <iostream>
#include <queue>
#include <random>
#include <coroutine>
#include <memory>

std::queue<int> numbers;

struct SuspendIfOdd {
    int value;

    bool await_ready() const {
        return (value % 2 == 0);
    }

    void await_suspend(std::coroutine_handle<>) const {
        std::cout << "  -> [AWAITER] Odd number detected (" << value << "). Suspending coroutine...\n";
    }

    void await_resume() const {}
};

struct Generator {
    struct promise_type {
        Generator get_return_object() {
            return Generator{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() { return {}; } // Зупинка на старті
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };

    std::coroutine_handle<promise_type> handle;

    Generator(std::coroutine_handle<promise_type> h) : handle(h) {}
    ~Generator() { if (handle) handle.destroy(); }

    void resume() {
        if (handle && !handle.done()) handle.resume();
    }

    bool done() const {
        return !handle || handle.done();
    }
};


Generator randomNumberGenerator() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(1, 256);

    while (true) {
        int random_number = distrib(gen);

        std::cout << "Generated: " << random_number << "\n";
        numbers.push(random_number);


        co_await SuspendIfOdd{random_number};
    }
}

void print_queue() {
    std::queue<int> temp = numbers;
    std::cout << "Current queue: [ ";
    while (!temp.empty()) {
        std::cout << temp.front() << " ";
        temp.pop();
    }
    std::cout << "]\n";
}

int main() {
    auto gen = randomNumberGenerator();

    std::cout << "--- Coroutine Demo Started ---\n";
    std::cout << "Type 'next' to generate until next odd number.\n";
    std::cout << "Type 'exit' to quit.\n\n";

    std::string command;
    while (true) {
        std::cout << "> Command: ";
        std::cin >> command;

        if (command == "next") {
            std::cout << "Resuming coroutine...\n";
            gen.resume();
            print_queue();
            std::cout << "-----------------------------\n";
        }
        else if (command == "exit") {
            break;
        }
        else {
            std::cout << "Unknown command.\n";
        }
    }

    return 0;
}