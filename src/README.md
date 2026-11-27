# 制图脚本目录（`src/`）

算法代码统一放在 `02_work/03_algorithms/` 下的模块目录：

- `01_interpolation/`：空间插值与交叉验证（IDW、普通克里金 OK、回归克里金 RK、RBF 及 LOOCV 精度评价）
- `02_spatial_analysis/`：空间自相关（Global Moran's I、Getis-Ord $G_i^*$ 热点）、多源距离衰减与道路/建筑/绿地空间关联分析
- `03_spectrum/`：1/3 倍频程频谱指纹、频带能量占比与分频距离衰减拟合

本目录只存放专题制图与统计图表脚本：

- `visualization/`：叠加正射影像底图的 300 DPI 规范专题地图与统计分析图绘制脚本

## 跨电脑相对路径写法规范（必用）

为确保张宇航、刘文昊、朱万里三台电脑无需改路径即可直接运行脚本，请在所有 Python 脚本开头统一用 `pathlib` 定位仓库根目录。目录层级不同，`parents` 层数也不同：

```python
from pathlib import Path

# 情况 A：脚本位于 02_work/03_algorithms/<模块目录>/xxx.py
PROJECT_ROOT = Path(__file__).resolve().parents[3]
```

```python
from pathlib import Path

# 情况 B：脚本位于 src/visualization/xxx.py
PROJECT_ROOT = Path(__file__).resolve().parents[2]
```

两种位置下的标准路径写法一致：

```python
# 标准输入数据路径
CLEAN_CSV = PROJECT_ROOT / "02_work" / "Input data" / "02_spatiotemporal_interp.csv"
BASEMAP_TIF = PROJECT_ROOT / "02_work" / "data preprocessing" / "01_data_range" / "campus" / "石湖校区0.24.tif"

# 标准输出目录
FIGURES_DIR = PROJECT_ROOT / "outputs" / "figures"
TABLES_DIR = PROJECT_ROOT / "outputs" / "tables"
RASTERS_DIR = PROJECT_ROOT / "outputs" / "rasters"
```
