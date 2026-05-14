// DimDB.cpp
#include "DimDB.h"
#include <fstream>
#include <sstream>

std::string DimDB::trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n\xEF\xBB\xBF");
    size_t b = s.find_last_not_of(" \t\r\n");
    return (a == std::string::npos) ? "" : s.substr(a, b - a + 1);
}

bool DimDB::LoadFromCSV(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return false;

    std::string line;
    std::getline(f, line); // skip header (may contain UTF-8 BOM)

    while (std::getline(f, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        char sep = (line.find('\t') != std::string::npos) ? '\t' : ',';
        std::stringstream ss(line);
        std::string id, name, sl, sw, sh;
        std::getline(ss, id,   sep);
        std::getline(ss, name, sep);
        std::getline(ss, sl,   sep);
        std::getline(ss, sw,   sep);
        std::getline(ss, sh,   sep);

        id = trim(id); name = trim(name);
        sl = trim(sl); sw = trim(sw); sh = trim(sh);

        try {
            int l = std::stoi(sl), w = std::stoi(sw), h = std::stoi(sh);
            if (l > 0 && w > 0 && h > 0) {
                DimEntry e{l, w, h};
                if (!id.empty())   byId_[id]     = e;
                if (!name.empty()) byName_[name] = e;
            }
        } catch (...) {}
    }
    return !byId_.empty();
}

bool DimDB::Lookup(const std::string& id, const std::string& name, DimEntry& out) const {
    auto it = byId_.find(id);
    if (it != byId_.end()) { out = it->second; return true; }
    auto it2 = byName_.find(name);
    if (it2 != byName_.end()) { out = it2->second; return true; }
    return false;
}
