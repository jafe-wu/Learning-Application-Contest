// PalletAlgorithm.h
#ifndef ITXUEXI_PALLETALGORITHM_H
#define ITXUEXI_PALLETALGORITHM_H
#pragma once
#include <vector>
#include "ItemDef.h"

// 拓扑轮廓线段
struct LineSegment {
    double x1, x2;
    double height;
};

// 已放置货物的包围盒，用于悬空判定
struct PlacedItem {
    double x, y, width, height;
};

class PalletSpace {
private:
    // 性能优化：将 std::list 替换为 std::vector 以提升 CPU 缓存命中率
    std::vector<LineSegment> skyline;

    // 最小烟型宽度 — 宽度小于此值的"死区"无法容纳任何新物件（默认 77.3，可动态设置）
    static double DEAD_ZONE_WIDTH;

    // 列累计高度表：每 COL_WIDTH mm 一列，记录该位置累计堆叠的总高度
    // 用于在候选评价中惩罚已经堆得很高的列，迫使算法向低列扩散
    static constexpr int    COL_COUNT = 44;        // 440 mm / 10 mm
    static constexpr double COL_WIDTH = 10.0;
    std::vector<double> columnHeights;

    // 已放置货物列表，用于真实支撑判定（不被死区抹平污染）
    std::vector<PlacedItem> placedItems;

    void UpdateSkyline(const GrabTask& task);
    void UpdateColumnHeights(const GrabTask& task);
    // 抹平狭窄凹陷：把窄于 DEAD_ZONE_WIDTH 的下凹段拉到左右邻居的较高者
    void FlattenDeadZones();

public:
    static constexpr double MAX_WIDTH  = 440.0;
    static constexpr double MAX_HEIGHT = 140.0;

    static void SetDeadZoneWidth(double w) { DEAD_ZONE_WIDTH = w; }

    PalletSpace();
    bool TryInsert(GrabTask& task);
    bool IsEmpty() const;
};
#endif //ITXUEXI_PALLETALGORITHM_H