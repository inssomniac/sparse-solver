#include <catch2/catch_test_macros.hpp>
#include "csv_writer.hpp"
#include <sstream>
#include <string>

TEST_CASE("CSV writer outputs header on construction", "[CSV]") {
    std::ostringstream out;
    CsvWriter w(out);
    std::string s = out.str();
    REQUIRE(s.find("matrix_name") != std::string::npos);
    REQUIRE(s.find("time_factorize_sec") != std::string::npos);
    REQUIRE(s.find("time_solve_sec") != std::string::npos);
    REQUIRE(s.find("cond_est") != std::string::npos);
}

TEST_CASE("CSV writer writes OK row with values", "[CSV]") {
    std::ostringstream out;
    CsvWriter w(out);
    CsvRow row;
    row.matrix_name    = "bcsstk17";
    row.rows           = 1000;
    row.nnz            = 5000;
    row.density        = 5e-3;
    row.cond_est       = 1.2e6;
    row.solver         = "MUMPS";
    row.status         = "OK";
    row.time_factorize = 0.38;
    row.time_solve     = 0.07;
    row.rel_residual   = 2.3e-14;
    row.memory_mb      = 180;
    row.threads        = 4;
    w.write(row);
    std::string s = out.str();
    REQUIRE(s.find("bcsstk17") != std::string::npos);
    REQUIRE(s.find("MUMPS") != std::string::npos);
    REQUIRE(s.find("OK") != std::string::npos);
}

TEST_CASE("CSV writer uses empty fields for missing values", "[CSV]") {
    std::ostringstream out;
    CsvWriter w(out);
    CsvRow row;
    row.matrix_name    = "test";
    row.rows           = 10;
    row.nnz            = 20;
    row.density        = 0.2;
    row.cond_est       = -1.0;
    row.solver         = "MUMPS";
    row.status         = "FAIL";
    row.time_factorize = -1.0;
    row.time_solve     = -1.0;
    row.rel_residual   = -1.0;
    row.memory_mb      = -1;
    row.threads        = 1;
    w.write(row);
    std::string s = out.str();
    REQUIRE(s.find(",,") != std::string::npos);
}
