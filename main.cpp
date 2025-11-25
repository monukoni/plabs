#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <random>
#include <chrono>


using namespace std;

enum class ActionType { READ, WRITE, STRING };

struct Command {
    ActionType type;
    int field_num;
    int value;
};

class DataStruct {
    vector<int> fields;
    mutable vector<shared_mutex> field_mutexes;

public:
    DataStruct(size_t m = 3) : fields(m), field_mutexes(m) {}

    int read_field(int field_num) const {
        if (field_num < 0 || field_num >= fields.size()) return 0;

        shared_lock<shared_mutex> lock(field_mutexes[field_num]);
        return fields[field_num];
    }

    void write_field(int field_num, int field_value) {
        if (field_num < 0 || field_num >= fields.size()) return;

        unique_lock<shared_mutex> lock(field_mutexes[field_num]);
        fields[field_num] = field_value;
    }

    operator string() const {
        if (fields.size() == 3) {
             scoped_lock lock(field_mutexes[0], field_mutexes[1], field_mutexes[2]);
             return to_string(fields[0]) + " " + to_string(fields[1]) + " " + to_string(fields[2]);
        }
        return "";
    }
};

template <typename F>
chrono::nanoseconds time_f(F&& f) {
    auto f_start = chrono::high_resolution_clock::now();
    f();
    auto f_end = chrono::high_resolution_clock::now();
    return f_end - f_start;
}

string format_time(const chrono::nanoseconds& time) {
    auto ms = chrono::duration_cast<chrono::milliseconds>(time).count();
    return to_string(ms) + " ms";
}

void generate_random_action_file(const string& output_file_name, const map<std::string, int>& operations, size_t action_count) {
    vector<string> keys;
    vector<int> weights;
    for (const auto& [k, v] : operations) {
        keys.push_back(k);
        weights.push_back(v);
    }

    static mt19937 gen(random_device{}());
    discrete_distribution<> dist(weights.begin(), weights.end());

    ofstream output_file(output_file_name);
    for (size_t i = 0; i < action_count; ++i) {
        string op = keys[dist(gen)];
        output_file << op;
        if (op.find("write") != string::npos) {
            output_file << " " << 1;
        }
        output_file << "\n";
    }
}

vector<Command> load_commands(const string& filename) {
    vector<Command> commands;
    ifstream file(filename);
    string line, op;
    while (file >> op) {
        Command cmd;
        if (op == "read") {
            cmd.type = ActionType::READ;
            file >> cmd.field_num;
        } else if (op == "write") {
            cmd.type = ActionType::WRITE;
            file >> cmd.field_num >> cmd.value;
        } else if (op == "string") {
            cmd.type = ActionType::STRING;
        }
        commands.push_back(cmd);
    }
    return commands;
}

void execute_commands_thread(const vector<Command>& commands, DataStruct& data) {
    for (const auto& cmd : commands) {
        switch (cmd.type) {
            case ActionType::READ:
                data.read_field(cmd.field_num);
                break;
            case ActionType::WRITE:
                data.write_field(cmd.field_num, cmd.value);
                break;
            case ActionType::STRING:
                string s = string(data);
                break;
        }
    }
}

void run_experiment(const vector<string>& filenames) {
    vector<vector<Command>> all_thread_commands;
    for (const auto& fname : filenames) {
        all_thread_commands.push_back(load_commands(fname));
    }

    DataStruct data(3);

    auto duration = time_f([&]() {
        vector<jthread> threads;
        for (size_t i = 0; i < filenames.size(); ++i) {
            threads.emplace_back(execute_commands_thread, ref(all_thread_commands[i]), ref(data));
        }
    });

    cout << filenames.size() << " thread(s): " << format_time(duration) << endl;
}

int main() {
    map<string, int> variant_ops = {
        {"read 0", 15}, {"write 0", 1},
        {"read 1", 50}, {"write 1", 3},
        {"read 2", 5},  {"write 2", 1},
        {"string", 25}
    };

    map<string, int> equal_ops = {
        {"read 0", 14}, {"write 0", 14},
        {"read 1", 14}, {"write 1", 14},
        {"read 2", 14}, {"write 2", 14},
        {"string", 16}
    };

    map<string, int> worst_ops = {
        {"read 0", 1},  {"write 0", 30},
        {"read 1", 1},  {"write 1", 30},
        {"read 2", 1},  {"write 2", 30},
        {"string", 7}
    };

    size_t N = 100000;

    cout << "Generating files for Variant 4..." << endl;
    for(int i=1; i<=3; ++i) {
        generate_random_action_file("var_" + to_string(i) + ".txt", variant_ops, N);
        generate_random_action_file("eq_" + to_string(i) + ".txt", equal_ops, N);
        generate_random_action_file("bad_" + to_string(i) + ".txt", worst_ops, N);
    }

    cout << "\n--- Variant #4 (m=3) ---" << endl;
    run_experiment({"var_1.txt"});
    run_experiment({"var_1.txt", "var_2.txt"});
    run_experiment({"var_1.txt", "var_2.txt", "var_3.txt"});

    cout << "\n--- Equal Probabilities ---" << endl;
    run_experiment({"eq_1.txt"});
    run_experiment({"eq_1.txt", "eq_2.txt"});
    run_experiment({"eq_1.txt", "eq_2.txt", "eq_3.txt"});

    cout << "\n--- Write Heavy (Worst Case) ---" << endl;
    run_experiment({"bad_1.txt"});
    run_experiment({"bad_1.txt", "bad_2.txt"});
    run_experiment({"bad_1.txt", "bad_2.txt", "bad_3.txt"});

    return 0;
}