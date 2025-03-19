//
// Created by Raunaq Pahwa on 2/19/25.
//

#include "Scheduler.h"
#include "iostream"

Scheduler::Scheduler(): num_processes_(0),
                        process_queue_(std::make_unique<ProcessMap>()),
                        run_queue_(std::make_unique<RuntimeSet>()),
                        tick_interval_(1),
                        cleanup_interval_(500),
                        ticker_running_(true),
                        cleanup_running_(true),
                        runtime_locations_(std::make_unique<RuntimeLocationMap>()),
                        blocked_processes_(std::make_unique<BlockedSet>()),
                        io_pool_(std::make_unique<ThreadPool>(4)),
                        ticker_(&Scheduler::Tick, this),
                        cleanup_(&Scheduler::Cleanup, this),
                        stopped_processes_(std::make_unique<std::deque<std::unique_ptr<Process>>>()),
                        process_weights_(0) {}

auto Scheduler::Schedule(std::unique_ptr<Process>&& process) -> bool {
    process->state = ProcessState::RUNNABLE;
    process->vruntime = std::chrono::milliseconds(0);
    num_processes_++;
    process_weights_ += kWeights.at(process->priority);
    auto time_slice = std::chrono::round<std::chrono::milliseconds>(
                                        (process->priority / static_cast<double>(process_weights_))
                                            * sched_latency_);
    process->time_slice = std::max(kMinTimeSlice, time_slice);
    std::unique_lock<std::mutex> lock(mutex_);
    if (not process_queue_->empty()) {
        process->vruntime = run_queue_->begin()->first;
    }
    auto [runtime_location, is_scheduled] = run_queue_->emplace(process->vruntime,
                                                                                    process->process_id);
    runtime_locations_->emplace(process->process_id, runtime_location);
    process_queue_->emplace(process->process_id, std::move(process));
    return is_scheduled;
}

auto Scheduler::BlockProcess(pid_t pid, std::chrono::milliseconds ms) -> bool {
    if (blocked_processes_->contains(pid)) {
        std::cerr << "Process " << pid << " is already blocked" << std::endl;
        return false;
    }
    std::unique_lock<std::mutex> lock(mutex_);
    const auto& current_process = process_queue_->at(pid);
    current_process->state = ProcessState::BLOCKED;
    const auto& runtime_location = runtime_locations_->at(pid);
    run_queue_->erase(runtime_location);
    runtime_locations_->erase(pid);
    io_pool_->Enqueue([this](pid_t pid, std::chrono::milliseconds ms) -> bool {
        std::this_thread::sleep_for(ms);
        std::cout << "i/o completed for process " << pid << std::endl;
        this->UnblockProcess(pid);
        return true;
    }, pid, ms);
    return true;
}

auto Scheduler::UnblockProcess(pid_t pid) -> bool {
    if (not blocked_processes_->contains(pid)) {
        std::cerr << "Process " << pid << " is already unblocked" << std::endl;
        return false;
    }
    auto& current_process = process_queue_->at(pid);
    Schedule(std::move(current_process));
    return true;
}

auto Scheduler::Preempt(pid_t pid, std::chrono::milliseconds runtime) -> bool {
    const auto& runtime_location = runtime_locations_->at(pid);
    run_queue_->erase(runtime_location);
    runtime_locations_->erase(pid);
    const auto& current_process = process_queue_->at(pid);
    current_process->state = ProcessState::RUNNABLE;
    current_process->vruntime += std::chrono::duration_cast<std::chrono::milliseconds>(
                    static_cast<double>(kWeights.at(20)) / kWeights.at(current_process->priority) * runtime);
    auto [new_runtime_location, is_preempted] = run_queue_->emplace(current_process->vruntime,
                                                                        current_process->process_id);
    if (is_preempted) {
        runtime_locations_->emplace(pid, new_runtime_location);
        return true;
    }
    return false;
}

auto Scheduler::IncreasePriority(pid_t pid, nice_t new_nice) -> bool {
    const auto& current_process = process_queue_->at(pid);
    auto new_priority = new_nice + 20;
    std::unique_lock<std::mutex> lock(mutex_);
    process_weights_ -= kWeights.at(current_process->priority);
    process_weights_ += kWeights.at(new_priority);
    std::cout << "Increasing priority of " << pid << " to " << new_priority << std::endl;
    current_process->nice = new_nice;
    current_process->priority = new_priority;
    return true;
}

auto Scheduler::StopProcess(pid_t pid) -> bool {
    std::unique_lock<std::mutex> lock(mutex_);
    const auto& runtime_location = runtime_locations_->at(pid);
    run_queue_->erase(runtime_location);
    runtime_locations_->erase(pid);
    auto& current_process = process_queue_->at(pid);
    current_process->state = ProcessState::STOPPED;
    stopped_processes_->push_back(std::move(current_process));
    process_queue_->erase(pid);
    return true;
}

auto Scheduler::Cleanup() -> void {
    while (cleanup_running_) {
        std::unique_lock<std::mutex> lock(mutex_);
        auto num_stopped_processes = std::min(static_cast<size_t>(10), stopped_processes_->size());
        lock.unlock();
        while (num_stopped_processes > 1) {
            stopped_processes_->pop_front();
            num_stopped_processes--;
        }
        std::this_thread::sleep_for(cleanup_interval_);
    }
}

auto Scheduler::GetProcesses() -> std::vector<ProcessInfo> {
    std::unique_lock<std::mutex> lock(mutex_);
    std::vector<ProcessInfo> processes;
    for (const auto& [_, process]: *process_queue_) {
        processes.emplace_back(process->process_id, process->state);
    }
    return processes;
}

auto Scheduler::Tick() -> void {
    while (ticker_running_) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (not run_queue_->empty()) {
            auto current_pid = run_queue_->begin()->second;
            const auto& current_process = process_queue_->at(current_pid);
            if (current_process->state == ProcessState::RUNNABLE) {
                current_process->last_executed_time = std::chrono::high_resolution_clock::now();
                current_process->state = ProcessState::RUNNING;
                std::cout << "Running process with pid: " << current_pid << std::endl;
            } else {
                auto runtime = std::chrono::duration_cast<std::chrono::milliseconds>(
                                                    (std::chrono::high_resolution_clock::now()
                                                        - current_process->last_executed_time));
                if (runtime >= current_process->time_slice) {
                    Scheduler::Preempt(current_pid, runtime);
                }
            }
        }
        lock.unlock();
        std::this_thread::sleep_for(tick_interval_);
    }
}

Scheduler::~Scheduler() {
    std::unique_lock<std::mutex> lock(mutex_);
    ticker_running_ = false;
    lock.unlock();
    if (ticker_.joinable()) {
        ticker_.join();
    }
    for (const auto& [_, process]: *process_queue_) {
        StopProcess(process->process_id);
    }
    std::this_thread::sleep_for(std::chrono::seconds(10));
    cleanup_running_ = false;
    if (cleanup_.joinable()) {
        cleanup_.join();
    }
}