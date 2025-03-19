#ifndef SCHEDULER_SCHEDULER_H
#define SCHEDULER_SCHEDULER_H

#include "chrono"
#include "map"
#include "set"
#include "unordered_set"
#include "thread"
#include "process.h"
#include "thread_pool.h"

using RuntimeSet = std::set<std::pair<std::chrono::milliseconds, pid_t>>;
using BlockedSet = std::unordered_set<pid_t>;
using ProcessMap = std::unordered_map<pid_t, std::unique_ptr<Process>>;
using RuntimeLocationMap = std::unordered_map<pid_t, RuntimeSet::const_iterator>;
using ProcessInfo = std::pair<pid_t, ProcessState>;

class Scheduler {
public:
    explicit Scheduler();

    auto Schedule(std::unique_ptr<Process>&& process) -> bool;

    auto IncreasePriority(pid_t pid, nice_t new_nice) -> bool;

    auto StopProcess(pid_t pid) -> bool;

    auto GetProcesses() -> std::vector<ProcessInfo>;

    auto BlockProcess(pid_t pid, std::chrono::milliseconds ms) -> bool;

    auto UnblockProcess(pid_t pid) -> bool;

    ~Scheduler();
private:
    std::unique_ptr<RuntimeSet> run_queue_;
    std::unique_ptr<ProcessMap> process_queue_;
    std::unique_ptr<RuntimeLocationMap> runtime_locations_;
    std::unique_ptr<std::deque<std::unique_ptr<Process>>> stopped_processes_;
    std::unique_ptr<BlockedSet> blocked_processes_;
    std::unique_ptr<ThreadPool> io_pool_;
    std::size_t num_processes_;
    std::chrono::milliseconds tick_interval_;
    std::chrono::milliseconds cleanup_interval_;
    uint64_t process_weights_;
    std::thread ticker_;
    std::thread cleanup_;
    std::mutex mutex_;
    bool ticker_running_;
    bool cleanup_running_;
    static constexpr std::chrono::milliseconds sched_latency_ = std::chrono::milliseconds(48);
    static constexpr std::chrono::milliseconds kMinTimeSlice = std::chrono::milliseconds(6);
    static constexpr std::array<int, 40> kWeights = {
            /* -20 */     88761,     71755,     56483,     46273,     36291,
            /* -15 */     29154,     23254,     18705,     14949,     11916,
            /* -10 */      9548,      7620,      6100,      4904,      3906,
            /*  -5 */      3121,      2501,      1991,      1586,      1277,
            /*   0 */      1024,       820,       655,       526,       423,
            /*   5 */       335,       272,       215,       172,       137,
            /*  10 */       110,        87,        70,        56,        45,
            /*  15 */        36,        29,        23,        18,        15,
    };

    auto Cleanup() -> void;

    auto Preempt(pid_t pid, std::chrono::milliseconds runtime) -> bool;

    auto Tick() -> void;
};

#endif //SCHEDULER_SCHEDULER_H