#pragma once
#include "cli.hpp"
#include "csv_writer.hpp"
#include "sparse_matrix.hpp"

struct RunResult {
    std::string status             = "FAIL";
    double      time_factorize_sec = -1.0;
    double      time_solve_sec     = -1.0;
    double      rel_residual       = -1.0;
    long        memory_mb          = -1;
    long        nnz_factors        = -1;
};

RunResult launch_worker(const std::string& exe_path,
                        const std::string& matrix_path,
                        const std::string& solver_name,
                        int threads,
                        unsigned seed,
                        int timeout_sec);

void run_orchestrator(const Config& cfg, CsvWriter& csv);
