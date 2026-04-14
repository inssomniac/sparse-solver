#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <fstream>
#include <cmath>
#include "cond_est.hpp"
#include "matrix_io.hpp"

TEST_CASE("cond_est for identity-like matrix is near 1", "[cond]") {
    std::ofstream f("tests/data/identity3.mtx");
    f << "%%MatrixMarket matrix coordinate real general\n3 3 3\n";
    f << "1 1 1.0\n2 2 1.0\n3 3 1.0\n";
    f.close();
    auto A = read_mtx("tests/data/identity3.mtx");
    double kappa = estimate_cond(A);
    REQUIRE_THAT(kappa, Catch::Matchers::WithinRel(1.0, 0.1));
}

TEST_CASE("cond_est for ill-conditioned matrix is large", "[cond]") {
    std::ofstream f("tests/data/illcond.mtx");
    f << "%%MatrixMarket matrix coordinate real general\n2 2 2\n";
    f << "1 1 1.0\n2 2 1e-8\n";
    f.close();
    auto A = read_mtx("tests/data/illcond.mtx");
    double kappa = estimate_cond(A);
    REQUIRE(kappa > 1e6);
}

TEST_CASE("cond_est returns positive finite value for small_unsym", "[cond]") {
    auto A = read_mtx("tests/data/small_unsym.mtx");
    double kappa = estimate_cond(A);
    REQUIRE(kappa > 0.0);
    REQUIRE(std::isfinite(kappa));
}

TEST_CASE("cond_est returns -1 for large matrix (n>10000)", "[cond]") {
    SparseMatrix A;
    A.rows = A.cols = 10001;
    A.nnz  = 10001;
    A.col_ptr.resize(10002, 0);
    A.row_idx.resize(10001);
    A.values.resize(10001, 1.0);
    for (int i = 0; i < 10001; ++i) {
        A.col_ptr[i + 1] = i + 1;
        A.row_idx[i]     = i;
    }
    double kappa = estimate_cond(A);
    REQUIRE(kappa < 0.0);
}

TEST_CASE("cond_est returns -1 for non-square matrix", "[cond]") {
    SparseMatrix A;
    A.rows = 3; A.cols = 4; A.nnz = 3;
    A.col_ptr = {0, 1, 2, 3, 3};
    A.row_idx = {0, 1, 2};
    A.values  = {1.0, 1.0, 1.0};
    REQUIRE(estimate_cond(A) < 0.0);
}
