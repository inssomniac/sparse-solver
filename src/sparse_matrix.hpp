#pragma once
#include <vector>
#include <string>

struct SparseMatrix {
    int rows = 0;
    int cols = 0;
    int nnz  = 0;

    std::vector<int>    col_ptr;
    std::vector<int>    row_idx;
    std::vector<double> values;

    std::string name;
};
