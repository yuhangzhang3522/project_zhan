// =============================================================================
// idw.cpp  -  反距离加权插值（IDW）+ LOOCV 留一法精度评价
// 项目：GIS 算法基础期末大作业 · 苏州科技大学石湖校区噪声制图
// 模块：02_work/algorithms/01_interpolation/
//
// 输入（相对仓库根目录）：
//   02_work/Input data/01_spatial_base_points.csv   32 点固定坐标（utm_e, utm_n, EPSG:32651）
//   02_work/Input data/02_spatiotemporal_interp.csv 96 行早/中/晚声级（默认插值字段 leq_energy）
// 输出：
//   outputs/rasters/idw_<period>_p<power>.asc       ESRI ASCII Grid（EPSG:32651，可直接拖入 ArcGIS/QGIS）
//   outputs/tables/idw_loocv_metrics.csv            各时段 × 各幂指数的 MAE / RMSE / R² / MaxErr
//   outputs/tables/idw_loocv_residuals.csv          每个点位的留一预测值与残差
//
// 编译（C++17）：
//   g++ -O2 -std=c++17 idw.cpp -o idw
//   或 MSVC： cl /O2 /std:c++17 /EHsc /utf-8 idw.cpp
// 运行：
//   ./idw                         在仓库内任意子目录运行，程序自动向上查找仓库根目录
//   ./idw --field L50 --cell 2 --buffer 80 --powers 1.5,2,3 --neighbors 12
// =============================================================================

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// ----------------------------------------------------------------------------- 数据结构
struct Sample {
    int id;
    double x, y;   // UTM 51N (m)
    double z;      // 声级 dB(A)
};

struct Config {
    std::string field = "leq_energy";
    std::vector<double> powers = {1.5, 2.0, 3.0};
    double cell = 5.0;        // 栅格分辨率 (m)
    double buffer = 100.0;    // 点位外包框外扩 (m)
    int neighbors = 0;        // 0 = 使用全部点；>0 = 仅用最近 k 个点
    double searchRadius = 0;  // 0 = 不限制搜索半径
};

struct Metrics {
    double mae = 0, rmse = 0, r2 = 0, maxErr = 0, me = 0;
};

// ----------------------------------------------------------------------------- CSV 工具
static std::vector<std::string> splitCsv(const std::string& line) {
    std::vector<std::string> out;
    std::string cur;
    bool inQuote = false;
    for (char c : line) {
        if (c == '"') inQuote = !inQuote;
        else if (c == ',' && !inQuote) { out.push_back(cur); cur.clear(); }
        else if (c != '\r') cur += c;
    }
    out.push_back(cur);
    return out;
}

static std::vector<std::map<std::string, std::string>> readCsv(const fs::path& p) {
    std::ifstream in(p);
    if (!in) throw std::runtime_error("无法打开文件: " + p.u8string());
    std::string line;
    if (!std::getline(in, line)) throw std::runtime_error("空文件: " + p.u8string());
    if (line.size() >= 3 && (unsigned char)line[0] == 0xEF) line = line.substr(3);  // 去掉 UTF-8 BOM
    auto header = splitCsv(line);
    std::vector<std::map<std::string, std::string>> rows;
    while (std::getline(in, line)) {
        if (line.empty() || line == "\r") continue;
        auto cols = splitCsv(line);
        std::map<std::string, std::string> r;
        for (size_t i = 0; i < header.size() && i < cols.size(); ++i) r[header[i]] = cols[i];
        rows.push_back(std::move(r));
    }
    return rows;
}

// ----------------------------------------------------------------------------- 仓库根目录定位
static fs::path findProjectRoot() {
    fs::path p = fs::current_path();
    for (int i = 0; i < 8; ++i) {
        if (fs::exists(p / "02_work" / "Input data")) return p;
        if (!p.has_parent_path() || p.parent_path() == p) break;
        p = p.parent_path();
    }
    throw std::runtime_error("未找到仓库根目录（需包含 02_work/Input data），请在仓库内运行。");
}

// ----------------------------------------------------------------------------- IDW 核心
// 估计点 (x0, y0) 处的值；excludeIdx >= 0 时跳过该样本（用于 LOOCV）
static double idwEstimate(const std::vector<Sample>& pts, double x0, double y0,
                          double power, const Config& cfg, int excludeIdx = -1) {
    std::vector<std::pair<double, int>> cand;  // (距离, 下标)
    cand.reserve(pts.size());
    for (int i = 0; i < (int)pts.size(); ++i) {
        if (i == excludeIdx) continue;
        double d = std::hypot(pts[i].x - x0, pts[i].y - y0);
        if (d < 1e-9) return pts[i].z;  // 与样本点重合，直接返回实测值（精确插值）
        if (cfg.searchRadius > 0 && d > cfg.searchRadius) continue;
        cand.emplace_back(d, i);
    }
    if (cand.empty()) return std::numeric_limits<double>::quiet_NaN();

    if (cfg.neighbors > 0 && (int)cand.size() > cfg.neighbors) {
        std::nth_element(cand.begin(), cand.begin() + cfg.neighbors, cand.end());
        cand.resize(cfg.neighbors);
    }

    double num = 0.0, den = 0.0;
    for (auto& [d, i] : cand) {
        double w = 1.0 / std::pow(d, power);
        num += w * pts[i].z;
        den += w;
    }
    return num / den;
}

// ----------------------------------------------------------------------------- LOOCV
static Metrics loocv(const std::vector<Sample>& pts, double power, const Config& cfg,
                     std::vector<double>& predOut) {
    const int n = (int)pts.size();
    predOut.assign(n, 0.0);
    double sumAbs = 0, sumSq = 0, sumErr = 0, maxErr = 0, mean = 0;
    for (auto& s : pts) mean += s.z;
    mean /= n;
    double ssTot = 0;
    int valid = 0;
    for (int i = 0; i < n; ++i) {
        double pred = idwEstimate(pts, pts[i].x, pts[i].y, power, cfg, i);
        predOut[i] = pred;
        if (std::isnan(pred)) continue;
        double e = pred - pts[i].z;
        sumAbs += std::fabs(e);
        sumSq += e * e;
        sumErr += e;
        maxErr = std::max(maxErr, std::fabs(e));
        ssTot += (pts[i].z - mean) * (pts[i].z - mean);
        ++valid;
    }
    Metrics m;
    if (valid == 0) return m;
    m.mae = sumAbs / valid;
    m.rmse = std::sqrt(sumSq / valid);
    m.me = sumErr / valid;
    m.maxErr = maxErr;
    m.r2 = ssTot > 0 ? 1.0 - sumSq / ssTot : 0.0;
    return m;
}

// ----------------------------------------------------------------------------- 栅格输出（ESRI ASCII Grid）
static void writeAsciiGrid(const fs::path& file, const std::vector<Sample>& pts,
                           double power, const Config& cfg) {
    double minX = 1e300, minY = 1e300, maxX = -1e300, maxY = -1e300;
    for (auto& s : pts) {
        minX = std::min(minX, s.x); maxX = std::max(maxX, s.x);
        minY = std::min(minY, s.y); maxY = std::max(maxY, s.y);
    }
    minX -= cfg.buffer; minY -= cfg.buffer; maxX += cfg.buffer; maxY += cfg.buffer;
    int ncols = (int)std::ceil((maxX - minX) / cfg.cell);
    int nrows = (int)std::ceil((maxY - minY) / cfg.cell);
    const double nodata = -9999.0;

    std::ofstream out(file);
    if (!out) throw std::runtime_error("无法写入: " + file.u8string());
    out << "ncols " << ncols << "\n"
        << "nrows " << nrows << "\n"
        << std::fixed << std::setprecision(3)
        << "xllcorner " << minX << "\n"
        << "yllcorner " << minY << "\n"
        << "cellsize " << cfg.cell << "\n"
        << "NODATA_value " << nodata << "\n"
        << std::setprecision(2);

    // ASCII Grid 从上往下逐行写；像元值取像元中心
    for (int r = 0; r < nrows; ++r) {
        double y = minY + (nrows - r - 0.5) * cfg.cell;
        for (int c = 0; c < ncols; ++c) {
            double x = minX + (c + 0.5) * cfg.cell;
            double v = idwEstimate(pts, x, y, power, cfg);
            out << (std::isnan(v) ? nodata : v) << (c + 1 < ncols ? " " : "");
        }
        out << "\n";
    }

    // 同名 .prj，写入 EPSG:32651 投影信息，GIS 软件可自动识别坐标系
    fs::path prj = file;
    prj.replace_extension(".prj");
    std::ofstream(prj)
        << "PROJCS[\"WGS_1984_UTM_Zone_51N\",GEOGCS[\"GCS_WGS_1984\",DATUM[\"D_WGS_1984\","
           "SPHEROID[\"WGS_1984\",6378137.0,298.257223563]],PRIMEM[\"Greenwich\",0.0],"
           "UNIT[\"Degree\",0.0174532925199433]],PROJECTION[\"Transverse_Mercator\"],"
           "PARAMETER[\"False_Easting\",500000.0],PARAMETER[\"False_Northing\",0.0],"
           "PARAMETER[\"Central_Meridian\",123.0],PARAMETER[\"Scale_Factor\",0.9996],"
           "PARAMETER[\"Latitude_Of_Origin\",0.0],UNIT[\"Meter\",1.0]]";

    std::cout << "  栅格: " << file.filename().u8string() << " (" << ncols << " x " << nrows << ")\n";
}

// ----------------------------------------------------------------------------- 参数解析
static Config parseArgs(int argc, char** argv) {
    Config cfg;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&]() -> std::string {
            if (i + 1 >= argc) throw std::runtime_error("参数缺少取值: " + a);
            return argv[++i];
        };
        if (a == "--field") cfg.field = next();
        else if (a == "--cell") cfg.cell = std::stod(next());
        else if (a == "--buffer") cfg.buffer = std::stod(next());
        else if (a == "--neighbors") cfg.neighbors = std::stoi(next());
        else if (a == "--radius") cfg.searchRadius = std::stod(next());
        else if (a == "--powers") {
            cfg.powers.clear();
            std::stringstream ss(next());
            std::string t;
            while (std::getline(ss, t, ',')) cfg.powers.push_back(std::stod(t));
        } else if (a == "-h" || a == "--help") {
            std::cout << "用法: idw [--field leq_energy] [--powers 1.5,2,3] [--cell 5] "
                         "[--buffer 100] [--neighbors 0] [--radius 0]\n";
            std::exit(0);
        } else throw std::runtime_error("未知参数: " + a);
    }
    return cfg;
}

static std::string powerTag(double p) {
    std::ostringstream os;
    os << p;
    std::string s = os.str();
    std::replace(s.begin(), s.end(), '.', '_');
    return s;
}

// ----------------------------------------------------------------------------- 主程序
int main(int argc, char** argv) {
    try {
        Config cfg = parseArgs(argc, argv);
        fs::path root = findProjectRoot();
        fs::path inputDir = root / "02_work" / "Input data";
        fs::path rasterDir = root / "outputs" / "rasters";
        fs::path tableDir = root / "outputs" / "tables";
        fs::create_directories(rasterDir);
        fs::create_directories(tableDir);

        // 1) 读取 32 点固定坐标
        std::map<int, std::pair<double, double>> coords;
        for (auto& r : readCsv(inputDir / "01_spatial_base_points.csv"))
            coords[std::stoi(r.at("point_id"))] = {std::stod(r.at("utm_e")), std::stod(r.at("utm_n"))};

        // 2) 读取早/中/晚声级，按时段分组
        const std::vector<std::string> periods = {"morning", "noon", "night"};
        std::map<std::string, std::vector<Sample>> data;
        for (auto& r : readCsv(inputDir / "02_spatiotemporal_interp.csv")) {
            if (!r.count(cfg.field)) throw std::runtime_error("字段不存在: " + cfg.field);
            if (r[cfg.field].empty()) continue;
            int id = std::stoi(r.at("point_id"));
            auto it = coords.find(id);
            if (it == coords.end()) continue;
            data[r.at("period")].push_back({id, it->second.first, it->second.second, std::stod(r[cfg.field])});
        }

        std::cout << "仓库根目录: " << root.u8string() << "\n插值字段: " << cfg.field
                  << " | 分辨率 " << cfg.cell << " m | 外扩 " << cfg.buffer << " m\n";

        // 3) LOOCV + 栅格输出
        std::ofstream met(tableDir / "idw_loocv_metrics.csv");
        std::ofstream res(tableDir / "idw_loocv_residuals.csv");
        met << "method,period,power,n,MAE,RMSE,R2,ME,MaxErr\n";
        res << "method,period,power,point_id,observed,predicted,residual\n";
        met << std::fixed << std::setprecision(4);
        res << std::fixed << std::setprecision(3);

        for (auto& period : periods) {
            auto& pts = data[period];
            if (pts.size() < 3) { std::cerr << "时段 " << period << " 样本不足，跳过\n"; continue; }
            std::cout << "\n[" << period << "] n = " << pts.size() << "\n";
            for (double p : cfg.powers) {
                std::vector<double> pred;
                Metrics m = loocv(pts, p, cfg, pred);
                std::cout << "  IDW p=" << p << "  RMSE=" << std::setprecision(3) << std::fixed << m.rmse
                          << "  MAE=" << m.mae << "  R2=" << m.r2 << "  MaxErr=" << m.maxErr << "\n";
                met << "IDW," << period << "," << p << "," << pts.size() << "," << m.mae << ","
                    << m.rmse << "," << m.r2 << "," << m.me << "," << m.maxErr << "\n";
                for (size_t i = 0; i < pts.size(); ++i)
                    res << "IDW," << period << "," << p << "," << pts[i].id << "," << pts[i].z << ","
                        << pred[i] << "," << pred[i] - pts[i].z << "\n";

                writeAsciiGrid(rasterDir / ("idw_" + period + "_p" + powerTag(p) + ".asc"), pts, p, cfg);
            }
        }
        std::cout << "\n精度表: " << (tableDir / "idw_loocv_metrics.csv").u8string() << "\n完成。\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << "\n";
        return 1;
    }
}
