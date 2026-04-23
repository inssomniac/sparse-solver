#include "pardiso_solver.hpp"
#include "metrics.hpp"
#include <mkl_pardiso.h>
#include <mkl_types.h>
#include <chrono>
#include <vector>
#include <algorithm>
#include <tuple>
#include <iostream>

static void csc_to_csr_1indexed(const SparseMatrix& A,
                                  std::vector<int>& row_ptr,
                                  std::vector<int>& col_ind,
                                  std::vector<double>& vals) {
    int n = A.rows;
    row_ptr.assign(n + 1, 0);
    col_ind.resize(A.nnz);
    vals.resize(A.nnz);

    for (int r : A.row_idx) row_ptr[r + 1]++;
    for (int i = 0; i < n; ++i) row_ptr[i+1] += row_ptr[i];

    std::vector<int> pos(row_ptr.begin(), row_ptr.end());
    for (int j = 0; j < A.cols; ++j) {
        for (int k = A.col_ptr[j]; k < A.col_ptr[j+1]; ++k) {
            int r = A.row_idx[k];
            int p = pos[r]++;
            col_ind[p] = j + 1;
            vals[p]    = A.values[k];
        }
    }
    for (int i = 0; i < n; ++i) {
        int beg = row_ptr[i], end = row_ptr[i+1];
        std::vector<std::pair<int,double>> row_entries;
        for (int k = beg; k < end; ++k)
            row_entries.emplace_back(col_ind[k], vals[k]);
        std::sort(row_entries.begin(), row_entries.end());
        for (int k = beg; k < end; ++k) {
            col_ind[k] = row_entries[k-beg].first;
            vals[k]    = row_entries[k-beg].second;
        }
    }
    for (auto& v : row_ptr) v++;
}

SolveResult PardisoSolver::solve(const SparseMatrix& A,
                                  const std::vector<double>& b,
                                  const std::vector<double>& x_true) {
    SolveResult result;
    int n = A.rows;

    std::vector<int>    ia, ja;
    std::vector<double> a_csr;
    csc_to_csr_1indexed(A, ia, ja, a_csr);

    void* pt[64] = {};
    MKL_INT iparm[64] = {};
    MKL_INT maxfct = 1, mnum = 1, mtype = 11;
    MKL_INT phase, error = 0;
    MKL_INT nrhs = 1;
    MKL_INT msglvl = 0;
    MKL_INT nn = n;

    pardisoinit(pt, &mtype, iparm);
    iparm[0]  = 1;
    iparm[1]  = 2;
    iparm[9]  = 8;
    iparm[10] = 1;
    iparm[12] = 1;
    iparm[20] = 1;
    iparm[23] = 1;
    iparm[34] = 0;

    std::vector<double> x(n);

    phase = 12;
    auto t0 = std::chrono::high_resolution_clock::now();
    pardiso(pt, &maxfct, &mnum, &mtype, &phase, &nn,
            a_csr.data(), ia.data(), ja.data(),
            nullptr, &nrhs, iparm, &msglvl,
            nullptr, nullptr, &error);
    auto t1 = std::chrono::high_resolution_clock::now();

    if (error != 0) {
        phase = -1; pardiso(pt, &maxfct, &mnum, &mtype, &phase, &nn,
                             nullptr, ia.data(), ja.data(), nullptr, &nrhs,
                             iparm, &msglvl, nullptr, nullptr, &error);
        result.status = "FAIL";
        return result;
    }
    result.time_factorize_sec = std::chrono::duration<double>(t1 - t0).count();
    result.nnz_factors        = iparm[17];

    phase = 33;
    std::vector<double> rhs(b.begin(), b.end());
    auto t2 = std::chrono::high_resolution_clock::now();
    pardiso(pt, &maxfct, &mnum, &mtype, &phase, &nn,
            a_csr.data(), ia.data(), ja.data(),
            nullptr, &nrhs, iparm, &msglvl,
            rhs.data(), x.data(), &error);
    auto t3 = std::chrono::high_resolution_clock::now();

    result.time_solve_sec = std::chrono::duration<double>(t3 - t2).count();
    result.rel_residual   = rel_residual(A, x, b);
    result.status         = "OK";

    phase = -1;
    MKL_INT cleanup_err = 0;
    pardiso(pt, &maxfct, &mnum, &mtype, &phase, &nn,
            nullptr, ia.data(), ja.data(), nullptr, &nrhs,
            iparm, &msglvl, nullptr, nullptr, &cleanup_err);
    if (cleanup_err != 0)
        std::cerr << "PARDISO cleanup error: " << cleanup_err << "\n";

    return result;
}
