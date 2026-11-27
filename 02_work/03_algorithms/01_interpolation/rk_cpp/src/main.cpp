#include "rk_solver.h"
#include "rk_types.h"

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace noise_rk;

namespace {

std::vector<std::string> splitCsv(const std::string& line) {
    std::vector<std::string> out;
    std::string cur;
    bool inQ = false;
    for (char c : line) {
        if (c == '"') inQ = !inQ;
        else if (c == ',' && !inQ) { out.push_back(cur); cur.clear(); }
        else if (c != '\r') cur += c;
    }
    out.push_back(cur);
    return out;
}

std::vector<std::map<std::string, std::string>> readCsv(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    if (!in) throw std::runtime_error("无法打开文件: " + p.u8string());
    std::string line;
    if (!std::getline(in, line)) throw std::runtime_error("空文件: " + p.u8string());
    if (line.size() >= 3 && static_cast<unsigned char>(line[0]) == 0xEF) line.erase(0, 3);
    auto hdr = splitCsv(line);
    std::vector<std::map<std::string, std::string>> rows;
    while (std::getline(in, line)) {
        if (line.empty() || line == "\r") continue;
        auto cols = splitCsv(line);
        std::map<std::string, std::string> r;
        for (size_t i = 0; i < hdr.size() && i < cols.size(); ++i) r[hdr[i]] = cols[i];
        rows.push_back(std::move(r));
    }
    return rows;
}

fs::path findRoot(const fs::path& hint) {
    std::vector<fs::path> starts;
    if (!hint.empty()) {
        std::error_code ec;
        fs::path p = fs::weakly_canonical(hint, ec);
        if (!ec) starts.push_back(fs::is_regular_file(p, ec) ? p.parent_path() : p);
    }
    starts.push_back(fs::current_path());
    for (fs::path cur : starts) {
        for (int i = 0; i < 10; ++i) {
            if (fs::exists(cur / "02_work" / "02_Input data") ||
                fs::exists(cur / "02_work" / "Input data")) return cur;
            if (!cur.has_parent_path() || cur.parent_path() == cur) break;
            cur = cur.parent_path();
        }
    }
    throw std::runtime_error("未找到项目仓库根目录。");
}

}  // namespace

int main(int argc, char** argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    try {
        const fs::path hint = (argc > 0 && argv[0]) ? fs::path(argv[0]) : fs::path{};
        const fs::path root = findRoot(hint);
        const fs::path inputDir = fs::exists(root / "02_work" / "02_Input data")
                                      ? (root / "02_work" / "02_Input data")
                                      : (root / "02_work" / "Input data");
        const fs::path covPath = inputDir / "04_env_covariates.csv";

        std::cout << "=== 回归克里金（Regression Kriging, C++17） ===\n"
                  << "项目根目录: " << root.u8string() << "\n";

        if (!fs::exists(covPath)) {
            std::cout
                << "\n[等待环境协变量文件] 未检测到: " << covPath.u8string() << "\n"
                << "请等待任务卡 #2（刘文昊）提交 04_env_covariates.csv 并放入上述目录后一键运行。\n"
                << "期望字段列: point_id, dist_road_m, building_density, green_ratio\n"
                << "当前工程已编译就绪，无需修改代码即可直接执行回归克里金与 LOOCV 对比。\n";
            return 0;
        }

        // 读取空间坐标 + 协变量
        std::map<int, std::pair<double, double>> coords;
        for (const auto& r : readCsv(inputDir / "01_spatial_base_points.csv")) {
            coords[std::stoi(r.at("point_id"))] = {std::stod(r.at("utm_e")), std::stod(r.at("utm_n"))};
        }

        struct Cov { double road, bld, grn; };
        std::map<int, Cov> covMap;
        for (const auto& r : readCsv(covPath)) {
            int id = std::stoi(r.at("point_id"));
            covMap[id] = {std::stod(r.at("dist_road_m")),
                          std::stod(r.at("building_density")),
                          std::stod(r.at("green_ratio"))};
        }

        std::map<std::string, std::vector<RkSample>> data;
        for (const auto& r : readCsv(inputDir / "02_spatiotemporal_interp.csv")) {
            int id = std::stoi(r.at("point_id"));
            if (!coords.count(id) || !covMap.count(id)) continue;
            data[r.at("period")].push_back({
                id, coords[id].first, coords[id].second, std::stod(r.at("leq_energy")),
                covMap[id].road, covMap[id].bld, covMap[id].grn
            });
        }

        const fs::path tableDir = root / "outputs" / "tables";
        fs::create_directories(tableDir);
        std::ofstream metOut(tableDir / "rk_loocv_metrics.csv");
        metOut << "\xEF\xBB\xBF"
               << "method,period,n,OLS_R2,beta0,beta_road,beta_building,beta_green,MAE,RMSE,R2,ME,MaxErr\n"
               << std::fixed << std::setprecision(4);

        for (const std::string& period : {"morning", "noon", "night"}) {
            const auto& samples = data[period];
            if (samples.size() < 5) continue;
            const OlsModel ols = fitOls(samples);
            std::vector<double> preds;
            const LoocvMetrics m = evaluateRkLoocv(samples, preds);
            std::cout << "[" << period << "] OLS R2=" << std::setprecision(3) << ols.r2
                      << " | RK LOOCV RMSE=" << m.rmse << " MAE=" << m.mae << " R2=" << m.r2 << "\n";
            metOut << "RK," << period << "," << m.count << "," << ols.r2 << ","
                   << ols.beta0 << "," << ols.betaRoad << "," << ols.betaBuilding << "," << ols.betaGreen << ","
                   << m.mae << "," << m.rmse << "," << m.r2 << "," << m.me << "," << m.maxErr << "\n";
        }
        std::cout << "已导出: " << (tableDir / "rk_loocv_metrics.csv").u8string() << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << "\n";
        return 1;
    }
}
