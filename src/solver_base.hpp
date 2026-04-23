#pragma once
#include "sparse_matrix.hpp"
#include <string>
#include <vector>

struct SolveResult {
    std::string status             = "FAIL";
    double      time_analyze_sec   = -1.0;
    double      time_factorize_sec = -1.0;
    double      time_solve_sec     = -1.0;
    double      rel_residual       = -1.0;
    long        nnz_factors        = -1;
    std::string reordering;
};

class SolverBase {
public:
    virtual ~SolverBase() = default;
    virtual std::string name() const = 0;
    virtual SolveResult solve(const SparseMatrix& A,
                              const std::vector<double>& b,
                              const std::vector<double>& x_true) = 0;
};
