// DimDB.h
#pragma once
#include <string>
#include <unordered_map>

struct DimEntry {
    int raw_l, raw_w, raw_h;
};

class DimDB {
public:
    // 从 CSV 加载（列：物料编号,物料名称,长,宽,高）
    bool LoadFromCSV(const std::string& path);

    // 先按 ID 查，找不到再按名称查
    bool Lookup(const std::string& id, const std::string& name, DimEntry& out) const;

    size_t Size() const { return byId_.size(); }

private:
    std::unordered_map<std::string, DimEntry> byId_;
    std::unordered_map<std::string, DimEntry> byName_;

    static std::string trim(const std::string& s);
};
