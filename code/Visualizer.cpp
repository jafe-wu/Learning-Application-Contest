// Visualizer.cpp
#include "Visualizer.h"
#include <graphics.h>
#include <conio.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <windows.h>

static const double SCALE    = 3.0;
static const int    X_OFFSET = 50;
static const int    Y_OFFSET = 80;
static const int    PALLET_W = 440;  // mm
static const int    PALLET_H = 140;  // mm

// UTF-8 std::string → 宽字符 std::wstring (用于 EasyX UNICODE 输出)
static std::wstring s2w(const std::string& s) {
    if (s.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring ws(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &ws[0], len);
    return ws;
}

static std::wstring fmtPct(double v) {
    std::wstringstream ss;
    ss << std::fixed << std::setprecision(2) << v << L"%";
    return ss.str();
}

static void DrawPallet(const PalletSnapshot& snap,
                       const PalletizerController& ctrl) {
    cleardevice();

    // ── 顶部 KPI 看板 ──────────────────────────────────
    settextcolor(BLACK);
    settextstyle(18, 0, _T("Consolas"));

    std::wstringstream header;
    header << L"当前垛号: " << snap.palletId
           << L"   订单: " << s2w(snap.orderId)
           << L"   抓取次数(本垛): " << (int)snap.tasks.size()
           << L"   利用率: " << fmtPct(snap.utilizationPct);
    outtextxy(X_OFFSET, 15, header.str().c_str());

    std::wstringstream kpi;
    kpi << L"[KPI]  总托盘: " << ctrl.TotalPallets()
        << L"   总抓取次数: " << ctrl.TotalGrabs()
        << L"   运行耗时: " << ctrl.RunTimeMicros() << L" us";
    outtextxy(X_OFFSET, 40, kpi.str().c_str());

    // ── 托盘物理外框 ──────────────────────────────────
    setlinecolor(BLACK);
    setlinestyle(PS_SOLID, 2);
    rectangle(X_OFFSET, Y_OFFSET,
              X_OFFSET + PALLET_W * (int)SCALE,
              Y_OFFSET + PALLET_H * (int)SCALE);

    // ── 5mm 禁飞区红线 ────────────────────────────────
    setlinecolor(RED);
    setlinestyle(PS_DASH, 1);
    line(X_OFFSET + 5   * (int)SCALE, Y_OFFSET,
         X_OFFSET + 5   * (int)SCALE, Y_OFFSET + PALLET_H * (int)SCALE);
    line(X_OFFSET + 435 * (int)SCALE, Y_OFFSET,
         X_OFFSET + 435 * (int)SCALE, Y_OFFSET + PALLET_H * (int)SCALE);
    setlinestyle(PS_SOLID, 1);

    // ── 绘制每次抓取 ──────────────────────────────────
    for (size_t i = 0; i < snap.tasks.size(); ++i) {
        const auto& task = snap.tasks[i];

        int screen_x = X_OFFSET + static_cast<int>(task.x * SCALE);
        int screen_y = Y_OFFSET + static_cast<int>(
            (PALLET_H - task.y - task.maxHeight) * SCALE);
        int rect_w = static_cast<int>(task.totalWidth * SCALE);
        int rect_h = static_cast<int>(task.maxHeight  * SCALE);

        // 双取 vs 单取用不同色调
        if (task.grabCount == 2) {
            setfillcolor(HSVtoRGB(static_cast<float>((i * 47) % 360), 0.55f, 0.95f));
        } else {
            setfillcolor(HSVtoRGB(static_cast<float>((i * 47) % 360), 0.75f, 0.90f));
        }
        fillrectangle(screen_x, screen_y, screen_x + rect_w, screen_y + rect_h);

        // 黑色边框
        setlinecolor(BLACK);
        rectangle(screen_x, screen_y, screen_x + rect_w, screen_y + rect_h);

        // 若是双取，在内部画一条分隔线标示两条烟
        if (task.grabCount == 2 && task.items.size() == 2) {
            int sep_x = screen_x + static_cast<int>(task.items[0].width * SCALE);
            setlinecolor(RGB(80, 80, 80));
            setlinestyle(PS_DOT, 1);
            line(sep_x, screen_y, sep_x, screen_y + rect_h);
            setlinestyle(PS_SOLID, 1);
        }

        // 中心数字：抓取顺序号（本托盘内）
        std::wstring seq_str = std::to_wstring((long long)(i + 1));
        settextcolor(BLACK);
        settextstyle(18, 0, _T("Arial"));
        int text_w = textwidth(seq_str.c_str());
        int text_h = textheight(seq_str.c_str());
        outtextxy(screen_x + (rect_w - text_w) / 2,
                  screen_y + (rect_h - text_h) / 2,
                  seq_str.c_str());

        // 在每个色块底部标注烟的名称
        settextcolor(RGB(40, 40, 40));
        settextstyle(11, 0, _T("SimSun"));
        if (task.grabCount == 2 && task.items.size() == 2) {
            // 双取：左右两半分别标注各自名称
            int sep_px = static_cast<int>(task.items[0].width * SCALE);
            // 左半
            std::wstring nameL = s2w(task.items[0].name);
            int twL = textwidth(nameL.c_str());
            int availableL = sep_px - 4;
            if (twL > availableL && availableL > 0) {
                // 超宽则逐字截断
                while (nameL.size() > 1 && textwidth(nameL.substr(0, nameL.size()-1).c_str()) > availableL)
                    nameL.pop_back();
            }
            outtextxy(screen_x + 2, screen_y + rect_h - 14, nameL.c_str());
            // 右半
            std::wstring nameR = s2w(task.items[1].name);
            int twR = textwidth(nameR.c_str());
            int availableR = rect_w - sep_px - 4;
            if (twR > availableR && availableR > 0) {
                while (nameR.size() > 1 && textwidth(nameR.substr(0, nameR.size()-1).c_str()) > availableR)
                    nameR.pop_back();
            }
            outtextxy(screen_x + sep_px + 2, screen_y + rect_h - 14, nameR.c_str());
        } else {
            // 单取：整块标注名称
            std::wstring nameStr = s2w(task.items[0].name);
            int tw = textwidth(nameStr.c_str());
            int available = rect_w - 4;
            if (tw > available && available > 0) {
                while (nameStr.size() > 1 && textwidth(nameStr.substr(0, nameStr.size()-1).c_str()) > available)
                    nameStr.pop_back();
            }
            outtextxy(screen_x + 2, screen_y + rect_h - 14, nameStr.c_str());
        }
    }

    // ── 底部图例 ──────────────────────────────────────
    settextcolor(RGB(80, 80, 80));
    settextstyle(14, 0, _T("Consolas"));
    int legendY = Y_OFFSET + PALLET_H * (int)SCALE + 20;
    outtextxy(X_OFFSET, legendY,
              _T("红色虚线 = 5mm 禁飞区   浅色块 = 双取合并   深色块 = 单取   块内数字 = 本托盘抓取顺序"));
    outtextxy(X_OFFSET, legendY + 20,
              _T("操作: 输入其它垛号切换查看，输入 0 退出"));
}

void RenderInteractive(const PalletizerController& controller) {
    const auto& pallets = controller.GetPallets();
    if (pallets.empty()) {
        initgraph(1420, 620, EX_SHOWCONSOLE);
        setbkcolor(RGB(255, 255, 220));
        cleardevice();
        settextcolor(BLACK);
        settextstyle(20, 0, _T("Consolas"));
        outtextxy(40, 40, _T("没有任何托盘数据。"));
        _getch();
        closegraph();
        return;
    }

    initgraph(1420, 620, EX_SHOWCONSOLE);
    setbkcolor(RGB(255, 255, 220));

    int currentId = pallets.front().palletId;

    while (true) {
        // 找到要渲染的托盘
        const PalletSnapshot* target = nullptr;
        for (const auto& p : pallets) {
            if (p.palletId == currentId) { target = &p; break; }
        }
        if (!target) target = &pallets.front();

        BeginBatchDraw();
        DrawPallet(*target, controller);
        EndBatchDraw();

        // InputBox 接收下一个垛号
        TCHAR buf[32] = {0};
        std::wstringstream prompt;
        prompt << L"可用垛号 1~" << controller.TotalPallets()
               << L"，输入 0 退出";
        if (!InputBox(buf, 32, prompt.str().c_str(), _T("切换垛型"),
                      _T(""), 0, 0, false)) {
            break;  // 用户取消
        }
        int next = _wtoi(buf);
        if (next == 0) break;
        if (next >= 1 && next <= controller.TotalPallets()) {
            currentId = next;
        }
    }

    closegraph();
}
