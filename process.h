//
// Created by Raunaq Pahwa on 2/19/25.
//
#include "chrono"

#ifndef SCHEDULER_PROCESS_H
#define SCHEDULER_PROCESS_H

enum ProcessState {
    INIT, RUNNING, RUNNABLE, BLOCKED, STOPPED, ZOMBIE
};

using nice_t = int8_t;
using priority_t = uint8_t;

struct Process {
    pid_t process_id;
    std::chrono::milliseconds vruntime;
    std::chrono::milliseconds time_slice;
    std::chrono::high_resolution_clock::time_point last_executed_time;
    ProcessState state;
    nice_t nice;
    priority_t priority;

    explicit Process(pid_t process_id): process_id(process_id),
                                      vruntime(0),
                                      state(INIT),
                                      nice(0),
                                      priority(nice+20),
                                      time_slice(0) {}
};

#endif //SCHEDULER_PROCESS_H
