#pragma once
#include <string>
#include <vector>

struct Config {
    std::vector<std::string> matrix_paths;
    std::vector<std::string> solvers;
    std::vector<int>         threads;
    std::string              output_path = "results/bench.csv";
    int                      timeout_sec = 300;
    bool                     worker_mode = false;
    std::string              worker_matrix;
    std::string              worker_solver;
    unsigned                 worker_seed = 42;
};

Config parse_args(int argc, char** argv);
