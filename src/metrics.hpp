#pragma once
#include "sparse_matrix.hpp"
#include <vector>

double rel_residual(const SparseMatrix& A,
                    const std::vector<double>& x,
                    const std::vector<double>& b);
