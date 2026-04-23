#include "superlu_solver.hpp"
#include "metrics.hpp"
#include <superlu/slu_ddefs.h>
#include <chrono>
#include <vector>
#include <cstring>

SolveResult SuperluSolver::solve(const SparseMatrix& A,
                                  const std::vector<double>& b,
                                  const std::vector<double>& x_true) {
    SolveResult result;
    if (A.rows != A.cols) { result.status = "FAIL"; return result; }
    int n = A.rows;

    std::vector<int>    colptr(A.col_ptr.begin(), A.col_ptr.end());
    std::vector<int>    rowind(A.row_idx.begin(), A.row_idx.end());
    std::vector<double> nzvals(A.values.begin(),  A.values.end());
    std::vector<double> rhs(b.begin(), b.end());

    SuperMatrix Amat, L, U, Bmat;
    dCreate_CompCol_Matrix(&Amat, n, n, A.nnz,
                           nzvals.data(), rowind.data(), colptr.data(),
                           SLU_NC, SLU_D, SLU_GE);
    dCreate_Dense_Matrix(&Bmat, n, 1, rhs.data(), n, SLU_DN, SLU_D, SLU_GE);

    superlu_options_t opts;
    set_default_options(&opts);
    opts.PrintStat = NO;

    std::vector<int> perm_c(n), perm_r(n);
    int info = 0;
    SuperLUStat_t stat;
    StatInit(&stat);

    auto t0 = std::chrono::high_resolution_clock::now();
    dgssv(&opts, &Amat, perm_c.data(), perm_r.data(),
          &L, &U, &Bmat, &stat, &info);
    auto t1 = std::chrono::high_resolution_clock::now();

    if (info == 0) {
        result.time_factorize_sec = std::chrono::duration<double>(t1 - t0).count();
        result.time_solve_sec     = 0.0;
        result.rel_residual       = rel_residual(A, rhs, b);
        result.nnz_factors        = static_cast<long>(((SCformat*)L.Store)->nnz) +
                                    static_cast<long>(((NCformat*)U.Store)->nnz);
        result.status             = "OK";
    } else {
        result.status = "FAIL";
    }

    Destroy_SuperMatrix_Store(&Amat);
    Destroy_SuperMatrix_Store(&Bmat);
    Destroy_SuperNode_Matrix(&L);
    Destroy_CompCol_Matrix(&U);
    StatFree(&stat);
    return result;
}
