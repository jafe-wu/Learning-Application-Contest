// OrderLoader.h
#pragma once
#include <vector>
#include <string>
#include "ItemDef.h"
#include "DimDB.h"

class OrderLoader {
public:
    // 从制表符分隔的订单文件加载，按数量展开，缺尺寸的条目记入 warnings
    bool LoadFromTSV(const std::string& path, const DimDB& dims,
                     std::vector<CigaretteItem>& out,
                     std::vector<std::string>& warnings);
};
