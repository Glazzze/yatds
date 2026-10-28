# common/stats：性能基准用的轻量统计工具

`stats.hpp`（header-only，非任务代码）为性能基准程序提供：

- `mean` / `sdPop`：均值与总体标准差（与基准表格口径一致）；
- `varSample`：样本方差（除以 `n-1`，供 t 检验用）；
- `welch(a, b, conf=0.95)`：**Welch 两样本 t 检验**，返回 t 统计量、Welch–Satterthwaite 自由度、双侧 p 值，以及均值差的 95% 置信区间。

**为何用 Welch 而非 Student**：自实现与 `std::vector`（或链表）在不同场景下方差并不相等，Welch 检验不要求方差齐性。t 分布 CDF 由“正则化不完全 Beta 函数 + 连分式”实现，分位数用二分法求出。

被 [task2_performance](../../task2_performance/) 与 [task4_array_vs_list](../../task4_array_vs_list/) 以相对路径 `../common/stats/stats.hpp` 引用。