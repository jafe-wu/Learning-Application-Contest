// PalletizerController.cpp
#include "PalletizerController.h"
#include <chrono>
#include <cmath>

PalletizerController::PalletizerController() {}

void PalletizerController::SealPallet(PalletSnapshot& current,
                                      PalletSpace& space,
                                      int& nextId) {
    if (current.tasks.empty()) return;

    // 面积利用率 = Σ(宽 × 高) / (440 × 140)
    double used = 0.0;
    for (const auto& t : current.tasks) used += t.totalWidth * t.maxHeight;
    current.utilizationPct =
        used / (PalletSpace::MAX_WIDTH * PalletSpace::MAX_HEIGHT) * 100.0;

    pallets_.push_back(current);

    // 重置：新托盘
    current = PalletSnapshot{};
    current.palletId = nextId++;
    space = PalletSpace{};
}

void PalletizerController::ProcessQueue(const std::vector<CigaretteItem>& queue) {
    using clock = std::chrono::steady_clock;
    auto t0 = clock::now();

    pallets_.clear();
    totalGrabs_ = 0;

    if (queue.empty()) {
        runtimeMicros_ =
            std::chrono::duration_cast<std::chrono::microseconds>(
                clock::now() - t0).count();
        return;
    }

    int nextId = 1;
    PalletSnapshot current;
    current.palletId = nextId++;
    PalletSpace space;

    size_t i = 0;
    while (i < queue.size()) {
        const CigaretteItem& a = queue[i];

        // 订单隔离墙：当前托盘非空且订单不同 → 封垛
        if (!current.tasks.empty() && current.orderId != a.order_id) {
            SealPallet(current, space, nextId);
        }
        if (current.tasks.empty()) current.orderId = a.order_id;

        // 前瞻合成：下一条同订单且 |Δh| ≤ 1mm 可双取
        GrabTask task;
        task.addItem(a);
        size_t consumed = 1;
        if (i + 1 < queue.size()) {
            const CigaretteItem& b = queue[i + 1];
            if (b.order_id == a.order_id &&
                std::abs(a.height - b.height) <= 1.0) {
                task.addItem(b);
                consumed = 2;
            }
        }

        // 尝试放置；失败则封垛重试
        if (!space.TryInsert(task)) {
            // 空托盘还放不下 → 数据异常，跳过以免死循环
            if (space.IsEmpty()) {
                i += consumed;
                continue;
            }
            SealPallet(current, space, nextId);
            current.orderId = a.order_id;
            if (!space.TryInsert(task)) {
                // 新的空托盘还是塞不下 → 跳过此任务
                i += consumed;
                continue;
            }
        }

        current.tasks.push_back(task);
        totalGrabs_++;
        i += consumed;
    }

    // 收尾
    if (!current.tasks.empty()) {
        SealPallet(current, space, nextId);
    }

    runtimeMicros_ =
        std::chrono::duration_cast<std::chrono::microseconds>(
            clock::now() - t0).count();
}
