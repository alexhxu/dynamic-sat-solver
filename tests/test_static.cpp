#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include "Master.h"
#include "Metrics.h"
#include "SchedulerConfig.h"

namespace {

int failures = 0;

void check(bool ok, const std::string& what) {
    std::cout << (ok ? "PASS " : "FAIL ") << what << "\n";
    if (!ok) ++failures;
}

Minisat::lbool solve(Mode mode, int workers, const std::string& cnf, Metrics* metrics = nullptr) {
    SchedulerConfig config;
    config.mode = mode;
    config.workers = workers;
    Metrics local;
    Metrics& out = metrics == nullptr ? local : *metrics;
    return Master(config).solve(cnf, out);
}

// Every pigeonhole clause is extended with variable 1. Assuming variable 1
// satisfies the formula immediately. Assuming its negation leaves pigeonhole,
// which stays running unless the satisfiable worker interrupts it.
std::string writeSatWithHardBranch(int pigeons) {
    const int holes = pigeons - 1;
    auto php_var = [holes](int p, int h) { return 1 + p * holes + h + 1; };
    const int php_vars = pigeons * holes;
    const int clauses = pigeons + holes * pigeons * (pigeons - 1) / 2;
    const std::string path = "test_static_sat_php.cnf";
    std::ofstream out(path);
    out << "p cnf " << 1 + php_vars << " " << clauses << "\n";
    for (int p = 0; p < pigeons; ++p) {
        out << "1 ";
        for (int h = 0; h < holes; ++h) out << php_var(p, h) << " ";
        out << "0\n";
    }
    for (int h = 0; h < holes; ++h) {
        for (int p = 0; p < pigeons; ++p) {
            for (int q = p + 1; q < pigeons; ++q) {
                out << "1 " << -php_var(p, h) << " " << -php_var(q, h) << " 0\n";
            }
        }
    }
    return path;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: test_static <smoke-sat.cnf> <smoke-unsat.cnf>\n";
        return EXIT_FAILURE;
    }
    const std::string sat_cnf = argv[1];
    const std::string unsat_cnf = argv[2];

    for (const char* name : {"smoke-sat", "smoke-unsat"}) {
        const std::string& cnf = name[6] == 's' ? sat_cnf : unsat_cnf;
        Minisat::lbool sequential;
        try {
            sequential = solve(Mode::Sequential, 1, cnf);
        } catch (const std::exception& ex) {
            check(false, std::string(name) + " sequential threw: " + ex.what());
            continue;
        }
        check(sequential == Minisat::l_True || sequential == Minisat::l_False,
              std::string(name) + " sequential decides SAT or UNSAT");

        for (int workers : {2, 4}) {
            Metrics metrics;
            Minisat::lbool parallel = Minisat::l_Undef;
            try {
                parallel = solve(Mode::Static, workers, cnf, &metrics);
            } catch (const std::exception& ex) {
                check(false, std::string(name) + " static/" + std::to_string(workers) +
                                 " threw: " + ex.what());
                continue;
            }
            check(parallel == sequential,
                  std::string(name) + " static/" + std::to_string(workers) +
                      " matches sequential");
            check(metrics.cubes_generated == workers,
                  std::string(name) + " static/" + std::to_string(workers) +
                      " generates one cube per worker");
        }
    }

    {
        const std::string one_var = "test_static_one_var.cnf";
        std::ofstream out(one_var);
        out << "p cnf 1 2\n1 0\n-1 0\n";
        out.close();
        bool threw = false;
        try {
            solve(Mode::Static, 4, one_var);
        } catch (const std::runtime_error&) {
            threw = true;
        }
        check(threw, "four workers on a one-variable formula is rejected");
        std::remove(one_var.c_str());
    }

    {
        const std::string cnf = writeSatWithHardBranch(10);
        Metrics metrics;
        const Minisat::lbool result = solve(Mode::Static, 2, cnf, &metrics);
        check(result == Minisat::l_True, "SAT cube cancels the hard sibling branch");
        check(metrics.wall_seconds < 2.0,
              "cancellation returns before the hard branch finishes (wall " +
                  std::to_string(metrics.wall_seconds) + " s)");
        std::remove(cnf.c_str());
    }

    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
