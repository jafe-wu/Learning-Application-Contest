// PalletizerController.cpp
#include "PalletizerController.h"
#include <chrono>
#include <cmath>
#include <map>

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

    // ── 预扫描：统计每个订单的总面积，计算最优封垛分界点 ──
    const double PALLET_AREA = PalletSpace::MAX_WIDTH * PalletSpace::MAX_HEIGHT;
    const double FILL_TARGET = 0.75; // 目标利用率

    struct OrderInfo {
        int    totalItems = 0;
        double totalArea  = 0.0;
        int    targetPallets = 1;
        // 每个子批应该包含多少"面积"，用于决定分界点
        double areaPerPallet = 0;
    };
    std::map<std::string, OrderInfo> orderInfo;
    for (const auto& item : queue) {
        auto& oi = orderInfo[item.order_id];
        oi.totalItems++;
        oi.totalArea += item.width * item.height;
    }
    for (auto& kv : orderInfo) {
        kv.second.targetPallets = std::max(1,
            (int)std::ceil(kv.second.totalArea / (PALLET_AREA * FILL_TARGET)));
        kv.second.areaPerPallet = kv.second.totalArea / kv.second.targetPallets;
    }

    // 对每个订单，计算每个 item 的分界点
    // 策略：将 items 按面积均分为 targetPallets 批，每批 item 数尽量接近
    // boundary[order_id] = vector<int>，第 k 个元素表示第 k 批结束时的累计 item 数
    std::map<std::string, std::vector<int>> boundaries;
    for (auto& kv : orderInfo) {
        const std::string& oid = kv.first;
        const OrderInfo& oi = kv.second;
        std::vector<int>& bd = boundaries[oid];
        if (oi.targetPallets <= 1) continue;

        // 按面积累计，尽量让每批面积接近 areaPerPallet
        double accumArea = 0;
        int countInBatch = 0;
        for (const auto& item : queue) {
            if (item.order_id != oid) continue;
            countInBatch++;
            accumArea += item.width * item.height;

            int remainingItems = oi.totalItems - countInBatch;
            int remainingBatches = oi.targetPallets - (int)bd.size() - 1;

            if (remainingBatches <= 0) break;

            // 到达目标面积时切分；但如果剩余物品不够填满剩余批次，也切
            if (accumArea >= oi.areaPerPallet) {
                // 确保切分后剩余物品还能合理分配
                if (remainingItems >= remainingBatches) {
                    bd.push_back(countInBatch);
                    accumArea = 0;
                }
            }
        }
    }

    int nextId = 1;
    PalletSnapshot current;
    current.palletId = nextId++;
    PalletSpace space;

    // 当前订单状态
    int itemsConsumedInOrder = 0;
    int batchIndex = 0; // 当前是第几批

    size_t i = 0;
    while (i < queue.size()) {
        const CigaretteItem& a = queue[i];

        // 订单隔离墙：当前托盘非空且订单不同 → 封垛
        if (!current.tasks.empty() && current.orderId != a.order_id) {
            SealPallet(current, space, nextId);
            itemsConsumedInOrder = 0;
            batchIndex = 0;
        }
        if (current.tasks.empty()) current.orderId = a.order_id;

        // 新订单开始时初始化
        auto bdIt = boundaries.find(a.order_id);
        if (itemsConsumedInOrder == 0 && batchIndex == 0) {
            // 已在上面初始化
        }

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
            if (space.IsEmpty()) {
                i += consumed;
                itemsConsumedInOrder += (int)consumed;
                continue;
            }
            SealPallet(current, space, nextId);
            batchIndex++;
            current.orderId = a.order_id;
            if (!space.TryInsert(task)) {
                i += consumed;
                itemsConsumedInOrder += (int)consumed;
                continue;
            }
        }

        current.tasks.push_back(task);
        totalGrabs_++;
        itemsConsumedInOrder += (int)consumed;
        i += consumed;

        // ── 均衡封垛：到达分界点时主动封垛 ────────────
        if (bdIt != boundaries.end()) {
            const std::vector<int>& bd = bdIt->second;
            if (batchIndex < (int)bd.size() &&
                itemsConsumedInOrder >= bd[batchIndex]) {
                // 已到分界点，且托盘非空 → 主动封垛
                if (!current.tasks.empty()) {
                    SealPallet(current, space, nextId);
                    batchIndex++;
                }
            }
        }
    }

    // 收尾
    if (!current.tasks.empty()) {
        SealPallet(current, space, nextId);
    }

    runtimeMicros_ =
        std::chrono::duration_cast<std::chrono::microseconds>(
            clock::now() - t0).count();
}
