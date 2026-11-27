#include "csv_io.h"

#include <fstream>
#include <stdexcept>
#include <utility>

namespace fs = std::filesystem;

namespace noise_kriging {

namespace {

std::vector<std::string> splitCsvLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string current;
    bool inQuotes = false;
    for (char c : line) {
        if (c == '"') {
            inQuotes = !inQuotes;
        } else if (c == ',' && !inQuotes) {
            fields.push_back(current);
            current.clear();
        } else if (c != '\r') {
            current.push_back(c);
        }
    }
    fields.push_back(current);
    return fields;
}

bool hasInputDir(const fs::path& root) {
    return fs::exists(root / "02_work" / "02_Input data") ||
           fs::exists(root / "02_work" / "Input data");
}

}  // namespace

fs::path findProjectRoot(const fs::path& startHint) {
    std::vector<fs::path> candidates;
    if (!startHint.empty()) {
        std::error_code ec;
        fs::path p = fs::weakly_canonical(startHint, ec);
        if (!ec) {
            if (fs::is_regular_file(p, ec)) p = p.parent_path();
            candidates.push_back(p);
        }
    }
    candidates.push_back(fs::current_path());

    for (fs::path cur : candidates) {
        for (int depth = 0; depth < 10; ++depth) {
            if (hasInputDir(cur)) return cur;
            if (!cur.has_parent_path() || cur.parent_path() == cur) break;
            cur = cur.parent_path();
        }
    }
    throw std::runtime_error(
        "未找到仓库根目录（需包含 02_work/02_Input data 或 02_work/Input data），请在项目目录内运行。");
}

fs::path resolveInputDir(const fs::path& projectRoot) {
    fs::path p1 = projectRoot / "02_work" / "02_Input data";
    if (fs::exists(p1)) return p1;
    fs::path p2 = projectRoot / "02_work" / "Input data";
    if (fs::exists(p2)) return p2;
    throw std::runtime_error("未找到标准输入数据目录: " + p1.u8string());
}

std::vector<CsvRow> readCsv(const fs::path& filePath) {
    std::ifstream in(filePath, std::ios::binary);
    if (!in) throw std::runtime_error("无法打开文件: " + filePath.u8string());

    std::string line;
    if (!std::getline(in, line)) throw std::runtime_error("文件为空: " + filePath.u8string());

    if (line.size() >= 3 &&
        static_cast<unsigned char>(line[0]) == 0xEF &&
        static_cast<unsigned char>(line[1]) == 0xBB &&
        static_cast<unsigned char>(line[2]) == 0xBF) {
        line.erase(0, 3);
    }

    const auto headers = splitCsvLine(line);
    std::vector<CsvRow> rows;
    while (std::getline(in, line)) {
        if (line.empty() || line == "\r") continue;
        const auto cols = splitCsvLine(line);
        CsvRow row;
        for (size_t i = 0; i < headers.size() && i < cols.size(); ++i) {
            row[headers[i]] = cols[i];
        }
        rows.push_back(std::move(row));
    }
    return rows;
}

std::map<std::string, std::vector<Sample>> loadPeriodSamples(
    const fs::path& inputDir,
    const std::string& valueField) {
    std::map<int, std::pair<double, double>> coords;
    for (const auto& row : readCsv(inputDir / "01_spatial_base_points.csv")) {
        int id = std::stoi(row.at("point_id"));
        coords[id] = {std::stod(row.at("utm_e")), std::stod(row.at("utm_n"))};
    }

    std::map<std::string, std::vector<Sample>> periodMap;
    for (const auto& row : readCsv(inputDir / "02_spatiotemporal_interp.csv")) {
        auto valIt = row.find(valueField);
        if (valIt == row.end()) throw std::runtime_error("字段不存在: " + valueField);
        if (valIt->second.empty()) continue;

        int id = std::stoi(row.at("point_id"));
        auto coordIt = coords.find(id);
        if (coordIt == coords.end()) continue;

        periodMap[row.at("period")].push_back(
            {id, coordIt->second.first, coordIt->second.second, std::stod(valIt->second)});
    }
    return periodMap;
}

}  // namespace noise_kriging
