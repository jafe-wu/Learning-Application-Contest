// PalletAlgorithm.cpp
#include "PalletAlgorithm.h"
#include <algorithm>
#include <cmath>

double PalletSpace::DEAD_ZONE_WIDTH = 77.3;

PalletSpace::PalletSpace()
    : columnHeights(COL_COUNT, 0.0) {
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

    // ── 计算当前天际线的最低高度（基准层高），用于层偏差惩罚 ──
    double min_skyline_h = skyline[0].height;
    for (const auto& seg : skyline)
        if (seg.height < min_skyline_h) min_skyline_h = seg.height;

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

        // 悬空稳态判定（20mm 限制）：基于真实货物而非天际线，避免死区抹平产生虚假支撑
        double support_length = 0.0;
        double supported_left_edge = place_end;
        double supported_right_edge = place_start;

        if (candidate_y < 0.01) {
            // 放置在地面上，全宽支撑
            support_length = task.totalWidth;
            supported_left_edge = place_start;
            supported_right_edge = place_end;
        } else {
            for (const auto& item : placedItems) {
                if (item.x + item.width <= place_start || item.x >= place_end) continue;
                if (std::abs(item.y + item.height - candidate_y) > 0.01) continue;
                double overlap_start = std::max(place_start, item.x);
                double overlap_end   = std::min(place_end, item.x + item.width);
                if (overlap_start < overlap_end) {
                    support_length += (overlap_end - overlap_start);
                    if (overlap_start < supported_left_edge)  supported_left_edge  = overlap_start;
                    if (overlap_end   > supported_right_edge) supported_right_edge = overlap_end;
                }
            }
        }
        double left_unsupported  = supported_left_edge  - place_start;
        double right_unsupported = place_end - supported_right_edge;
        if (left_unsupported > 20.0 || right_unsupported > 20.0) continue;

        // ── 综合评价 ──
        double new_seg_height = candidate_y + task.maxHeight;
        double left_adj = 0.0, right_adj = 0.0;
        for (const auto& seg : skyline) {
            if (seg.x2 > place_start - 5.1 && seg.x2 <= place_start + 0.1)
                left_adj = seg.height;
            if (seg.x1 >= place_end - 0.1 && seg.x1 < place_end + 5.1)
                right_adj = seg.height;
        }
        double total_penalty = 0.0;

        // 1) 邻段高度差惩罚：仅在邻段高于候选底面时扣分
        //    邻段高于底面 = 并排放置（同一水平面不同高度），需要惩罚
        //    邻段等于底面 = 垂直堆垛（同一水平面同高度），高度差正常不扣分
        if (left_adj > 0.01 && left_adj > candidate_y + 0.01)
            total_penalty += std::abs(new_seg_height - left_adj);
        if (right_adj > 0.01 && right_adj > candidate_y + 0.01)
            total_penalty += std::abs(new_seg_height - right_adj);

        // 2) 层偏差惩罚：离开最低天际线越远扣分越重
        //    确保"先填平再堆垛"，但在该层会产生严重不平时允许适当偏离
        double layer_dev = candidate_y - min_skyline_h;
        if (layer_dev > 20.0)
            total_penalty += (layer_dev - 20.0) * 3.0;

        // 3) 列均匀性：覆盖列的累计堆叠高度，惩罚往已堆高处继续堆
        double total_col_h = 0.0;
        int col_count = 0;
        for (int ci = 0; ci < (int)columnHeights.size(); ++ci) {
            double col_start = ci * COL_WIDTH;
            double col_end   = (ci + 1) * COL_WIDTH;
            if (col_start < place_end && col_end > place_start) {
                total_col_h += columnHeights[ci];
                col_count++;
            }
        }
        double avg_col_h = (col_count > 0) ? total_col_h / col_count : 0.0;
        total_penalty += avg_col_h * 0.5;

        // 三级评价：综合评分（越低越好）→ Y 最小 → 支撑率最大 → X 最小
        bool better = false;
        if (total_penalty < best_uniformity - 0.01) {
            better = true;
        } else if (std::abs(total_penalty - best_uniformity) < 0.01) {
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
        if (better) {
            best_uniformity = total_penalty;
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

    UpdateColumnHeights(task);
    placedItems.push_back({task.x, task.y, task.totalWidth, task.maxHeight});
}

void PalletSpace::UpdateColumnHeights(const GrabTask& task) {
    double start_x = task.x;
    double end_x   = task.x + task.totalWidth;
    for (int ci = 0; ci < (int)columnHeights.size(); ++ci) {
        double col_start = ci * COL_WIDTH;
        double col_end   = (ci + 1) * COL_WIDTH;
        if (col_start < end_x && col_end > start_x) {
            columnHeights[ci] += task.maxHeight;
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