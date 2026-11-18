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
// for the activity division heuristic.
class Worker {
public:
    explicit Worker(int id);

    void load(const std::string& cnf_path);

    // Solves the formula under the cube's assumptions. Returns l_True, l_False,
    // or l_Undef if requestInterrupt() stopped the search. An interrupted
    // solver is back at decision level 0, so it can be given another cube.
    //
    // solve() does not clear a pending interrupt. Call clearInterrupt() before
    // handing out new work; otherwise an interrupt sent just before the solve
    // starts (for example a SAT cancellation) would be lost.
    Minisat::lbool solve(const Cube& cube);

    // Safe to call from any thread while solve() is running.
    void requestInterrupt();
    void clearInterrupt();

    // The cube passed to the most recent solve(). After an interrupt this is
    // the unfinished subspace to resume or divide.
    const Cube& currentCube() const { return current_cube_; }

    int id() const { return id_; }
    Minisat::Solver& solver();

    // Returns var_Undef until a DivisionHeuristic is implemented.
    Minisat::Var pickDivisionVar(DivisionHeuristic heuristic) const;

private:
    int id_;
    std::unique_ptr<Minisat::Solver> solver_;
    Cube current_cube_;
};
