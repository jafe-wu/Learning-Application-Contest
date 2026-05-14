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

    // 输入数据集中最小烟型宽度 77.3mm — 宽度小于此值的"死区"无法容纳任何新物件
    static constexpr double DEAD_ZONE_WIDTH = 77.3;

    void UpdateSkyline(const GrabTask& task);
    // 抹平狭窄凹陷：把窄于 DEAD_ZONE_WIDTH 的下凹段拉到左右邻居的较高者
    void FlattenDeadZones();

public:
    static constexpr double MAX_WIDTH  = 440.0;
    static constexpr double MAX_HEIGHT = 140.0;

    PalletSpace();
    bool TryInsert(GrabTask& task);
    bool IsEmpty() const;
};
#endif //ITXUEXI_PALLETALGORITHM_H