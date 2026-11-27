#include "raster_io.h"
#include "idw.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <stdexcept>

namespace fs = std::filesystem;

namespace noise_idw {

namespace {

constexpr const char* kUtm51nPrjWkt =
    "PROJCS[\"WGS_1984_UTM_Zone_51N\","
    "GEOGCS[\"GCS_WGS_1984\","
    "DATUM[\"D_WGS_1984\","
    "SPHEROID[\"WGS_1984\",6378137.0,298.257223563]],"
    "PRIMEM[\"Greenwich\",0.0],"
    "UNIT[\"Degree\",0.0174532925199433]],"
    "PROJECTION[\"Transverse_Mercator\"],"
    "PARAMETER[\"False_Easting\",500000.0],"
    "PARAMETER[\"False_Northing\",0.0],"
    "PARAMETER[\"Central_Meridian\",123.0],"
    "PARAMETER[\"Scale_Factor\",0.9996],"
    "PARAMETER[\"Latitude_Of_Origin\",0.0],"
    "UNIT[\"Meter\",1.0]]";

}  // namespace

std::pair<int, int> writeAsciiGrid(const fs::path& ascPath,
                                   const std::vector<Sample>& samples,
                                   double power,
                                   const IdwConfig& cfg) {
    if (samples.empty()) {
        throw std::runtime_error("样本为空，无法生成栅格: " + ascPath.u8string());
    }

    double minX = std::numeric_limits<double>::max();
    double minY = std::numeric_limits<double>::max();
    double maxX = std::numeric_limits<double>::lowest();
    double maxY = std::numeric_limits<double>::lowest();
    for (const auto& s : samples) {
        minX = std::min(minX, s.x);
        maxX = std::max(maxX, s.x);
        minY = std::min(minY, s.y);
        maxY = std::max(maxY, s.y);
    }

    minX -= cfg.buffer;
    minY -= cfg.buffer;
    maxX += cfg.buffer;
    maxY += cfg.buffer;

    const int ncols = static_cast<int>(std::ceil((maxX - minX) / cfg.cellSize));
    const int nrows = static_cast<int>(std::ceil((maxY - minY) / cfg.cellSize));
    const double nodata = -9999.0;

    std::ofstream out(ascPath);
    if (!out) {
        throw std::runtime_error("无法写入栅格文件: " + ascPath.u8string());
    }

    out << "ncols " << ncols << "\n"
        << "nrows " << nrows << "\n"
        << std::fixed << std::setprecision(3)
        << "xllcorner " << minX << "\n"
        << "yllcorner " << minY << "\n"
        << "cellsize " << cfg.cellSize << "\n"
        << "NODATA_value " << nodata << "\n"
        << std::setprecision(2);

    // ESRI ASCII Grid 按从北到南逐行输出，采样点取各像元中心坐标
    for (int r = 0; r < nrows; ++r) {
        const double y = minY + (nrows - r - 0.5) * cfg.cellSize;
        for (int c = 0; c < ncols; ++c) {
            const double x = minX + (c + 0.5) * cfg.cellSize;
            double val = nodata;
            bool insideDomain = true;
            if (cfg.searchRadius > 0.0) {
                double minDist = std::numeric_limits<double>::max();
                for (const auto& s : samples) {
                    minDist = std::min(minDist, std::hypot(s.x - x, s.y - y));
                }
                insideDomain = (minDist <= cfg.searchRadius);
            }
            if (insideDomain) {
                const double est = idwEstimate(samples, x, y, power, cfg);
                val = std::isnan(est) ? nodata : est;
            }
            out << val;
            if (c + 1 < ncols) out << ' ';
        }
        out << '\n';
    }

    fs::path prjPath = ascPath;
    prjPath.replace_extension(".prj");
    std::ofstream prjOut(prjPath);
    if (prjOut) {
        prjOut << kUtm51nPrjWkt;
    }

    return {ncols, nrows};
}

}  // namespace noise_idw
