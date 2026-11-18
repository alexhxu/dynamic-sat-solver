#pragma once

#include <string>

#include "Metrics.h"
#include "SchedulerConfig.h"
#include "minisat/core/SolverTypes.h"

// Owns worker lifetime and, later, the cube queue and victim selection.
// Sequential mode solves the full formula on one worker. Static partitioning
// and dynamic division throw until they are implemented.
class Master {
public:
    explicit Master(SchedulerConfig config);

    Minisat::lbool solve(const std::string& cnf_path, Metrics& metrics);

private:
    SchedulerConfig config_;
};
