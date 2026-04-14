#include "cond_est.hpp"
#include <vector>
#include <cmath>
#include <algorithm>

extern "C" {
    void dgetrf_(int* m, int* n, double* a, int* lda, int* ipiv, int* info);
    double dlange_(char* norm, int* m, int* n, double* a, int* lda, double* work);
    void dgecon_(char* norm, int* n, double* a, int* lda, double* anorm,
                 double* rcond, double* work, int* iwork, int* info);
}

static std::vector<double> to_dense(const SparseMatrix& A) {
    std::vector<double> D(static_cast<size_t>(A.rows) * A.cols, 0.0);
    for (int j = 0; j < A.cols; ++j)
        for (int k = A.col_ptr[j]; k < A.col_ptr[j+1]; ++k)
            D[static_cast<size_t>(j) * A.rows + A.row_idx[k]] = A.values[k];
    return D;
}

double estimate_cond(const SparseMatrix& A) {
    if (A.rows != A.cols) return -1.0;
    int n = A.rows;

    if (n > 10000) return -1.0;

    auto D = to_dense(A);
    std::vector<int> ipiv(n);

    char norm_1 = '1';
    std::vector<double> work_norm(n);
    double anorm = dlange_(&norm_1, &n, &n, D.data(), &n, work_norm.data());

    int info = 0;
    dgetrf_(&n, &n, D.data(), &n, ipiv.data(), &info);
    if (info != 0) return -1.0;

    double rcond = 0.0;
    std::vector<double> work(4 * n);
    std::vector<int>    iwork(n);
    dgecon_(&norm_1, &n, D.data(), &n, &anorm, &rcond, work.data(), iwork.data(), &info);
    if (info != 0 || rcond == 0.0) return -1.0;

    return 1.0 / rcond;
}
