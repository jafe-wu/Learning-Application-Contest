# Cigarette Pallet Optimization (卷烟码垛优化)

基于天际线算法 (Skyline Algorithm) 的卷烟纸箱码垛优化系统，实现订单隔离、前瞻合并双取、均衡封垛等功能，并提供 EasyX 图形化可视化界面。

## 功能特性

- **天际线装箱算法** — 利用拓扑轮廓线实现 2D 矩形放置，支持底部优先 + 左侧优先策略
- **订单隔离** — 严格按订单分托盘，不同订单绝不混放
- **前瞻合并双取** — 相邻同订单且高度差 ≤1mm 的烟型可一次抓取 2 条，减少机械臂动作
- **均衡封垛** — 按面积均匀分配订单到多个托盘，避免最后托盘过满或过空
- **5mm 禁飞区** — 托盘左右各留 5mm 安全间距
- **EasyX 可视化** — 图形化展示每个托盘的摆放效果，支持交互切换查看

## 项目结构

```
Learning-Application-Contest/
├── .gitignore
├── README.md
├── CONTRIBUTING.md
└── code/
    ├── CMakeLists.txt          # CMake 构建配置
    ├── build.bat               # MSVC 一键编译脚本
    ├── run.bat                 # 编译 + 运行脚本
    ├── main.cpp                # 程序入口
    ├── ItemDef.h               # 数据结构定义 (CigaretteItem, GrabTask)
    ├── DimDB.h / DimDB.cpp     # 尺寸数据库加载与查询
    ├── OrderLoader.h / .cpp    # 订单 TSV 文件解析
    ├── PalletAlgorithm.h / .cpp # 天际线装箱核心算法
    ├── PalletizerController.h / .cpp # 码垛调度控制器
    ├── Visualizer.h / .cpp     # EasyX 图形化可视化
    ├── dims.csv                # 尺寸数据库 (物料编号, 名称, 长, 宽, 高)
    └── orders.tsv              # 订单数据 (制表符分隔)
```

## 环境要求

- **操作系统**: Windows 10/11
- **编译器**: MSVC (Visual Studio 2019 或更高版本)
- **图形库**: [EasyX](https://easyx.cn/) (随 Visual Studio 安装)
- **C++ 标准**: C++17

## 构建与运行

### 方式一：使用 build.bat (推荐)

```batch
# 在项目 code/ 目录下执行
build.bat

# 编译并运行
run.bat
```

> **注意**: `build.bat` 中的 Visual Studio 路径默认为 `D:\Visual Studio\VC\Auxiliary\Build\`，
> 如果你的安装路径不同，请修改 `build.bat` 和 `CMakeLists.txt` 中对应的路径。

### 方式二：使用 CMake + CLion / Visual Studio

1. 用 CLion 或 Visual Studio 打开 `code/` 目录
2. 确保工具链为 MSVC (不支持 MinGW，因为依赖 EasyX)
3. 构建并运行 `main` 目标

## 数据文件格式

### dims.csv (尺寸数据库)

| 列 | 说明 |
|----|------|
| 物料编号 | 唯一标识 |
| 物料名称 | 可读名称 |
| 长 | 0.1mm 为单位的整数 |
| 宽 | 0.1mm 为单位的整数 |
| 高 | 0.1mm 为单位的整数 |

### orders.tsv (订单数据)

制表符分隔，列顺序：订单号、序号、物料编号、物料名称、数量

## 算法说明

1. **预扫描阶段**: 统计每个订单的总面积，计算最优封垛分界点
2. **放置阶段**: 逐条处理队列，通过天际线算法寻找最优放置位置
   - 候选点生成 + 碰撞检测 + 悬空稳态判定
   - 评价函数: Y 最小 (底部优先)，平局 X 最小 (左侧优先)
3. **封垛阶段**: 订单切换或到达分界点时主动封垛，开启新托盘

## License

MIT License
