#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "metrics.hpp"
#include "matrix_io.hpp"
#include "rhs_gen.hpp"

TEST_CASE("rel_residual is zero for exact solution") {
    auto A = read_mtx("tests/data/small_unsym.mtx");
    auto [x_true, b] = generate_rhs(A, 42);
    double r = rel_residual(A, x_true, b);
    REQUIRE_THAT(r, Catch::Matchers::WithinAbs(0.0, 1e-12));
}

TEST_CASE("rel_residual is large for zero solution") {
    auto A = read_mtx("tests/data/small_unsym.mtx");
    auto [x_true, b] = generate_rhs(A, 42);
    std::vector<double> x_zero(A.cols, 0.0);
    double r = rel_residual(A, x_zero, b);
    REQUIRE(r > 0.99);
}

TEST_CASE("rel_residual formula: ||Ax-b|| / ||b||") {
    auto A = read_mtx("tests/data/small_unsym.mtx");
    auto [x_true, b] = generate_rhs(A, 42);
    std::vector<double> x_perturbed = x_true;
    x_perturbed[0] += 1e-6;
    double r = rel_residual(A, x_perturbed, b);
    REQUIRE(r > 0.0);
    REQUIRE(r < 1.0);
}

TEST_CASE("rel_residual returns sentinel -1 when b is zero") {
    // A 2x2 identity, x_true = zeros → b = zeros → division by zero path
    SparseMatrix A;
    A.rows = A.cols = 2; A.nnz = 2;
    A.col_ptr = {0, 1, 2};
    A.row_idx = {0, 1};
    A.values  = {1.0, 1.0};
    std::vector<double> x = {0.0, 0.0};
    std::vector<double> b = {0.0, 0.0};
    REQUIRE(rel_residual(A, x, b) < 0.0);
}
