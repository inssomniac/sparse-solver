#pragma once
#include "sparse_matrix.hpp"
#include <vector>
#include <utility>

std::pair<std::vector<double>, std::vector<double>>
generate_rhs(const SparseMatrix& A, unsigned seed = 42);
