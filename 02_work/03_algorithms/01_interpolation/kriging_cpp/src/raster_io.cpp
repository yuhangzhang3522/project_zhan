#include "raster_io.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <stdexcept>

namespace fs = std::filesystem;

namespace noise_kriging {

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

void writePrj(const fs::path& ascPath) {
    fs::path prjPath = ascPath;
    prjPath.replace_extension(".prj");
    std::ofstream prjOut(prjPath);
    if (prjOut) prjOut << kUtm51nPrjWkt;
}

}  // namespace

std::pair<int, int> writeKrigingRasters(const fs::path& valAscPath,
                                        const fs::path& varAscPath,
                                        const std::vector<Sample>& samples,
                                        const OrdinaryKrigingSolver& solver,
                                        const KrigingConfig& cfg) {
    if (samples.empty()) {
        throw std::runtime_error("样本为空，无法生成克里金栅格。");
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

    std::ofstream valOut(valAscPath);
    std::ofstream varOut(varAscPath);
    if (!valOut || !varOut) {
        throw std::runtime_error("无法写入克里金栅格文件。");
    }

    auto writeHeader = [&](std::ofstream& out) {
        out << "ncols " << ncols << "\n"
            << "nrows " << nrows << "\n"
            << std::fixed << std::setprecision(3)
            << "xllcorner " << minX << "\n"
            << "yllcorner " << minY << "\n"
            << "cellsize " << cfg.cellSize << "\n"
            << "NODATA_value " << nodata << "\n"
            << std::setprecision(2);
    };
    writeHeader(valOut);
    writeHeader(varOut);

    for (int r = 0; r < nrows; ++r) {
        const double y = minY + (nrows - r - 0.5) * cfg.cellSize;
        for (int c = 0; c < ncols; ++c) {
            const double x = minX + (c + 0.5) * cfg.cellSize;
            bool inside = true;
            if (cfg.maxExtrapolation > 0.0) {
                double minDist = std::numeric_limits<double>::max();
                for (const auto& s : samples) {
                    minDist = std::min(minDist, std::hypot(s.x - x, s.y - y));
                }
                inside = (minDist <= cfg.maxExtrapolation);
            }

            if (inside) {
                const KrigingEstimate est = solver.predict(x, y);
                valOut << (std::isnan(est.value) ? nodata : est.value);
                varOut << (std::isnan(est.variance) ? nodata : est.variance);
            } else {
                valOut << nodata;
                varOut << nodata;
            }
            if (c + 1 < ncols) {
                valOut << ' ';
                varOut << ' ';
            }
        }
        valOut << '\n';
        varOut << '\n';
    }

    writePrj(valAscPath);
    writePrj(varAscPath);
    return {ncols, nrows};
}

}  // namespace noise_kriging
