#include "Metrics.h"

#include <iostream>

void Metrics::print() const {
    std::cout << "c wall_seconds " << wall_seconds << "\n"
              << "c idle_seconds " << idle_seconds << "\n"
              << "c cubes_generated " << cubes_generated << "\n"
              << "c steals " << steals << "\n"
              << "c interrupt_latency_seconds " << interrupt_latency_seconds << "\n";
}
