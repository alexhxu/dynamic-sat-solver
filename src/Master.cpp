#include "Master.h"

#include <chrono>
#include <stdexcept>

#include "Cube.h"
#include "Worker.h"

Master::Master(SchedulerConfig config) : config_(config) {}

Minisat::lbool Master::solve(const std::string& cnf_path, Metrics& metrics) {
    if (config_.mode != Mode::Sequential || config_.workers != 1 || config_.share_clauses) {
        throw std::runtime_error(
            "only sequential mode with one worker is implemented; "
            "static partitioning, dynamic division, and clause sharing are not");
    }

    Worker worker(0);
    worker.load(cnf_path);

    const auto start = std::chrono::steady_clock::now();
    const Minisat::lbool result = worker.solve(Cube{});
    const auto stop = std::chrono::steady_clock::now();

    metrics.wall_seconds = std::chrono::duration<double>(stop - start).count();
    metrics.cubes_generated = 1;
    return result;
}
