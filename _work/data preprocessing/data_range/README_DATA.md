# 正射影像底图放置说明

由于 `石湖校区0.24.tif` 体积约 **96.4 MiB**，逼近 GitHub `100 MiB` 单文件大小限制，仓库已在 `.gitignore` 中忽略所有 `*.tif` 栅格文件。

克隆仓库后，请从**小组共享文件 / 网盘**获取底图原文件，并将其放置在本目录下，保持文件名严格一致：

- **目标路径**：`_work/data preprocessing/data_range/石湖校区0.24.tif`
- **栅格尺寸**：`10008 × 6979` RGB
- **空间分辨率**：`0.24 m`
- **投影坐标系**：`WGS 84 / UTM zone 51N (EPSG:32651)`
- **左上角坐标**：`E = 267617.0, N = 3460864.011`

同目录下的采样点位文件已随 Git 正常同步：
- `Orthophoto_point_location.csv`：点位坐标表
- `points_final.json`：32 个最终布设点位 JSON
- `points_final.png`：32 个采样点位布设示意图
