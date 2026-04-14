#include "metrics.hpp"
#include <cmath>

double rel_residual(const SparseMatrix& A,
                    const std::vector<double>& x,
                    const std::vector<double>& b) {
    std::vector<double> r(A.rows, 0.0);
    for (int j = 0; j < A.cols; ++j)
        for (int k = A.col_ptr[j]; k < A.col_ptr[j+1]; ++k)
            r[A.row_idx[k]] += A.values[k] * x[j];
    for (int i = 0; i < A.rows; ++i) r[i] -= b[i];

    double norm_r = 0.0, norm_b = 0.0;
    for (double v : r) norm_r += v * v;
    for (double v : b) norm_b += v * v;
    norm_r = std::sqrt(norm_r);
    norm_b = std::sqrt(norm_b);

    if (norm_b == 0.0) return -1.0;
    return norm_r / norm_b;
}
