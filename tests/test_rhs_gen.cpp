#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "matrix_io.hpp"
#include "rhs_gen.hpp"

TEST_CASE("x_true has correct size") {
    auto A = read_mtx("tests/data/small_unsym.mtx");
    auto [x_true, b] = generate_rhs(A, /*seed=*/42);
    REQUIRE(x_true.size() == 3);
    REQUIRE(b.size() == 3);
}

TEST_CASE("b = A * x_true exactly") {
    auto A = read_mtx("tests/data/small_unsym.mtx");
    auto [x_true, b] = generate_rhs(A, /*seed=*/42);
    std::vector<double> b_check(A.rows, 0.0);
    for (int j = 0; j < A.cols; ++j)
        for (int k = A.col_ptr[j]; k < A.col_ptr[j+1]; ++k)
            b_check[A.row_idx[k]] += A.values[k] * x_true[j];
    for (int i = 0; i < A.rows; ++i)
        REQUIRE_THAT(b[i], Catch::Matchers::WithinRel(b_check[i], 1e-12));
}

TEST_CASE("same seed produces same x_true") {
    auto A = read_mtx("tests/data/small_unsym.mtx");
    auto [x1, b1] = generate_rhs(A, 42);
    auto [x2, b2] = generate_rhs(A, 42);
    for (int i = 0; i < A.rows; ++i)
        REQUIRE(x1[i] == x2[i]);
}

TEST_CASE("different seed produces different x_true") {
    auto A = read_mtx("tests/data/small_unsym.mtx");
    auto [x1, b1] = generate_rhs(A, 42);
    auto [x2, b2] = generate_rhs(A, 99);
    bool differs = false;
    for (int i = 0; i < A.rows; ++i)
        if (x1[i] != x2[i]) { differs = true; break; }
    REQUIRE(differs);
}
