#include "csv_writer.hpp"
#include <iomanip>

static const char* HEADER =
    "matrix_name,rows,nnz,density,cond_est,solver,status,"
    "time_analyze_sec,time_factorize_sec,time_solve_sec,rel_residual,"
    "memory_mb,memory_solve_mb,iterations,nnz_factors,reordering,threads\n";

CsvWriter::CsvWriter(std::ostream& out) : out_(out) {
    out_ << HEADER;
    out_.flush();
}

static void write_double(std::ostream& out, double v) {
    if (v < 0.0) out << "";
    else         out << std::scientific << std::setprecision(6) << v;
}

static void write_long(std::ostream& out, long v) {
    if (v < 0) out << "";
    else       out << v;
}

void CsvWriter::write(const CsvRow& row) {
    out_ << row.matrix_name << ","
         << row.rows        << ","
         << row.nnz         << ","
         << std::scientific << std::setprecision(3) << row.density << ",";
    write_double(out_, row.cond_est);
    out_ << "," << row.solver << "," << row.status << ",";
    write_double(out_, row.time_analyze);
    out_ << ",";
    write_double(out_, row.time_factorize);
    out_ << ",";
    write_double(out_, row.time_solve);
    out_ << ",";
    write_double(out_, row.rel_residual);
    out_ << ",";
    write_long(out_, row.memory_mb);
    out_ << ",";
    write_long(out_, row.memory_solve_mb);
    out_ << ",";
    out_ << ",";
    write_long(out_, row.nnz_factors);
    out_ << "," << row.reordering
         << "," << row.threads << "\n";
    out_.flush();
}
