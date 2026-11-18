#pragma once

// Runtime knobs for the parallel scheduler. Only sequential mode is implemented.

enum class Mode {
    Sequential,
    Static,
    Dynamic,
};

enum class DivisionHeuristic {
    Activity,
    FlipCount,
};

struct SchedulerConfig {
    int workers = 1;
    Mode mode = Mode::Sequential;
    double min_runtime_sec = 0.0;
    DivisionHeuristic heuristic = DivisionHeuristic::Activity;
    bool share_clauses = false;
};
