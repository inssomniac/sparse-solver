#include "worker.hpp"
#include "matrix_io.hpp"
#include "rhs_gen.hpp"
#include "metrics.hpp"
#include "solver_base.hpp"
#include <memory>
#include <iostream>
#include <cstdlib>
#include <string>

#ifdef HAVE_MUMPS
#include "mumps_solver.hpp"
#endif
#ifdef HAVE_SUPERLU
#include "superlu_solver.hpp"
#endif
#ifdef HAVE_PARDISO
#include "pardiso_solver.hpp"
#endif

static std::unique_ptr<SolverBase> make_solver(const std::string& name) {
#ifdef HAVE_MUMPS
    if (name == "mumps")   return std::make_unique<MumpsSolver>();
#endif
#ifdef HAVE_SUPERLU
    if (name == "superlu") return std::make_unique<SuperluSolver>();
#endif
#ifdef HAVE_PARDISO
    if (name == "pardiso") return std::make_unique<PardisoSolver>();
#endif
    std::cerr << "Solver not compiled: " << name << "\n";
    return nullptr;
}

int run_worker(const Config& cfg) {
    int nthreads = cfg.threads.empty() ? 1 : cfg.threads[0];
    setenv("OMP_NUM_THREADS", std::to_string(nthreads).c_str(), 1);

    try {
        auto A            = read_mtx(cfg.worker_matrix);
        auto [x_true, b]  = generate_rhs(A, cfg.worker_seed);
        auto solver       = make_solver(cfg.worker_solver);
        if (!solver) return 1;

        auto result = solver->solve(A, b, x_true);

        std::cout << "status="        << result.status             << "\n";
        std::cout << "time_factorize=" << result.time_factorize_sec << "\n";
        std::cout << "time_solve="     << result.time_solve_sec     << "\n";
        std::cout << "rel_residual="   << result.rel_residual       << "\n";
        std::cout << "nnz_factors="    << result.nnz_factors        << "\n";
        std::cout.flush();
        return result.status == "OK" ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << "Worker error: " << e.what() << "\n";
        std::cout << "status=FAIL\n";
        std::cout.flush();
        return 1;
    }
}
