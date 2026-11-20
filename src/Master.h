#pragma once

#include <string>

#include "Metrics.h"
#include "SchedulerConfig.h"
#include "minisat/core/SolverTypes.h"

// Owns worker lifetime and, later, the cube queue and victim selection.
// Sequential mode solves the full formula on one worker. Static mode splits
// the root cube until each worker has one subspace. Dynamic division throws
// until it is implemented.
class Master {
public:
    explicit Master(SchedulerConfig config);

    Minisat::lbool solve(const std::string& cnf_path, Metrics& metrics);

private:
    Minisat::lbool solveSequential(const std::string& cnf_path, Metrics& metrics);
    Minisat::lbool solveStatic(const std::string& cnf_path, Metrics& metrics);

    SchedulerConfig config_;
};
