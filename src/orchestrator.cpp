#include "orchestrator.hpp"
#include "matrix_io.hpp"
#include "cond_est.hpp"
#include <cstdio>
#include <cstring>
#include <sstream>
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <filesystem>
#include <map>
#include <thread>
#include <chrono>
#include <cmath>

static std::map<std::string,std::string> parse_kv(const std::string& s) {
    std::map<std::string,std::string> m;
    std::istringstream ss(s);
    std::string line;
    while (std::getline(ss, line)) {
        auto pos = line.find('=');
        if (pos != std::string::npos)
            m[line.substr(0, pos)] = line.substr(pos+1);
    }
    return m;
}

static long parse_peak_rss_kb(const std::string& time_stderr) {
    const char* needle = "Maximum resident set size (kbytes):";
    auto pos = time_stderr.find(needle);
    if (pos == std::string::npos) return -1;
    pos += strlen(needle);
    while (pos < time_stderr.size() && (time_stderr[pos] == ' ' || time_stderr[pos] == '\t')) ++pos;
    try { return std::stol(time_stderr.substr(pos)); } catch (...) { return -1; }
}

RunResult launch_worker(const std::string& exe_path,
                        const std::string& matrix_path,
                        const std::string& solver_name,
                        int threads,
                        unsigned seed,
                        int timeout_sec) {
    RunResult result;

    int stdout_pipe[2], stderr_pipe[2];
    if (pipe(stdout_pipe) || pipe(stderr_pipe)) return result;

    pid_t pid = fork();
    if (pid < 0) { close(stdout_pipe[0]); close(stdout_pipe[1]);
                   close(stderr_pipe[0]); close(stderr_pipe[1]); return result; }

    if (pid == 0) {
        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);
        close(stdout_pipe[0]); close(stdout_pipe[1]);
        close(stderr_pipe[0]); close(stderr_pipe[1]);

        std::string t = std::to_string(threads);
        std::string sd = std::to_string(seed);
        execlp("/usr/bin/time", "/usr/bin/time", "-v",
               exe_path.c_str(),
               "--worker",
               "--matrix", matrix_path.c_str(),
               "--solver", solver_name.c_str(),
               "--threads", t.c_str(),
               "--seed", sd.c_str(),
               nullptr);
        _exit(127);
    }

    close(stdout_pipe[1]);
    close(stderr_pipe[1]);

    std::string child_stdout, child_stderr;
    auto read_fd = [](int fd, std::string& out) {
        char buf[4096];
        ssize_t n;
        while ((n = ::read(fd, buf, sizeof(buf))) > 0)
            out.append(buf, static_cast<size_t>(n));
        close(fd);
    };
    std::thread t1(read_fd, stdout_pipe[0], std::ref(child_stdout));
    std::thread t2(read_fd, stderr_pipe[0], std::ref(child_stderr));

    int status = -1;
    bool timed_out = false;

    if (timeout_sec > 0) {
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(timeout_sec);
        while (std::chrono::steady_clock::now() < deadline) {
            int r = waitpid(pid, &status, WNOHANG);
            if (r > 0) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        if (waitpid(pid, &status, WNOHANG) == 0) {
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            timed_out = true;
        }
    } else {
        waitpid(pid, &status, 0);
    }

    t1.join();
    t2.join();

    if (timed_out) {
        result.status = "TIMEOUT";
        return result;
    }

    {
        auto kv = parse_kv(child_stdout);
        if (kv.count("status"))         result.status             = kv["status"];
        if (kv.count("time_analyze"))   result.time_analyze_sec   = std::stod(kv["time_analyze"]);
        if (kv.count("time_factorize")) result.time_factorize_sec = std::stod(kv["time_factorize"]);
        if (kv.count("time_solve"))     result.time_solve_sec     = std::stod(kv["time_solve"]);
        if (kv.count("rel_residual"))   result.rel_residual       = std::stod(kv["rel_residual"]);
        if (kv.count("nnz_factors"))    result.nnz_factors        = std::stol(kv["nnz_factors"]);
        if (kv.count("reordering"))     result.reordering         = kv["reordering"];
    }

    long kb = parse_peak_rss_kb(child_stderr);
    if (kb > 0) result.memory_mb = static_cast<long>(std::round(kb / 1024.0));

    return result;
}

void run_orchestrator(const Config& cfg, CsvWriter& csv) {
    std::string exe = std::filesystem::canonical("/proc/self/exe").string();

    for (const auto& mpath : cfg.matrix_paths) {
        SparseMatrix A;
        try { A = read_mtx(mpath); }
        catch (const std::exception& e) {
            std::cerr << "Warning: skip " << mpath << ": " << e.what() << "\n";
            continue;
        }

        double kappa = estimate_cond(A);

        for (const auto& solver : cfg.solvers) {
            for (int nthreads : cfg.threads) {
                std::cerr << "Running: " << A.name << " / " << solver
                          << " / " << nthreads << " threads\n";

                auto res = launch_worker(exe, mpath, solver, nthreads,
                                         cfg.worker_seed, cfg.timeout_sec);

                CsvRow row;
                row.matrix_name    = A.name;
                row.rows           = A.rows;
                row.nnz            = A.nnz;
                row.density        = static_cast<double>(A.nnz) /
                                     (static_cast<double>(A.rows) * A.cols);
                row.cond_est       = (kappa > 0) ? kappa : -1.0;
                row.solver         = solver;
                row.status         = res.status;
                row.time_analyze   = res.time_analyze_sec;
                row.time_factorize = res.time_factorize_sec;
                row.time_solve     = res.time_solve_sec;
                row.rel_residual   = res.rel_residual;
                row.memory_mb      = res.memory_mb;
                row.nnz_factors    = res.nnz_factors;
                row.reordering     = res.reordering;
                row.threads        = nthreads;
                csv.write(row);
            }
        }
    }
}
