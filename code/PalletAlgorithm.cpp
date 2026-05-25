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
    double best_post_range = 1e9;
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

    // B4: 在足够宽的段内部添加候选（居中 + 左右留余量），避免只能在段边界放置
    for (const auto& seg : skyline) {
        double seg_w = seg.x2 - seg.x1;
        if (seg_w < task.totalWidth + 10.0) continue;
        double center = seg.x1 + (seg_w - task.totalWidth) / 2.0;
        if (center >= 5.0 && center + task.totalWidth <= MAX_WIDTH - 5.0)
            candidate_xs.push_back(center);
        double left_in = seg.x1 + 5.0;
        if (left_in >= 5.0 && left_in + task.totalWidth <= MAX_WIDTH - 5.0)
            candidate_xs.push_back(left_in);
        double right_in = seg.x2 - 5.0 - task.totalWidth;
        if (right_in >= 5.0 && right_in + task.totalWidth <= MAX_WIDTH - 5.0)
            candidate_xs.push_back(right_in);
    }

    // ── 计算当前天际线的最低和最高高度 ──
    double min_skyline_h = skyline[0].height;
    double max_skyline_h = skyline[0].height;
    for (const auto& seg : skyline) {
        if (seg.height < min_skyline_h) min_skyline_h = seg.height;
        if (seg.height > max_skyline_h) max_skyline_h = seg.height;
    }

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

        // A1: 虚拟包围盒防碰撞：5mm 扩展区内检测，仅当邻段与货物垂直范围有交集才抬升
        double check_start_x = candidate_x - 5.0;
        double check_end_x   = candidate_x + task.totalWidth + 5.0;

        for (const auto& seg : skyline) {
            if (seg.x2 <= check_start_x || seg.x1 >= check_end_x) continue;
            // 仅当货物顶部会伸入邻段高度范围时才需抬升；货物完全在邻段下方则不抬升
            if (seg.height > candidate_y && seg.height < candidate_y + task.maxHeight) {
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
        if (task.totalWidth - support_length > 20.0) continue;

        // ── 综合评价：以放置后顶层高度差最小为首要目标 ──
        double new_seg_height = candidate_y + task.maxHeight;

        // 计算放置后天际线的高度范围（max - min），越小顶面越平整
        double post_min = new_seg_height;
        double post_max = new_seg_height;
        for (const auto& seg : skyline) {
            bool remains = (seg.x2 <= place_start || seg.x1 >= place_end)
                        || (seg.x1 < place_start || seg.x2 > place_end);
            if (remains) {
                if (seg.height < post_min) post_min = seg.height;
                if (seg.height > post_max) post_max = seg.height;
            }
        }
        double post_range = post_max - post_min;

        // 列均匀性辅助：覆盖列的累计堆叠高度
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

        // 评价：顶层平整度（硬优先）→ Y 最低 → 列均匀 → 支撑率 → X 最小
        bool better = false;
        if (post_range < best_post_range - 0.01) {
            better = true;
        } else if (std::abs(post_range - best_post_range) < 0.01) {
            if (candidate_y < best_y - 0.01) {
                better = true;
            } else if (std::abs(candidate_y - best_y) < 0.01) {
                if (avg_col_h < best_uniformity - 0.01) {
                    better = true;
                } else if (std::abs(avg_col_h - best_uniformity) < 0.01) {
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
            best_post_range = post_range;
            best_uniformity = avg_col_h;
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