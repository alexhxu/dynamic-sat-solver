#pragma once

// Counters the evaluation harness will report. Fields stay zero until the
// scheduler that produces them is implemented.
struct Metrics {
    double wall_seconds = 0.0;
    double idle_seconds = 0.0;
    int cubes_generated = 0;
    int steals = 0;
    double interrupt_latency_seconds = 0.0;

    void print() const;
};
