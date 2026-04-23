#include "mumps_solver.hpp"
#include "metrics.hpp"
#include <dmumps_c.h>
#include <mpi.h>
#include <chrono>
#include <stdexcept>
#include <vector>
#include <cstring>

SolveResult MumpsSolver::solve(const SparseMatrix& A,
                                const std::vector<double>& b,
                                const std::vector<double>& x_true) {
    SolveResult result;

    int mpi_initialized = 0;
    MPI_Initialized(&mpi_initialized);
    if (!mpi_initialized) {
        int argc = 0; char** argv = nullptr;
        MPI_Init(&argc, &argv);
    }

    DMUMPS_STRUC_C id;
    memset(&id, 0, sizeof(id));
    id.job  = -1;
    id.par  = 1;
    id.sym  = 0;
    id.comm_fortran = MPI_Comm_c2f(MPI_COMM_WORLD);
    dmumps_c(&id);
    if (id.infog[0] < 0) {
        result.status = "FAIL";
        id.job = -2; dmumps_c(&id);
        return result;
    }

    id.n   = A.rows;
    id.nnz = A.nnz;

    std::vector<int> irn(A.nnz), jcn(A.nnz);
    for (int j = 0; j < A.cols; ++j)
        for (int k = A.col_ptr[j]; k < A.col_ptr[j+1]; ++k) {
            irn[k] = A.row_idx[k] + 1;
            jcn[k] = j + 1;
        }
    id.irn = irn.data();
    id.jcn = jcn.data();
    id.a   = const_cast<double*>(A.values.data());

    std::vector<double> rhs(b.begin(), b.end());
    id.rhs  = rhs.data();
    id.nrhs = 1;
    id.lrhs = A.rows;

    id.icntl[0] = -1;
    id.icntl[1] = -1;
    id.icntl[2] = -1;
    id.icntl[3] = 0;
    id.icntl[7]  = 77;
    id.icntl[9]  = 2;
    id.icntl[12] = 1;
    id.cntl[0]   = 0.01;

    static const char* ordering_names[] = {
        "AMD", "user", "AMF", "SCOTCH", "PORD", "METIS", "QAMD", "auto"
    };

    id.job = 1;
    auto t0 = std::chrono::high_resolution_clock::now();
    dmumps_c(&id);
    auto t1 = std::chrono::high_resolution_clock::now();
    if (id.infog[0] < 0) {
        result.status = "FAIL";
        id.job = -2; dmumps_c(&id);
        return result;
    }
    result.time_analyze_sec = std::chrono::duration<double>(t1 - t0).count();
    {
        int ord = id.infog[6];
        if (ord >= 0 && ord <= 7) result.reordering = ordering_names[ord];
        else                      result.reordering = std::to_string(ord);
    }

    id.job = 2;
    auto t2 = std::chrono::high_resolution_clock::now();
    dmumps_c(&id);
    auto t3 = std::chrono::high_resolution_clock::now();
    if (id.infog[0] < 0) {
        result.status = "FAIL";
        id.job = -2; dmumps_c(&id);
        return result;
    }
    result.time_factorize_sec = std::chrono::duration<double>(t3 - t2).count();

    id.job = 3;
    auto t4 = std::chrono::high_resolution_clock::now();
    dmumps_c(&id);
    auto t5 = std::chrono::high_resolution_clock::now();
    if (id.infog[0] < 0) {
        result.status = "FAIL";
        id.job = -2; dmumps_c(&id);
        return result;
    }
    result.time_solve_sec =
        std::chrono::duration<double>(t5 - t4).count();

    result.rel_residual = rel_residual(A, rhs, b);
    result.nnz_factors  = id.infog[19];
    result.status = "OK";

    id.job = -2;
    dmumps_c(&id);
    return result;
}
