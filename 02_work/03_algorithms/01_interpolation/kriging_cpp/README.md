# 普通克里金插值（Ordinary Kriging, C++17 纯标准库工程）

零第三方库依赖的普通克里金（OK）空间插值工程，包含实验半变异函数计算、球状/指数理论模型加权最小二乘（WLS）拟合、手写列主元高斯消元求解器、零拷贝 LOOCV 验证与估值/方差栅格导出（ESRI ASCII Grid + EPSG:32651 `.prj`）。

## 输出文件

- `outputs/tables/ok_variogram_empirical.csv`：早/中/晚实验半变异函数滞后分组表
- `outputs/tables/ok_variogram_params.csv`：球状/指数理论模型参数（块金值、偏基台值、基台值、变程、块基比、拟合误差）
- `outputs/tables/ok_loocv_metrics.csv` / `ok_loocv_residuals.csv`：LOOCV 精度汇总与逐点残差/估计方差
- `outputs/rasters/ok_{morning,noon,night}.asc`（含 `.prj`）：最优模型声级估值栅格（>160 m 置 `NODATA=-9999`）
- `outputs/rasters/ok_var_{morning,noon,night}.asc`（含 `.prj`）：对应克里金估计方差栅格

## 编译运行

双击 `KrigingInterpolation.sln` 在 Visual Studio 中以 `Release | x64` 编译运行，或使用命令行：

```powershell
& "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe" KrigingInterpolation.sln /p:Configuration=Release /p:Platform=x64
.\x64\Release\KrigingInterpolation.exe --radius 160 --cell 5
```
