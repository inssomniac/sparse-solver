#pragma once
#include "solver_base.hpp"

class MumpsSolver : public SolverBase {
public:
    std::string name() const override { return "mumps"; }
    SolveResult solve(const SparseMatrix& A,
                      const std::vector<double>& b,
                      const std::vector<double>& x_true) override;
};
