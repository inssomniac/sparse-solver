#include "rhs_gen.hpp"
#include <random>

std::pair<std::vector<double>, std::vector<double>>
generate_rhs(const SparseMatrix& A, unsigned seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> dist(-1.0, 1.0);

    std::vector<double> x_true(A.cols);
    for (auto& v : x_true) v = dist(rng);

    std::vector<double> b(A.rows, 0.0);
    for (int j = 0; j < A.cols; ++j)
        for (int k = A.col_ptr[j]; k < A.col_ptr[j+1]; ++k)
            b[A.row_idx[k]] += A.values[k] * x_true[j];

    return {x_true, b};
}
