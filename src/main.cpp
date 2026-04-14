#include "cli.hpp"
#include "worker.hpp"
#include "orchestrator.hpp"
#include "csv_writer.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <stdexcept>

int main(int argc, char** argv) {
    Config cfg;
    try {
        cfg = parse_args(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    if (cfg.worker_mode) {
        return run_worker(cfg);
    }

    std::filesystem::create_directories(
        std::filesystem::path(cfg.output_path).parent_path());

    std::ofstream out(cfg.output_path);
    if (!out) {
        std::cerr << "Cannot open output: " << cfg.output_path << "\n";
        return 1;
    }

    CsvWriter csv(out);
    try {
        run_orchestrator(cfg, csv);
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
