#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <fstream>
#include "matrix_io.hpp"

TEST_CASE("read small unsymmetric MTX", "[MTX]") {
    auto A = read_mtx("tests/data/small_unsym.mtx");
    REQUIRE(A.rows == 3);
    REQUIRE(A.cols == 3);
    REQUIRE(A.nnz == 5);
}

TEST_CASE("MTX CSC column pointers correct", "[MTX]") {
    auto A = read_mtx("tests/data/small_unsym.mtx");
    REQUIRE(A.col_ptr.size() == 4);
    REQUIRE(A.col_ptr[0] == 0);
    REQUIRE(A.col_ptr[3] == 5);
}

TEST_CASE("MTX CSC values correct", "[MTX]") {
    auto A = read_mtx("tests/data/small_unsym.mtx");
    REQUIRE(A.row_idx[A.col_ptr[0]] == 0);
    REQUIRE_THAT(A.values[A.col_ptr[0]], Catch::Matchers::WithinRel(4.0, 1e-12));
}

TEST_CASE("symmetric MTX expanded to full", "[MTX]") {
    std::ofstream f("tests/data/sym_test.mtx");
    f << "%%MatrixMarket matrix coordinate real symmetric\n";
    f << "2 2 3\n1 1 2.0\n2 1 1.0\n2 2 3.0\n";
    f.close();
    auto A = read_mtx("tests/data/sym_test.mtx");
    REQUIRE(A.nnz == 4);
}

TEST_CASE("symmetric diagonal-only MTX not doubled", "[MTX]") {
    // A purely diagonal symmetric matrix: off-diagonal expansion must not fire
    std::ofstream f("tests/data/sym_diag.mtx");
    f << "%%MatrixMarket matrix coordinate real symmetric\n";
    f << "3 3 3\n1 1 1.0\n2 2 2.0\n3 3 3.0\n";
    f.close();
    auto A = read_mtx("tests/data/sym_diag.mtx");
    REQUIRE(A.nnz == 3);
}

TEST_CASE("read_mtx throws on malformed size line", "[MTX]") {
    std::ofstream f("tests/data/bad_size.mtx");
    f << "%%MatrixMarket matrix coordinate real general\n";
    f << "% comment\n";
    f << "not_a_number\n";
    f.close();
    REQUIRE_THROWS_AS(read_mtx("tests/data/bad_size.mtx"), std::runtime_error);
}

TEST_CASE("read_mtx throws when file ends after header", "[MTX]") {
    std::ofstream f("tests/data/header_only.mtx");
    f << "%%MatrixMarket matrix coordinate real general\n";
    f.close();
    REQUIRE_THROWS_AS(read_mtx("tests/data/header_only.mtx"), std::runtime_error);
}
