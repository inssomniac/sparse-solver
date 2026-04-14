#include "cli.hpp"
#include <stdexcept>
#include <filesystem>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <algorithm>

static void usage(const char* prog) {
    std::cerr
        << "Usage: " << prog << " [options]\n"
        << "  --matrices <path>       directory or .mtx file\n"
        << "  --solvers <list>        comma-separated: mumps,superlu,pardiso\n"
        << "  --threads <list>        comma-separated thread counts (default: 1)\n"
        << "  --output <path>         CSV output path (default: results/bench.csv)\n"
        << "  --timeout <sec>         per-run timeout (default: 300, 0=disabled)\n"
        << "  --seed <uint>           RNG seed for x_true (default: 42)\n"
        << "\nWorker mode (internal, do not call directly):\n"
        << "  --worker --matrix <path> --solver <name> --threads <n> --seed <n>\n";
    std::exit(1);
}

static std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> out;
    std::istringstream ss(s);
    std::string tok;
    while (std::getline(ss, tok, delim))
        if (!tok.empty()) out.push_back(tok);
    return out;
}

Config parse_args(int argc, char** argv) {
    Config cfg;
    std::string matrices_arg;

    for (int i = 1; i < argc; ++i) {
        std::string a(argv[i]);
        auto val = [&]() -> std::string {
            if (i+1 >= argc) throw std::invalid_argument("Missing value for " + a);
            return argv[++i];
        };
        if      (a == "--help")     usage(argv[0]);
        else if (a == "--matrices") matrices_arg    = val();
        else if (a == "--solvers")  cfg.solvers     = split(val(), ',');
        else if (a == "--output")   cfg.output_path = val();
        else if (a == "--timeout")  cfg.timeout_sec = std::stoi(val());
        else if (a == "--seed")     cfg.worker_seed = static_cast<unsigned>(std::stoul(val()));
        else if (a == "--threads") {
            for (auto& t : split(val(), ','))
                cfg.threads.push_back(std::stoi(t));
        }
        else if (a == "--worker")   cfg.worker_mode   = true;
        else if (a == "--matrix")   cfg.worker_matrix = val();
        else if (a == "--solver")   cfg.worker_solver = val();
        else throw std::invalid_argument("Unknown argument: " + a);
    }

    if (cfg.threads.empty()) cfg.threads = {1};

    if (!cfg.worker_mode) {
        if (matrices_arg.empty()) throw std::invalid_argument("--matrices required");
        namespace fs = std::filesystem;
        if (fs::is_directory(matrices_arg)) {
            for (auto& entry : fs::directory_iterator(matrices_arg))
                if (entry.path().extension() == ".mtx")
                    cfg.matrix_paths.push_back(entry.path().string());
            std::sort(cfg.matrix_paths.begin(), cfg.matrix_paths.end());
        } else {
            cfg.matrix_paths.push_back(matrices_arg);
        }
        if (cfg.solvers.empty())
            throw std::invalid_argument("--solvers required");
    }

    return cfg;
}
