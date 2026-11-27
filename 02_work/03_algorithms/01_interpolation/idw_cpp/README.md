# IDW 空间插值与 LOOCV 精度评价（C++17 项目）

苏州科技大学《GIS 算法基础》期末大作业（石湖校区校园噪声制图）模块 1 —— 反距离加权插值（IDW）C++ 实现，提供 **Visual Studio 2019/2022 工程** 与 **CMake 跨平台构建** 两套完整配置。

## 项目结构

```text
idw_cpp/
├── IDWInterpolation.sln              # Visual Studio 解决方案（双击打开即可编译运行）
├── IDWInterpolation.vcxproj          # Visual Studio C++17 项目文件
├── IDWInterpolation.vcxproj.filters  # VS 头文件 / 源文件筛选器
├── CMakeLists.txt                    # CMake 构建脚本（CLion / VS Code / MinGW / Linux）
├── include/
│   ├── types.h                       # 核心数据结构（Sample, IdwConfig, LoocvMetrics 等）
│   ├── csv_io.h                      # CSV 解析与仓库根目录定位声明
│   ├── idw.h                         # IDW 插值与 LOOCV 交叉验证声明
│   └── raster_io.h                   # ESRI ASCII Grid (.asc + .prj) 输出声明
└── src/
    ├── csv_io.cpp                    # UTF-8 CSV 读取与路径定位实现
    ├── idw.cpp                       # IDW 核心算法与 LOOCV 实现
    ├── raster_io.cpp                 # 栅格与 EPSG:32651 投影文件输出实现
    └── main.cpp                      # 命令行入口与批量调度
```

## 输入与输出

程序会自动向上查找仓库根目录（兼容 `02_work/02_Input data/` 与 `02_work/Input data/`），无需修改路径：

- **输入**：
  - `02_work/02_Input data/01_spatial_base_points.csv`（32 点 UTM 51N 坐标）
  - `02_work/02_Input data/02_spatiotemporal_interp.csv`（早/中/晚声级主表，默认 `leq_energy`）
- **输出**：
  - `outputs/rasters/idw_<period>_p<power>.asc` 及同名 `.prj`（EPSG:32651，可直接拖入 ArcGIS / QGIS）
  - `outputs/tables/idw_loocv_metrics.csv`（各时段 × 幂指数的 MAE / RMSE / R² / ME / MaxErr）
  - `outputs/tables/idw_loocv_residuals.csv`（逐点留一预测值与残差）

## 编译与运行

### 方式一：Visual Studio 2019 / 2022

1. 双击打开 `IDWInterpolation.sln`；
2. 顶部切换配置为 `Release | x64`；
3. 按 `Ctrl+Shift+B` 编译，按 `Ctrl+F5` 直接运行。

### 方式二：命令行 MSBuild（VS 2019）

```powershell
& "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe" IDWInterpolation.sln /p:Configuration=Release /p:Platform=x64
.\x64\Release\IDWInterpolation.exe
```

### 方式三：CMake

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
./build/Release/IDWInterpolation --powers 1.5,2,3 --cell 5 --buffer 100
```

## 可选命令行参数

| 参数 | 默认值 | 说明 |
| :--- | :--- | :--- |
| `--field` | `leq_energy` | 插值字段名（如 `L10`、`L50`、`L90`、`band_low_100_315_dB` 等） |
| `--powers` | `1.5,2.0,3.0` | 逗号分隔的 IDW 幂指数列表 |
| `--cell` | `5.0` | 输出栅格像元分辨率（m） |
| `--buffer` | `100.0` | 采样点外包框外扩距离（m） |
| `--neighbors` | `0` | 最近邻点数（`0` 表示使用全部点） |
| `--radius` | `0` | 最大搜索半径（m，`0` 表示不限制） |
