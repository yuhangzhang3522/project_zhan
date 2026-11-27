#include "csv_io.h"
#include "idw.h"
#include "raster_io.h"
#include "types.h"

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace noise_idw;

namespace {

IdwConfig parseArgs(int argc, char** argv) {
    IdwConfig cfg;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto requireValue = [&]() -> std::string {
            if (i + 1 >= argc) {
                throw std::runtime_error("参数缺少取值: " + arg);
            }
            return argv[++i];
        };

        if (arg == "--field") {
            cfg.field = requireValue();
        } else if (arg == "--cell") {
            cfg.cellSize = std::stod(requireValue());
        } else if (arg == "--buffer") {
            cfg.buffer = std::stod(requireValue());
        } else if (arg == "--neighbors") {
            cfg.neighborsList.clear();
            std::stringstream ss(requireValue());
            std::string token;
            while (std::getline(ss, token, ',')) {
                if (!token.empty()) cfg.neighborsList.push_back(std::stoi(token));
            }
            if (!cfg.neighborsList.empty()) cfg.neighbors = cfg.neighborsList.front();
        } else if (arg == "--radius") {
            cfg.searchRadius = std::stod(requireValue());
        } else if (arg == "--powers") {
            cfg.powers.clear();
            std::stringstream ss(requireValue());
            std::string token;
            while (std::getline(ss, token, ',')) {
                if (!token.empty()) cfg.powers.push_back(std::stod(token));
            }
        } else if (arg == "--best-only") {
            cfg.bestOnly = true;
        } else if (arg == "-h" || arg == "--help") {
            std::cout
                << "用法: IDWInterpolation [选项]\n"
                << "  --field <name>        插值字段名（默认 leq_energy）\n"
                << "  --powers <p1,p2..>    幂指数列表（默认 1.0,1.5,2.0,2.5,3.0）\n"
                << "  --neighbors <k1,k2..> 最近邻点数列表，0 表示全部点（默认 0）\n"
                << "  --cell <meters>       栅格像元分辨率（默认 5.0）\n"
                << "  --buffer <meters>     外包框外扩距离（默认 100.0）\n"
                << "  --radius <meters>     栅格外推截断半径，0 表示不限（默认 160.0）\n"
                << "  --best-only           仅输出每时段 LOOCV RMSE 最优组合的栅格\n";
            std::exit(0);
        } else {
            throw std::runtime_error("未知参数: " + arg);
        }
    }
    return cfg;
}

std::string formatPowerTag(double p) {
    std::ostringstream oss;
    oss << p;
    std::string s = oss.str();
    std::replace(s.begin(), s.end(), '.', '_');
    return s;
}

}  // namespace

int main(int argc, char** argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    try {
        IdwConfig cfg = parseArgs(argc, argv);
        const fs::path exeHint = (argc > 0 && argv[0] != nullptr) ? fs::path(argv[0]) : fs::path{};
        const fs::path root = findProjectRoot(exeHint);
        const fs::path inputDir = resolveInputDir(root);
        const fs::path rasterDir = root / "outputs" / "rasters";
        const fs::path tableDir = root / "outputs" / "tables";
        fs::create_directories(rasterDir);
        fs::create_directories(tableDir);

        auto dataByPeriod = loadPeriodSamples(inputDir, cfg.field);

        std::cout << "项目根目录: " << root.u8string() << "\n"
                  << "输入目录:   " << inputDir.u8string() << "\n"
                  << "插值字段:   " << cfg.field
                  << " | 分辨率: " << cfg.cellSize << " m"
                  << " | 外推截断半径: " << cfg.searchRadius << " m\n";

        const fs::path metricsPath = tableDir / "idw_loocv_metrics.csv";
        const fs::path residualsPath = tableDir / "idw_loocv_residuals.csv";
        std::ofstream metOut(metricsPath);
        std::ofstream resOut(residualsPath);
        if (!metOut || !resOut) {
            throw std::runtime_error("无法写入输出精度表至: " + tableDir.u8string());
        }

        metOut << "\xEF\xBB\xBF" << "method,period,power,neighbors,n,MAE,RMSE,R2,ME,MaxErr\n"
               << std::fixed << std::setprecision(4);
        resOut << "\xEF\xBB\xBF" << "method,period,power,neighbors,point_id,observed,predicted,residual\n"
               << std::fixed << std::setprecision(3);

        const std::vector<std::string> periods = {"morning", "noon", "night"};
        for (const auto& period : periods) {
            const auto& samples = dataByPeriod[period];
            if (samples.size() < 3) {
                std::cerr << "[跳过] 时段 " << period << " 有效样本不足 3 个\n";
                continue;
            }
            std::cout << "\n[" << period << "] 样本数 n = " << samples.size() << "\n";

            double bestRmse = std::numeric_limits<double>::max();
            double bestPower = cfg.powers.front();
            int bestNeighbors = cfg.neighborsList.front();

            for (int k : cfg.neighborsList) {
                cfg.neighbors = k;
                for (double p : cfg.powers) {
                    std::vector<ResidualRecord> residuals;
                    const LoocvMetrics m = evaluateLoocv(samples, p, cfg, residuals);

                    std::cout << "  IDW p=" << std::setprecision(1) << std::fixed << p
                              << " k=" << std::setw(2) << k
                              << std::setprecision(3)
                              << "  RMSE=" << m.rmse
                              << "  MAE=" << m.mae
                              << "  R2=" << m.r2
                              << "  MaxErr=" << m.maxErr << "\n";

                    metOut << "IDW," << period << "," << p << "," << k << "," << m.count << ","
                           << m.mae << "," << m.rmse << "," << m.r2 << ","
                           << m.me << "," << m.maxErr << "\n";

                    for (const auto& rec : residuals) {
                        resOut << "IDW," << period << "," << p << "," << k << "," << rec.pointId << ","
                               << rec.observed << "," << rec.predicted << "," << rec.residual << "\n";
                    }

                    if (m.rmse < bestRmse) {
                        bestRmse = m.rmse;
                        bestPower = p;
                        bestNeighbors = k;
                    }

                    if (!cfg.bestOnly) {
                        const fs::path ascPath =
                            rasterDir / ("idw_" + period + "_p" + formatPowerTag(p) +
                                         (k > 0 ? ("_k" + std::to_string(k)) : "") + ".asc");
                        const auto [ncols, nrows] = writeAsciiGrid(ascPath, samples, p, cfg);
                        std::cout << "    -> 栅格: " << ascPath.filename().u8string()
                                  << " (" << ncols << " x " << nrows << ")\n";
                    }
                }
            }

            if (cfg.bestOnly) {
                cfg.neighbors = bestNeighbors;
                const fs::path ascPath =
                    rasterDir / ("idw_" + period + "_best_p" + formatPowerTag(bestPower) +
                                 "_k" + std::to_string(bestNeighbors) + ".asc");
                const auto [ncols, nrows] = writeAsciiGrid(ascPath, samples, bestPower, cfg);
                std::cout << "  ★ 最优组合: p=" << std::setprecision(1) << bestPower
                          << ", k=" << bestNeighbors
                          << " (RMSE=" << std::setprecision(3) << bestRmse << ")"
                          << " -> " << ascPath.filename().u8string()
                          << " (" << ncols << " x " << nrows << ")\n";
            }
        }

        std::cout << "\n精度汇总表: " << metricsPath.u8string()
                  << "\n逐点残差表: " << residualsPath.u8string()
                  << "\n全部完成。\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << "\n";
        return 1;
    }
}
