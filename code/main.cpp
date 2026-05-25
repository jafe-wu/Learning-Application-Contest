// main.cpp
#include <iostream>
#include <vector>
#include <locale>
#include <fstream>
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

    // 设置死区宽度为数据集中的最小烟型宽度
    PalletSpace::SetDeadZoneWidth(dims.MinWidth());
    std::cout << "最小烟型宽度: " << dims.MinWidth() << " mm\n";

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

    // ── 4. 导出码垛数据 CSV ─────────────────────────────
    {
        std::ofstream csv("pallet_result.csv");
        csv << "码垛号,码垛机械手抓取顺序号,订单顺序号,来料顺序号,卷烟名称,抓取数量\n";
        for (const auto& p : controller.GetPallets()) {
            int orderId = 0;
            try { orderId = std::stoi(p.orderId.substr(6)); } catch (...) {}
            for (size_t gi = 0; gi < p.tasks.size(); ++gi) {
                const auto& task = p.tasks[gi];
                for (const auto& item : task.items) {
                    csv << p.palletId << ","
                        << (gi + 1) << ","
                        << orderId << ","
                        << item.seq << ","
                        << item.name << ","
                        << 1 << "\n";
                }
            }
        }
        csv.close();
        std::cout << "\n码垛数据已导出到 pallet_result.csv（可用 Excel 打开）\n";
    }

    // ── 5. 可视化 ──────────────────────────────────────
    RenderInteractive(controller);
    return 0;
}
