#include "Master.h"

#include <chrono>
#include <memory>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

#include "Cube.h"
#include "Worker.h"

namespace {

// Split the shortest cube on the next variable index until there are exactly
// `workers` cubes. Powers of two come out balanced: workers 4 is every
// combination of variables 0 and 1. A formula without enough variables to
// cut that many ways throws.
std::vector<Cube> partition(int workers, int nvars) {
    std::vector<Cube> cubes(1);
    while (static_cast<int>(cubes.size()) < workers) {
        auto shortest = cubes.begin();
        for (auto it = cubes.begin() + 1; it != cubes.end(); ++it) {
            if (it->assumptions.size() < shortest->assumptions.size()) shortest = it;
        }
        const int var = static_cast<int>(shortest->assumptions.size());
        if (var >= nvars) {
            throw std::runtime_error("not enough variables to give each worker a cube");
        }
        auto children = shortest->divide(static_cast<Minisat::Var>(var));
        *shortest = std::move(children.first);
        cubes.push_back(std::move(children.second));
    }
    return cubes;
}

}  // namespace

Master::Master(SchedulerConfig config) : config_(config) {}

Minisat::lbool Master::solve(const std::string& cnf_path, Metrics& metrics) {
    if (config_.workers < 1) {
        throw std::runtime_error("workers must be at least 1");
    }
    if (config_.share_clauses) {
        throw std::runtime_error("clause sharing is not implemented");
    }
    if (config_.mode == Mode::Dynamic) {
        throw std::runtime_error("dynamic division is not implemented");
    }
    if (config_.mode == Mode::Sequential && config_.workers != 1) {
        throw std::runtime_error("sequential mode uses one worker");
    }
    if (config_.mode == Mode::Sequential) {
        return solveSequential(cnf_path, metrics);
    }
    return solveStatic(cnf_path, metrics);
}

Minisat::lbool Master::solveSequential(const std::string& cnf_path, Metrics& metrics) {
    Worker worker(0);
    worker.load(cnf_path);

    const auto start = std::chrono::steady_clock::now();
    const Minisat::lbool result = worker.solve(Cube{});
    const auto stop = std::chrono::steady_clock::now();

    metrics.wall_seconds = std::chrono::duration<double>(stop - start).count();
    metrics.cubes_generated = 1;
    return result;
}

Minisat::lbool Master::solveStatic(const std::string& cnf_path, Metrics& metrics) {
    const int n = config_.workers;
    std::vector<std::unique_ptr<Worker>> workers;
    workers.reserve(n);
    for (int i = 0; i < n; ++i) {
        workers.push_back(std::make_unique<Worker>(i));
        workers.back()->load(cnf_path);
    }

    const std::vector<Cube> cubes = partition(n, workers.front()->solver().nVars());
    std::vector<Minisat::lbool> results(n, Minisat::l_Undef);
    std::vector<std::thread> threads;
    threads.reserve(n);

    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < n; ++i) {
        threads.emplace_back([&, i] {
            const Minisat::lbool result = workers[i]->solve(cubes[i]);
            results[i] = result;
            if (result == Minisat::l_True) {
                for (int j = 0; j < n; ++j) {
                    if (j != i) workers[j]->requestInterrupt();
                }
            }
        });
    }
    for (std::thread& thread : threads) thread.join();
    const auto stop = std::chrono::steady_clock::now();

    metrics.wall_seconds = std::chrono::duration<double>(stop - start).count();
    metrics.cubes_generated = n;

    bool sat = false;
    bool all_unsat = true;
    for (Minisat::lbool result : results) {
        if (result == Minisat::l_True) sat = true;
        if (result != Minisat::l_False) all_unsat = false;
    }
    if (sat) return Minisat::l_True;
    if (all_unsat) return Minisat::l_False;
    return Minisat::l_Undef;
}
