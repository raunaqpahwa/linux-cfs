#include <iostream>
#include "Scheduler.h"
#include "util.h"

int main() {
    std::unique_ptr<Scheduler> scheduler = std::make_unique<Scheduler>();
    std::string input;
    do {
        std::cout << "Enter your choice" << std::endl;
        std::cout << "sched_p <pid> (Schedule Process)" << std::endl;
        std::cout << "incr_p <pid> <nice_value> (Increase Priority)" << std::endl;
        std::cout << "curr_p (Currently Running Processes)" << std::endl;
        std::cout << "stop_p <pid> (Stop Process)" << std::endl;
        std::getline(std::cin, input);
        std::vector<std::string> tokens = SplitString(input, ' ');
        if (tokens.empty()) {
            continue;
        }
        std::string command = tokens.front();
        if (command == "sched_p") {
            if (tokens.size() < 2) {
                std::cerr << "Missing argument: <pid>" << std::endl;
                continue;
            }
            std::string s_pid = tokens.at(1);
            pid_t pid = std::stoi(s_pid);
            auto new_process = std::make_unique<Process>(pid);
            auto is_scheduled = scheduler->Schedule(std::move(new_process));
            if (is_scheduled) {
                std::cout << "Scheduled process with pid " << pid << std::endl;
            } else {
                std::cout << "Could not schedule process with pid " << pid << std::endl;
            }
        } else if (command == "curr_p") {
            std::vector<ProcessInfo> process_info = scheduler->GetProcesses();
            std::cout << "pid\tstate"<< std::endl;
            for (const auto& [pid, state]: process_info) {
                std::cout << pid << "\t" << state << std::endl;
            }
        } else if (command == "stop_p") {
            if (tokens.size() < 2) {
                std::cerr << "Missing argument: <pid>" << std::endl;
                continue;
            }
            std::string s_pid = tokens.at(1);
            pid_t pid = std::stoi(s_pid);
            auto has_stopped = scheduler->StopProcess(pid);
            if (has_stopped) {
                std::cout << "Stopped process with pid " << pid << std::endl;
            } else {
                std::cout << "Could not stop process with pid " << pid << std::endl;
            }
        } else if (command == "incr_p") {
            if (tokens.size() < 3) {
                std::cerr << "Missing argument(s): <pid> <nice_value>" << std::endl;
                continue;
            }
            std::string s_pid = tokens.at(1);
            std::string s_nice = tokens.at(2);
            pid_t pid = std::stoi(s_pid);
            auto nice = static_cast<nice_t>(std::stoi(s_nice));
            if (nice < -20 or nice > 19) {
                std::cerr << "Invalid nice value: should be -20 (highest priority) to 19 (lowest priority)";
                continue;
            }
            scheduler->IncreasePriority(pid, nice);
        } else if (command == "block_p") {
            if (tokens.size() < 2) {
                std::cerr << "Missing argument: <pid>" << std::endl;
                continue;
            }
        } else if (command == "unblock_p") {

        }
    } while (input != "q");
    return 0;
}