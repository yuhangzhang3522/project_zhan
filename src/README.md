# 算法与制图源码目录（`src/`）

本目录用于存放阶段二 & 阶段三的 GIS 算法建模、空间统计分析与 300 DPI 专题制图代码：

- `interpolation/`：空间插值与交叉验证（IDW、普通克里金 OK、回归克里金 RK、RBF 及 LOOCV 精度评价）
- `spatial_analysis/`：空间自相关（Global Moran's I、Getis-Ord $G_i^*$ 热点）、多源距离衰减与频谱特征、道路/建筑/绿地空间关联分析
- `visualization/`：叠加正射影像底图的 300 DPI 规范专题地图与统计分析图绘制脚本

## 跨电脑相对路径写法规范（必用）

为确保张宇航、刘文昊、朱万里三台电脑无需改路径即可直接运行脚本，请在所有 Python 脚本开头统一用 `pathlib` 定位仓库根目录：

```python
from pathlib import Path

# 自动定位到 project_zhan 仓库根目录（假设当前脚本位于 src/子目录/xxx.py）
PROJECT_ROOT = Path(__file__).resolve().parents[2]

# 标准输入数据路径
CLEAN_CSV = PROJECT_ROOT / "02_work" / "Input data" / "02_spatiotemporal_interp.csv"
BASEMAP_TIF = PROJECT_ROOT / "02_work" / "data preprocessing" / "01_data_range" / "campus" / "石湖校区0.24.tif"

# 标准输出目录
FIGURES_DIR = PROJECT_ROOT / "outputs" / "figures"
TABLES_DIR = PROJECT_ROOT / "outputs" / "tables"
RASTERS_DIR = PROJECT_ROOT / "outputs" / "rasters"
```
