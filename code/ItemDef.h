//
// Created by 18149 on 2026/5/12.
//

#ifndef ITXUEXI_ITEMDEF_H
#define ITXUEXI_ITEMDEF_H
// ItemDef.h
#pragma once
#include <string>
#include <vector>

// 卷烟实体类
class CigaretteItem {
public:
    std::string id;
    std::string name;
    std::string order_id;
    int    seq;      // 来料顺序号（原始数据中的行号）
    double length;   // 长 (mm)
    double width;    // 宽 (mm)
    double height;   // 高 (mm)

    CigaretteItem(std::string id, std::string n, std::string oid,
                  int seq, int raw_l, int raw_w, int raw_h)
        : id(id), name(n), order_id(oid), seq(seq) {
        // 原始单位 0.1mm，这里统一换算成 mm
        this->length = raw_l / 10.0;
        this->width  = raw_w / 10.0;
        this->height = raw_h / 10.0;
    }
};

// 机械臂抓取任务类 (可能包含 1 条或合并的 2 条)
class GrabTask {
public:
    std::vector<CigaretteItem> items;
    std::string order_id;  // 本次抓取归属的订单号；空 = 尚未归属
    double totalWidth;
    double maxHeight;
    int    grabCount;

    // 最终安置坐标 (mm)
    double x;
    double y;

    GrabTask()
        : totalWidth(0), maxHeight(0), grabCount(0), x(-1.0), y(-1.0) {}

    void addItem(const CigaretteItem& item) {
        if (items.empty()) order_id = item.order_id;
        items.push_back(item);
        totalWidth += item.width;
        if (item.height > maxHeight) maxHeight = item.height;
        grabCount++;
    }
};

#endif //ITXUEXI_ITEMDEF_H
