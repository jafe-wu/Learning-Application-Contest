// main.cpp
#include <iostream>
#include <vector>
#include <locale>
#include <conio.h>
#include "ItemDef.h"
#include "DimDB.h"
#include "OrderLoader.h"
#include "PalletizerController.h"
#include "Visualizer.h"

int main() {
    std::locale::global(std::locale(""));

    // ── 1. 加载尺寸数据库 ──────────────────────────────
    DimDB dims;
    if (!dims.LoadFromCSV("dims.csv")) {
        std::cerr << "[错误] 无法加载 dims.csv，请确认文件存在于程序同目录\n";
        system("pause");
        return 1;
    }
    std::cout << "尺寸库加载完成，共 " << dims.Size() << " 条记录\n";

    // ── 2. 加载订单队列 ────────────────────────────────
    std::vector<CigaretteItem> queue;
    std::vector<std::string>   warnings;
    OrderLoader loader;
    if (!loader.LoadFromTSV("orders.tsv", dims, queue, warnings)) {
        std::cerr << "[错误] 无法加载 orders.tsv，请确认文件存在于程序同目录\n";
        system("pause");
        return 1;
    }
    if (!warnings.empty()) {
        std::cout << "[警告] 以下物料缺少尺寸数据，已跳过：\n";
        for (const auto& w : warnings)
            std::cout << "  " << w << "\n";
    }
    std::cout << "订单队列加载完成，共 " << queue.size() << " 条烟\n";

    // ── 3. 码垛计算 ────────────────────────────────────
    std::cout << "\n--- 开始码垛计算 ---\n";
    PalletizerController controller;
    controller.ProcessQueue(queue);

    std::cout << "总托盘数: "    << controller.TotalPallets()
              << "  总抓取次数: " << controller.TotalGrabs()
              << "  耗时: "        << controller.RunTimeMicros() << " us\n\n";

    for (const auto& p : controller.GetPallets()) {
        std::cout << "  [垛 " << p.palletId
                  << "] 订单=" << p.orderId
                  << "  抓取=" << p.tasks.size()
                  << "  利用率=" << p.utilizationPct << "%\n";
    }

    // ── 4. 可视化 ──────────────────────────────────────
    RenderInteractive(controller);
    return 0;
}
