#pragma once
#include <ostream>
#include <string>

struct CsvRow {
    std::string matrix_name;
    int         rows           = 0;
    int         nnz            = 0;
    double      density        = 0.0;
    double      cond_est       = -1.0;
    std::string solver;
    std::string status;
    double      time_analyze   = -1.0;
    double      time_factorize = -1.0;
    double      time_solve     = -1.0;
    double      rel_residual   = -1.0;
    long        memory_mb       = -1;
    long        memory_solve_mb = -1;
    long        nnz_factors     = -1;
    std::string reordering;
    int         threads        = 1;
};

class CsvWriter {
public:
    explicit CsvWriter(std::ostream& out);
    void write(const CsvRow& row);
private:
    std::ostream& out_;
};
