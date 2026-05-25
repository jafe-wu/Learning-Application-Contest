// PalletAlgorithm.cpp
#include "PalletAlgorithm.h"
#include <algorithm>
#include <cmath>

double PalletSpace::DEAD_ZONE_WIDTH = 77.3;

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
    double best_support = -1.0;
    double best_uniformity = 1e9;

    std::vector<double> candidate_xs;
    candidate_xs.reserve(skyline.size() * 3 + 1);
    candidate_xs.push_back(5.0);

    // 生成边界候选点
    for (const auto& seg : skyline) {
        if (seg.x1 >= 5.0 && seg.x1 + task.totalWidth <= MAX_WIDTH - 5.0) {
            candidate_xs.push_back(seg.x1);
        }
        if (seg.x2 + 5.0 + task.totalWidth <= MAX_WIDTH - 5.0) {
            candidate_xs.push_back(seg.x2 + 5.0);
        }
    }

    // B3: 在大间隙中添加中点候选
    for (size_t k = 0; k + 1 < skyline.size(); ++k) {
        double gap_start = skyline[k].x2;
        double gap_end   = skyline[k+1].x1;
        double gap_width = gap_end - gap_start;
        if (gap_width >= task.totalWidth + 10.0) {
            double mid = gap_start + (gap_width - task.totalWidth) / 2.0;
            if (mid >= 5.0 && mid + task.totalWidth <= MAX_WIDTH - 5.0) {
                candidate_xs.push_back(mid);
            }
        }
    }

    // ── 第一遍扫描：计算当前最低可用水平面 ──
    double min_possible_y = 1e9;
    for (double candidate_x : candidate_xs) {
        double place_start = candidate_x;
        double place_end   = candidate_x + task.totalWidth;
        double cy = 0.0;

        for (const auto& seg : skyline) {
            if (seg.x2 <= place_start || seg.x1 >= place_end) continue;
            if (seg.height > cy) cy = seg.height;
        }

        double check_start_x = candidate_x - 5.0;
        double check_end_x   = candidate_x + task.totalWidth + 5.0;
        for (const auto& seg : skyline) {
            if (seg.x2 <= check_start_x || seg.x1 >= check_end_x) continue;
            if (seg.height > cy) cy = seg.height;
        }

        if (cy + task.maxHeight > MAX_HEIGHT) continue;
        if (cy < min_possible_y) min_possible_y = cy;
    }

    // ── 第二遍扫描：评价候选位置，优先填平同一水平线 ──
    double best_level_score = 1e9;

    for (double candidate_x : candidate_xs) {
        double place_start = candidate_x;
        double place_end   = candidate_x + task.totalWidth;
        double candidate_y = 0.0;

        // 获取放置区域最大高度
        for (const auto& seg : skyline) {
            if (seg.x2 <= place_start || seg.x1 >= place_end) continue;
            if (seg.height > candidate_y) candidate_y = seg.height;
        }

        // 破顶检查
        if (candidate_y + task.maxHeight > MAX_HEIGHT) continue;

        // A1: 虚拟包围盒防碰撞：5mm 扩展区内抬升 candidate_y 以避开障碍
        double check_start_x = candidate_x - 5.0;
        double check_end_x   = candidate_x + task.totalWidth + 5.0;

        for (const auto& seg : skyline) {
            if (seg.x2 <= check_start_x || seg.x1 >= check_end_x) continue;
            if (seg.height > candidate_y) {
                candidate_y = seg.height;
            }
        }

        // 抬升后重新检查越顶
        if (candidate_y + task.maxHeight > MAX_HEIGHT) continue;

        // 悬空稳态判定（20mm 限制）
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

        // 垛型均匀性评价：新段与相邻段的高度差，差值越小垛型越均匀
        double new_seg_height = candidate_y + task.maxHeight;
        double left_adj = 0.0, right_adj = 0.0;
        for (const auto& seg : skyline) {
            if (seg.x2 > place_start - 5.1 && seg.x2 <= place_start + 0.1)
                left_adj = seg.height;
            if (seg.x1 >= place_end - 0.1 && seg.x1 < place_end + 5.1)
                right_adj = seg.height;
        }
        double uniformity_penalty = 0.0;
        if (left_adj > 0.01)
            uniformity_penalty = std::max(uniformity_penalty, std::abs(new_seg_height - left_adj));
        if (right_adj > 0.01)
            uniformity_penalty = std::max(uniformity_penalty, std::abs(new_seg_height - right_adj));

        // 水平约束评分：在最低水平面上的候选者获得强偏好，实现"先平铺后堆垛"
        double level_score = (std::abs(candidate_y - min_possible_y) < 0.01)
            ? -1000.0          // 同水平面 → 强烈优先
            : candidate_y;      // 高于水平面 → 按 Y 比较

        // 五级评价：同水平面优先 → 垛型均匀性 → Y 最小 → 支撑率最大 → X 最小
        bool better = false;
        if (level_score < best_level_score - 0.01) {
            better = true;
        } else if (std::abs(level_score - best_level_score) < 0.01) {
            if (uniformity_penalty < best_uniformity - 0.01) {
                better = true;
            } else if (std::abs(uniformity_penalty - best_uniformity) < 0.01) {
                if (candidate_y < best_y - 0.01) {
                    better = true;
                } else if (std::abs(candidate_y - best_y) < 0.01) {
                    double sr = support_length / task.totalWidth;
                    if (sr > best_support + 0.01) {
                        better = true;
                    } else if (std::abs(sr - best_support) < 0.01 && candidate_x < best_x) {
                        better = true;
                    }
                }
            }
        }
        if (better) {
            best_level_score = level_score;
            best_uniformity = uniformity_penalty;
            best_y = candidate_y;
            best_x = candidate_x;
            best_support = support_length / task.totalWidth;
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