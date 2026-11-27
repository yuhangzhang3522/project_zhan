#include "csv_io.h"
#include "kriging.h"
#include "raster_io.h"
#include "types.h"
#include "variogram.h"

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace noise_kriging;

namespace {

KrigingConfig parseArgs(int argc, char** argv) {
    KrigingConfig cfg;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto requireValue = [&]() -> std::string {
            if (i + 1 >= argc) throw std::runtime_error("参数缺少取值: " + arg);
            return argv[++i];
        };
        if (arg == "--field") cfg.field = requireValue();
        else if (arg == "--cell") cfg.cellSize = std::stod(requireValue());
        else if (arg == "--buffer") cfg.buffer = std::stod(requireValue());
        else if (arg == "--radius") cfg.maxExtrapolation = std::stod(requireValue());
        else if (arg == "--lags") cfg.numLags = std::stoi(requireValue());
        else if (arg == "--max-lag") cfg.maxLagDist = std::stod(requireValue());
        else if (arg == "-h" || arg == "--help") {
            std::cout
                << "用法: KrigingInterpolation [选项]\n"
                << "  --field <name>    插值字段（默认 leq_energy）\n"
                << "  --cell <m>        栅格分辨率（默认 5.0）\n"
                << "  --buffer <m>      外包框外扩距离（默认 100.0）\n"
                << "  --radius <m>      外推截断半径，超出置 NODATA（默认 160.0）\n"
                << "  --lags <n>        实验半变异函数滞后分组数（默认 10）\n"
                << "  --max-lag <m>     最大滞后距离（默认 650.0）\n";
            std::exit(0);
        } else {
            throw std::runtime_error("未知参数: " + arg);
        }
    }
    return cfg;
}

}  // namespace

int main(int argc, char** argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    try {
        const KrigingConfig cfg = parseArgs(argc, argv);
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
                  << " | 外推截断半径: " << cfg.maxExtrapolation << " m\n";

        std::ofstream empOut(tableDir / "ok_variogram_empirical.csv");
        std::ofstream parOut(tableDir / "ok_variogram_params.csv");
        std::ofstream metOut(tableDir / "ok_loocv_metrics.csv");
        std::ofstream resOut(tableDir / "ok_loocv_residuals.csv");

        empOut << "\xEF\xBB\xBF" << "period,lag_idx,lag_center_m,mean_dist_m,pair_count,semivariance\n"
               << std::fixed << std::setprecision(4);
        parOut << "\xEF\xBB\xBF" << "period,model,nugget,partial_sill,sill,range_m,nugget_sill_ratio,wls_sse,wls_rmse\n"
               << std::fixed << std::setprecision(4);
        metOut << "\xEF\xBB\xBF" << "method,period,model,n,MAE,RMSE,R2,ME,MaxErr\n"
               << std::fixed << std::setprecision(4);
        resOut << "\xEF\xBB\xBF" << "method,period,model,point_id,observed,predicted,residual,kriging_var\n"
               << std::fixed << std::setprecision(3);

        const std::vector<std::string> periods = {"morning", "noon", "night"};
        const std::vector<VariogramModel> models = {
            VariogramModel::Spherical,
            VariogramModel::Exponential
        };

        for (const auto& period : periods) {
            const auto& samples = dataByPeriod[period];
            if (samples.size() < 4) {
                std::cerr << "[跳过] 时段 " << period << " 有效样本不足\n";
                continue;
            }
            std::cout << "\n[" << period << "] 样本数 n = " << samples.size() << "\n";

            const auto empLags = computeEmpiricalVariogram(samples, cfg.numLags, cfg.maxLagDist);
            for (const auto& lag : empLags) {
                empOut << period << "," << lag.lagIdx << "," << lag.lagCenter << ","
                       << lag.meanDist << "," << lag.pairCount << "," << lag.semivariance << "\n";
            }

            VariogramParams bestParams;
            double bestRmse = std::numeric_limits<double>::max();

            for (VariogramModel model : models) {
                const VariogramParams vp = fitVariogram(empLags, model, samples);
                const double ratio = (vp.sill > 1e-12) ? (vp.nugget / vp.sill) : 0.0;
                parOut << period << "," << modelName(model) << ","
                       << vp.nugget << "," << vp.partialSill << "," << vp.sill << ","
                       << vp.range << "," << ratio << "," << vp.wlsSse << "," << vp.wlsRmse << "\n";

                std::vector<ResidualRecord> residuals;
                const LoocvMetrics m = evaluateKrigingLoocv(samples, vp, cfg.nuggetReg, residuals);

                std::cout << "  OK [" << std::setw(11) << modelName(model) << "]"
                          << std::setprecision(2) << std::fixed
                          << " c0=" << vp.nugget << " c1=" << vp.partialSill << " a=" << vp.range << "m"
                          << std::setprecision(3)
                          << " | RMSE=" << m.rmse << " MAE=" << m.mae
                          << " R2=" << m.r2 << " MaxErr=" << m.maxErr << "\n";

                metOut << "OK," << period << "," << modelName(model) << "," << m.count << ","
                       << m.mae << "," << m.rmse << "," << m.r2 << ","
                       << m.me << "," << m.maxErr << "\n";

                for (const auto& rec : residuals) {
                    resOut << "OK," << period << "," << modelName(model) << "," << rec.pointId << ","
                           << rec.observed << "," << rec.predicted << ","
                           << rec.residual << "," << rec.variance << "\n";
                }

                if (m.rmse < bestRmse) {
                    bestRmse = m.rmse;
                    bestParams = vp;
                }
            }

            // 导出该时段最优克里金估值与估计方差栅格
            OrdinaryKrigingSolver solver(samples, bestParams, cfg.nuggetReg);
            const fs::path valAsc = rasterDir / ("ok_" + period + ".asc");
            const fs::path varAsc = rasterDir / ("ok_var_" + period + ".asc");
            const auto [ncols, nrows] = writeKrigingRasters(valAsc, varAsc, samples, solver, cfg);
            std::cout << "  ★ 最优模型: " << modelName(bestParams.model)
                      << " (RMSE=" << std::setprecision(3) << bestRmse << ")"
                      << " -> " << valAsc.filename().u8string()
                      << " & " << varAsc.filename().u8string()
                      << " (" << ncols << " x " << nrows << ")\n";
        }

        std::cout << "\n普通克里金全部输出完成。\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << "\n";
        return 1;
    }
}
