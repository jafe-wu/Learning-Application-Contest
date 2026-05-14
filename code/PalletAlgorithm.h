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

class PalletSpace {
private:
    // 性能优化：将 std::list 替换为 std::vector 以提升 CPU 缓存命中率
    std::vector<LineSegment> skyline;

    // 最小烟型宽度 — 宽度小于此值的"死区"无法容纳任何新物件（默认 77.3，可动态设置）
    static double DEAD_ZONE_WIDTH;

    void UpdateSkyline(const GrabTask& task);
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