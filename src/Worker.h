#pragma once

#include <memory>
#include <string>

#include "Cube.h"
#include "SchedulerConfig.h"
#include "minisat/core/Solver.h"

// One MiniSat instance. Owns its own clause database. Parallel workers will
// each hold a Worker; the sequential path uses a single one.
//
// MiniSat::Solver::activity is protected. A later subclass should expose it
// for the activity division heuristic, and Solver::interrupt() is the hook
// for safe cooperative preemption.
class Worker {
public:
    explicit Worker(int id);

    void load(const std::string& cnf_path);
    Minisat::lbool solve(const Cube& cube);
    void requestInterrupt();
    void clearInterrupt();

    int id() const { return id_; }
    Minisat::Solver& solver();

    // Returns var_Undef until a DivisionHeuristic is implemented.
    Minisat::Var pickDivisionVar(DivisionHeuristic heuristic) const;

private:
    int id_;
    std::unique_ptr<Minisat::Solver> solver_;
};
