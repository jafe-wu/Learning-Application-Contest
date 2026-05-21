//
// PalletizerController.h — 全局中枢调度器
//
#ifndef ITXUEXI_PALLETIZERCONTROLLER_H
#define ITXUEXI_PALLETIZERCONTROLLER_H
#pragma once
#include <vector>
#include <memory>
#include "ItemDef.h"
#include "PalletAlgorithm.h"

// 一个完整托盘的快照：编号 + 所属订单 + 已完成的抓取任务
struct PalletSnapshot {
    int palletId;
    std::string orderId;
    std::vector<GrabTask> tasks;
    double utilizationPct;  // 面积利用率 (%)
};

class PalletizerController {
public:
    PalletizerController();

    // 消费整条进料队列：严格 FIFO，内部完成前瞻合并 + 订单隔离 + 分托盘
    void ProcessQueue(const std::vector<CigaretteItem>& queue);

    const std::vector<PalletSnapshot>& GetPallets() const { return pallets_; }
    int    TotalPallets()   const { return (int)pallets_.size(); }
    int    TotalGrabs()     const { return totalGrabs_; }
    long long RunTimeMicros() const { return runtimeMicros_; }

private:
    static constexpr int MIN_ITEMS_BEFORE_SEAL = 12; // 托盘至少需要填入的物品数

    std::vector<PalletSnapshot> pallets_;
    int       totalGrabs_    = 0;
    long long runtimeMicros_ = 0;

    // 封垛：把当前托盘快照 push 到 pallets_，并开一个新的空托盘
    void SealPallet(PalletSnapshot& current, PalletSpace& space, int& nextId);
};

#endif //ITXUEXI_PALLETIZERCONTROLLER_H
