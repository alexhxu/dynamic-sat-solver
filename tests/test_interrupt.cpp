#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

#include "Cube.h"
#include "Worker.h"

namespace {

int failures = 0;

void check(bool ok, const std::string& what) {
    std::cout << (ok ? "PASS " : "FAIL ") << what << "\n";
    if (!ok) ++failures;
}

// Pigeonhole: `pigeons` pigeons into `pigeons - 1` holes. Unsatisfiable, and
// exponentially hard for resolution-based solvers, so it is still running
// when the test interrupts it.
std::string writePigeonhole(int pigeons) {
    const int holes = pigeons - 1;
    auto var = [holes](int p, int h) { return p * holes + h + 1; };
    const std::string path = "test_interrupt_php.cnf";
    std::ofstream out(path);
    const int clauses = pigeons + holes * pigeons * (pigeons - 1) / 2;
    out << "p cnf " << pigeons * holes << " " << clauses << "\n";
    for (int p = 0; p < pigeons; ++p) {
        for (int h = 0; h < holes; ++h) out << var(p, h) << " ";
        out << "0\n";
    }
    for (int h = 0; h < holes; ++h) {
        for (int p = 0; p < pigeons; ++p) {
            for (int q = p + 1; q < pigeons; ++q) {
                out << -var(p, h) << " " << -var(q, h) << " 0\n";
            }
        }
    }
    return path;
}

}  // namespace

int main() {
    const std::string cnf = writePigeonhole(12);
    Worker worker(0);
    worker.load(cnf);

    const Cube cube = Cube{}.divide(0).first;
    Minisat::lbool result = Minisat::l_True;
    std::chrono::steady_clock::time_point returned_at;

    std::thread solver_thread([&] {
        result = worker.solve(cube);
        returned_at = std::chrono::steady_clock::now();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    const auto interrupted_at = std::chrono::steady_clock::now();
    worker.requestInterrupt();
    solver_thread.join();

    const double latency_ms =
        std::chrono::duration<double, std::milli>(returned_at - interrupted_at).count();

    check(result == Minisat::l_Undef, "interrupted solve returns l_Undef");
    check(latency_ms >= 0.0, "solve was still running when interrupted");
    check(latency_ms < 1000.0, "solver stopped within 1s (latency " +
                                    std::to_string(latency_ms) + " ms)");
    check(worker.currentCube().assumptions == cube.assumptions,
          "current cube is preserved after interrupt");

    // A pending interrupt must survive until clearInterrupt(), so a
    // cancellation sent before solve() starts is not lost.
    worker.requestInterrupt();
    check(worker.solve(cube) == Minisat::l_Undef, "pending interrupt stops the next solve");

    worker.clearInterrupt();
    worker.solver().setConfBudget(100);
    check(worker.solve(cube) == Minisat::l_Undef,
          "solver is reusable after clearInterrupt (stops on conflict budget)");

    std::remove(cnf.c_str());
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
