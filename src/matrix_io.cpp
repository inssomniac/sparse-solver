#include "matrix_io.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <filesystem>
#include <vector>
#include <tuple>

SparseMatrix read_mtx(const std::string& path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("Cannot open: " + path);

    std::string line;
    std::getline(f, line);
    if (line.rfind("%%MatrixMarket", 0) != 0)
        throw std::runtime_error("Not a MatrixMarket file: " + path);

    bool is_symmetric = (line.find("symmetric") != std::string::npos ||
                         line.find("Symmetric") != std::string::npos);

    while (std::getline(f, line) && !line.empty() && line[0] == '%') {}

    std::istringstream ss(line);
    int M = 0, N = 0, NNZ = 0;
    ss >> M >> N >> NNZ;
    if (!ss || M <= 0 || N <= 0 || NNZ < 0)
        throw std::runtime_error("Invalid or missing size line in: " + path);

    std::vector<std::tuple<int,int,double>> entries;
    entries.reserve(is_symmetric ? NNZ * 2 : NNZ);

    for (int k = 0; k < NNZ; ++k) {
        int r, c; double v;
        f >> r >> c >> v;
        entries.emplace_back(r-1, c-1, v);
        if (is_symmetric && r != c)
            entries.emplace_back(c-1, r-1, v);
    }

    std::sort(entries.begin(), entries.end(),
        [](const auto& a, const auto& b) {
            if (std::get<1>(a) != std::get<1>(b)) return std::get<1>(a) < std::get<1>(b);
            return std::get<0>(a) < std::get<0>(b);
        });

    SparseMatrix A;
    A.rows = M;
    A.cols = N;
    A.nnz  = static_cast<int>(entries.size());
    A.col_ptr.resize(N + 1, 0);
    A.row_idx.reserve(A.nnz);
    A.values.reserve(A.nnz);

    for (auto& [r, c, v] : entries) {
        A.col_ptr[c + 1]++;
        A.row_idx.push_back(r);
        A.values.push_back(v);
    }
    for (int j = 0; j < N; ++j)
        A.col_ptr[j+1] += A.col_ptr[j];

    A.name = std::filesystem::path(path).stem().string();
    return A;
}
