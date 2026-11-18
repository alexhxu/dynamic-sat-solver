#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

#include "Master.h"
#include "Metrics.h"
#include "SchedulerConfig.h"

namespace {

void usage(const char* argv0) {
    std::cerr
        << "usage: " << argv0
        << " [--workers N] [--mode sequential|static|dynamic]\n"
        << "          [--min-runtime SEC] [--heuristic activity|flip]\n"
        << "          [--share-clauses] <cnf>\n";
}

bool parseArgs(int argc, char** argv, SchedulerConfig& config, std::string& cnf_path) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto need = [&](const char* flag) -> std::string {
            if (i + 1 >= argc) {
                throw std::runtime_error(std::string("missing value for ") + flag);
            }
            return argv[++i];
        };

        if (arg == "--workers") {
            config.workers = std::stoi(need("--workers"));
        } else if (arg == "--mode") {
            const std::string mode = need("--mode");
            if (mode == "sequential") config.mode = Mode::Sequential;
            else if (mode == "static") config.mode = Mode::Static;
            else if (mode == "dynamic") config.mode = Mode::Dynamic;
            else throw std::runtime_error("unknown mode: " + mode);
        } else if (arg == "--min-runtime") {
            config.min_runtime_sec = std::stod(need("--min-runtime"));
        } else if (arg == "--heuristic") {
            const std::string heuristic = need("--heuristic");
            if (heuristic == "activity") config.heuristic = DivisionHeuristic::Activity;
            else if (heuristic == "flip") config.heuristic = DivisionHeuristic::FlipCount;
            else throw std::runtime_error("unknown heuristic: " + heuristic);
        } else if (arg == "--share-clauses") {
            config.share_clauses = true;
        } else if (arg == "--help" || arg == "-h") {
            return false;
        } else if (!arg.empty() && arg[0] == '-') {
            throw std::runtime_error("unknown option: " + arg);
        } else if (cnf_path.empty()) {
            cnf_path = arg;
        } else {
            throw std::runtime_error("unexpected argument: " + arg);
        }
    }
    return !cnf_path.empty();
}

}  // namespace

int main(int argc, char** argv) {
    SchedulerConfig config;
    std::string cnf_path;
    try {
        if (!parseArgs(argc, argv, config, cnf_path)) {
            usage(argv[0]);
            return EXIT_FAILURE;
        }
        Metrics metrics;
        const Minisat::lbool result = Master(config).solve(cnf_path, metrics);
        if (result == Minisat::l_True) {
            std::cout << "s SATISFIABLE\n";
            metrics.print();
            return 10;
        }
        if (result == Minisat::l_False) {
            std::cout << "s UNSATISFIABLE\n";
            metrics.print();
            return 20;
        }
        std::cout << "s UNKNOWN\n";
        metrics.print();
        return EXIT_FAILURE;
    } catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << "\n";
        usage(argv[0]);
        return EXIT_FAILURE;
    }
}
