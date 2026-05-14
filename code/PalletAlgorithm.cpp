// PalletAlgorithm.cpp
#include "PalletAlgorithm.h"
#include <algorithm>
#include <cmath>

PalletSpace::PalletSpace() {
    // 初始化地面，高度 0，预留容量避免扩容开销
    skyline.reserve(32); 
    skyline.push_back({0.0, MAX_WIDTH, 0.0});
}

bool PalletSpace::IsEmpty() const {
    if (skyline.size() != 1) return false;
    return std::abs(skyline.front().height) < 0.01;
}

bool PalletSpace::TryInsert(GrabTask& task) {
    double best_x = -1.0;
    double best_y = 1e9;

    std::vector<double> candidate_xs;
    candidate_xs.reserve(skyline.size() * 2 + 1);
    candidate_xs.push_back(5.0);

    // 剪枝优化1：直接在生成候选点时过滤掉明显越界的坐标
    for (const auto& seg : skyline) {
        if (seg.x1 >= 5.0 && seg.x1 + task.totalWidth <= MAX_WIDTH - 5.0) {
            candidate_xs.push_back(seg.x1);
        }
        if (seg.x2 + 5.0 + task.totalWidth <= MAX_WIDTH - 5.0) {
            candidate_xs.push_back(seg.x2 + 5.0);
        }
    }

    for (double candidate_x : candidate_xs) {
        double place_start = candidate_x;
        double place_end   = candidate_x + task.totalWidth;
        double candidate_y = 0.0;
        
        // 剪枝优化2：合并最大高度获取与高度越界检查
        for (const auto& seg : skyline) {
            if (seg.x2 <= place_start || seg.x1 >= place_end) continue;
            if (seg.height > candidate_y) candidate_y = seg.height;
        }

        // 破顶直接跳过
        if (candidate_y + task.maxHeight > MAX_HEIGHT) continue;

        // 3. 虚拟包围盒防碰撞（左右各扩 5mm 确保抓取间隙）
        double check_start_x = candidate_x - 5.0;
        double check_end_x   = candidate_x + task.totalWidth + 5.0;
        bool collision = false;
        
        for (const auto& seg : skyline) {
            if (seg.x2 <= check_start_x || seg.x1 >= check_end_x) continue;
            // 只有当障碍物高度高于我们打算放置的高度时，才算作碰撞
            if (seg.height > candidate_y) {
                collision = true;
                break;
            }
        }
        if (collision) continue;

        // 4. 悬空稳态判定
        double support_length = 0.0;
        for (const auto& seg : skyline) {
            if (std::abs(seg.height - candidate_y) < 0.01) {
                double overlap_start = std::max(place_start, seg.x1);
                double overlap_end   = std::min(place_end,   seg.x2);
                if (overlap_start < overlap_end) {
                    support_length += (overlap_end - overlap_start);
                }
            }
        }
        if (task.totalWidth - support_length > 20.0) continue;

        // 5. 评价函数：优先 Y 最小 (底层优先)，平局挑 X 最小 (左侧优先)
        if (candidate_y < best_y - 0.01 ||
            (std::abs(candidate_y - best_y) < 0.01 && candidate_x < best_x)) {
            best_y = candidate_y;
            best_x = candidate_x;
        }
    }

    if (best_x < 0.0) return false;

    task.x = best_x;
    task.y = best_y;
    UpdateSkyline(task);
    FlattenDeadZones();
    return true;
}

void PalletSpace::UpdateSkyline(const GrabTask& task) {
    double start_x    = task.x;
    double end_x      = task.x + task.totalWidth;
    double new_height = task.y + task.maxHeight;

    std::vector<LineSegment> new_skyline;
    new_skyline.reserve(skyline.size() + 2);

    bool new_added = false;

    // 性能爆发点：O(N) 线性合并，彻底移除 std::sort
    for (const auto& seg : skyline) {
        if (seg.x2 <= start_x) {
            new_skyline.push_back(seg);
        } else if (seg.x1 >= end_x) {
            if (!new_added) {
                new_skyline.push_back({start_x, end_x, new_height});
                new_added = true;
            }
            new_skyline.push_back(seg);
        } else {
            if (seg.x1 < start_x) {
                new_skyline.push_back({seg.x1, start_x, seg.height});
            }
            if (!new_added) {
                new_skyline.push_back({start_x, end_x, new_height});
                new_added = true;
            }
            if (seg.x2 > end_x) {
                new_skyline.push_back({end_x, seg.x2, seg.height});
            }
        }
    }
    
    if (!new_added) {
        new_skyline.push_back({start_x, end_x, new_height});
    }

    // 清空原数组，执行等高线段合并
    skyline.clear();
    for (const auto& seg : new_skyline) {
        if (!skyline.empty() && 
            std::abs(skyline.back().height - seg.height) < 0.01 && 
            std::abs(skyline.back().x2 - seg.x1) < 0.01) {
            skyline.back().x2 = seg.x2;
        } else {
            skyline.push_back(seg);
        }
    }
}

void PalletSpace::FlattenDeadZones() {
    bool changed = true;
    while (changed) {
        changed = false;
        // 抹平阶段
        for (size_t i = 0; i < skyline.size(); ++i) {
            if ((skyline[i].x2 - skyline[i].x1) >= DEAD_ZONE_WIDTH) continue;

            bool prev_higher = (i > 0 && skyline[i-1].height > skyline[i].height);
            bool next_higher = (i + 1 < skyline.size() && skyline[i+1].height > skyline[i].height);
            
            if (!prev_higher && !next_higher) continue;

            double lifted = skyline[i].height;
            if (prev_higher) lifted = std::max(lifted, skyline[i-1].height);
            if (next_higher) lifted = std::max(lifted, skyline[i+1].height);
            
            skyline[i].height = lifted;
            changed = true;
        }

        // 合并阶段
        if (changed) {
            std::vector<LineSegment> merged;
            merged.reserve(skyline.size());
            for (const auto& seg : skyline) {
                if (!merged.empty() && 
                    std::abs(merged.back().height - seg.height) < 0.01 && 
                    std::abs(merged.back().x2 - seg.x1) < 0.01) {
                    merged.back().x2 = seg.x2;
                } else {
                    merged.push_back(seg);
                }
            }
            skyline = std::move(merged);
        }
    }
}