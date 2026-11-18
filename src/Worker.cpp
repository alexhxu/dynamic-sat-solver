#include "Worker.h"

#include <stdexcept>

#include <zlib.h>

#include "minisat/core/Dimacs.h"

Worker::Worker(int id) : id_(id), solver_(std::make_unique<Minisat::Solver>()) {}

void Worker::load(const std::string& cnf_path) {
    solver_ = std::make_unique<Minisat::Solver>();
    solver_->verbosity = 0;
    gzFile in = gzopen(cnf_path.c_str(), "rb");
    if (in == nullptr) {
        throw std::runtime_error("cannot open CNF: " + cnf_path);
    }
    Minisat::parse_DIMACS(in, *solver_);
    gzclose(in);
}

Minisat::lbool Worker::solve(const Cube& cube) {
    Minisat::vec<Minisat::Lit> assumps;
    for (Minisat::Lit lit : cube.assumptions) {
        assumps.push(lit);
    }
    clearInterrupt();
    return solver_->solveLimited(assumps);
}

void Worker::requestInterrupt() { solver_->interrupt(); }

void Worker::clearInterrupt() { solver_->clearInterrupt(); }

Minisat::Solver& Worker::solver() { return *solver_; }

Minisat::Var Worker::pickDivisionVar(DivisionHeuristic) const {
    return Minisat::var_Undef;
}
