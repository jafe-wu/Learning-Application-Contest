// OrderLoader.cpp
#include "OrderLoader.h"
#include <fstream>
#include <sstream>

bool OrderLoader::LoadFromTSV(const std::string& path, const DimDB& dims,
                               std::vector<CigaretteItem>& out,
                               std::vector<std::string>& warnings) {
    std::ifstream f(path);
    if (!f.is_open()) return false;

    std::string line;
    std::getline(f, line); // skip header

    while (std::getline(f, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string order_id, seq, item_id, item_name, qty_str;
        std::getline(ss, order_id,  '\t');
        std::getline(ss, seq,       '\t');
        std::getline(ss, item_id,   '\t');
        std::getline(ss, item_name, '\t');
        std::getline(ss, qty_str,   '\t');

        if (order_id.empty() || item_id.empty()) continue;

        int qty = 1;
        try { qty = std::stoi(qty_str); } catch (...) {}
        if (qty <= 0) qty = 1;

        DimEntry dim;
        if (!dims.Lookup(item_id, item_name, dim)) {
            warnings.push_back("缺尺寸: " + item_id + " " + item_name);
            continue;
        }

        for (int i = 0; i < qty; ++i)
            out.emplace_back(item_id, item_name,
                             "ORDER_" + order_id,
                             dim.raw_l, dim.raw_w, dim.raw_h);
    }
    return true;
}
