#include "pch.h" 
// ==========================================
// 必须放在 Windows.h 之前，确保 Unicode 版本 API 和 NOMINMAX
#define UNICODE
#define _UNICODE
#define NOMINMAX
// ==========================================
#include <Windows.h>
// ==========================================
// 极其关键的修复：解决 VS 默认精简模式导致的 interface 宏丢失问题
#ifndef interface
#define interface struct
#endif
// ==========================================
#include "PluginInterface.h"
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <algorithm>
#include <limits>
// ==========================================
// 引入 Windows 自带的网络库和时间库
#include <wininet.h>
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#include <chrono>
// ==========================================

// ==========================================
// 配置文件：proxy_latency_urls.txt
// 放在 DLL 同级目录下，每行一个测速 URL
// 如果文件不存在，使用下方默认 URL
// 提示：请使用走代理节点而非直连的站点，否则测到的是本地延迟
// ==========================================
static const wchar_t* DEFAULT_URLS[] = {
    L"http://cp.cloudflare.com/generate_204",
};
static const int DEFAULT_URL_COUNT = sizeof(DEFAULT_URLS) / sizeof(DEFAULT_URLS[0]);

// ==========================================
// 1. 数据显示项类 (负责在任务栏上显示数据)
// ==========================================
class CLatencyItem : public IPluginItem
{
private:
    std::wstring m_item_name = L"代理延迟";
    std::wstring m_item_value = L"检测中...";
    std::atomic<bool> m_is_updating{ false };
    std::vector<std::wstring> m_urls;

    // 颜色状态
    unsigned int m_label_color = 0xCCCCCC;   // 标签颜色（从主程序获取）
    unsigned int m_latency_color = 0x00CC00;  // 延迟数值颜色（自动计算）
    long long m_last_latency = -1;            // 上次测到的延迟，-1 表示未测到
    std::chrono::steady_clock::time_point m_last_measure_time; // 上次测速时间
    const int m_measure_interval_ms = 3000;   // 测速间隔（毫秒）

    // 从配置文件加载测速 URL
    void LoadUrls() {
        m_urls.clear();

        // 获取 DLL 所在目录
        wchar_t dllPath[MAX_PATH];
        GetModuleFileNameW(NULL, dllPath, MAX_PATH);
        std::wstring configPath = dllPath;
        auto pos = configPath.rfind(L'\\');
        if (pos != std::wstring::npos) {
            configPath = configPath.substr(0, pos + 1);
        }
        configPath += L"proxy_latency_urls.txt";

        // 尝试读取配置文件
        FILE* file = NULL;
        if (_wfopen_s(&file, configPath.c_str(), L"r,ccs=UTF-8") == 0 && file) {
            wchar_t line[1024];
            while (fgetws(line, 1024, file)) {
                // 去除末尾换行符
                size_t len = wcslen(line);
                while (len > 0 && (line[len - 1] == L'\n' || line[len - 1] == L'\r')) {
                    line[--len] = L'\0';
                }
                // 跳过空行和注释行
                if (len > 0 && line[0] != L'#') {
                    m_urls.push_back(line);
                }
            }
            fclose(file);
        }

        // 如果配置文件不存在或为空，使用默认 URL
        if (m_urls.empty()) {
            for (int i = 0; i < DEFAULT_URL_COUNT; i++) {
                m_urls.push_back(DEFAULT_URLS[i]);
            }
        }
    }

    // 测量到单个站点的延迟 (毫秒)，失败返回 -1
    long long MeasureLatency(HINTERNET hInternet, const wchar_t* url) {
        auto start_time = std::chrono::high_resolution_clock::now();
        HINTERNET hUrl = InternetOpenUrlW(hInternet, url, NULL, 0, INTERNET_FLAG_RELOAD, 0);
        if (hUrl) {
            auto end_time = std::chrono::high_resolution_clock::now();
            InternetCloseHandle(hUrl);
            return std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        }
        return -1;
    }

    // 根据延迟值返回颜色 (COLORREF: 0x00BBGGRR)
    unsigned int GetLatencyColor(long long latency) const {
        if (latency < 0) return 0x888888;    // 错误/超时 → 灰色
        if (latency < 50) return 0x00CC00;   // 优秀 → 绿色
        if (latency < 100) return 0x66CC00;  // 良好 → 浅绿
        if (latency < 200) return 0xDDCC00;  // 一般 → 黄色
        if (latency < 500) return 0xDD6600;  // 较慢 → 橙色
        return 0xDD3333;                      // 很慢 → 红色
    }

public:
    CLatencyItem() {
        LoadUrls();
    }

    virtual const wchar_t* GetItemName() const override { return m_item_name.c_str(); }
    virtual const wchar_t* GetItemId() const override { return L"proxy_latency_item_01"; }
    virtual const wchar_t* GetItemLableText() const override { return L"延迟: "; }
    virtual const wchar_t* GetItemValueText() const override { return m_item_value.c_str(); }
    virtual const wchar_t* GetItemValueSampleText() const override { return L"999 ms"; }

    // ==========================================
    // 自定义绘制 — 实现颜色数字
    // ==========================================
    virtual bool IsCustomDraw() const override { return true; }

    // 固定宽度 (96 DPI 下)，主程序会根据 DPI 自动缩放
    virtual int GetItemWidth() const override { return 90; }

    // 实际宽度
    virtual int GetItemWidthEx(void* hDC) const override {
        return 90;
    }

    // 自定义绘制 — 旧 API（使用 HDC）
    virtual void DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode) override {
        HDC dc = (HDC)hDC;
        int font_size = (std::max)(14, h - 1);

        // 正常字体（标签）
        HFONT hFont = CreateFontW(font_size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        // 粗体字体（数值）
        HFONT hBoldFont = CreateFontW(font_size, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        SetBkMode(dc, TRANSPARENT);

        int label_len = (int)wcslen(GetItemLableText());
        SIZE label_size{};
        SelectObject(dc, hFont);
        GetTextExtentPoint32W(dc, GetItemLableText(), label_len, &label_size);

        // 绘制标签（正常）
        SetTextColor(dc, m_label_color);
        SelectObject(dc, hFont);
        RECT label_rc = { x, y, x + w, y + h };
        DrawTextW(dc, GetItemLableText(), -1, &label_rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

        // 绘制数值（粗体 + 延迟颜色）
        SetTextColor(dc, m_latency_color);
        SelectObject(dc, hBoldFont);
        RECT value_rc = { x + label_size.cx, y, x + w, y + h };
        DrawTextW(dc, m_item_value.c_str(), -1, &value_rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

        SelectObject(dc, hFont);
        DeleteObject(hFont);
        DeleteObject(hBoldFont);
    }

    // 自定义绘制 — 新 API（使用 IPluginDrawer）
    virtual bool DrawItemEx(IPluginDrawer* pDrawer, int x, int y, int w, int h, bool dark_mode) override {
        int font_size = (std::max)(14, h - 1);
        const wchar_t* font_name = L"Segoe UI";

        const wchar_t* label = GetItemLableText();
        int label_w = 0, label_h = 0;
        pDrawer->GetTextExtent(label, font_name, font_size, false, false, &label_w, &label_h);
        pDrawer->DrawText(x, y, label_w, h, label, font_name, font_size, false, false, m_label_color, 0);

        int value_x = x + label_w;
        int value_w = w - label_w;
        pDrawer->DrawText(value_x, y, value_w, h, m_item_value.c_str(), font_name, font_size, true, false, m_latency_color, 0);

        return true;
    }

    // 接收主程序传过来的标签颜色
    void SetLabelColor(unsigned int color) { m_label_color = color; }

    // 多站点测速逻辑 — 取中位数，避免直连站点拉低结果
    void UpdateLatencyAsync() {
        // 5 秒间隔限制
        auto now = std::chrono::steady_clock::now();
        if (now - m_last_measure_time < std::chrono::milliseconds(m_measure_interval_ms)) {
            return;
        }
        m_last_measure_time = now;

        if (m_is_updating.exchange(true)) return;

        std::thread([this]() {
            HINTERNET hInternet = InternetOpenW(L"TM_Plugin", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
            if (hInternet) {
                std::vector<long long> latencies;
                int total = static_cast<int>(m_urls.size());

                for (int i = 0; i < total; i++) {
                    long long delay = MeasureLatency(hInternet, m_urls[i].c_str());
                    if (delay >= 0) {
                        latencies.push_back(delay);
                    }
                }

                if (!latencies.empty()) {
                    // 取中位数 (median) 而非最小值，避免直连站点的干扰
                    std::sort(latencies.begin(), latencies.end());
                    long long median = latencies[latencies.size() / 2];
                    m_last_latency = median;
                    m_latency_color = GetLatencyColor(median);
                    m_item_value = std::to_wstring(median) + L" ms";
                } else {
                    m_last_latency = -1;
                    m_latency_color = GetLatencyColor(-1);
                    m_item_value = L"超时";
                }
                InternetCloseHandle(hInternet);
            } else {
                m_last_latency = -1;
                m_latency_color = GetLatencyColor(-1);
                m_item_value = L"网络错误";
            }
            m_is_updating = false;
        }).detach();
    }
};

// ==========================================
// 2. 主插件类 (继承真实的 ITMPlugin 基类)
// ==========================================
class CLatencyPlugin : public ITMPlugin
{
private:
    CLatencyItem m_latency_item;
public:
    virtual IPluginItem* GetItem(int index) override {
        if (index == 0) return &m_latency_item;
        return nullptr;
    }
    virtual void DataRequired() override {
        m_latency_item.UpdateLatencyAsync();
    }
    virtual const wchar_t* GetInfo(PluginInfoIndex index) override {
        switch (index)
        {
        case TMI_NAME: return L"代理节点延迟监控";
        case TMI_DESCRIPTION: return L"多站点测速 (中位数)，支持自定义测速 URL，颜色显示";
        case TMI_AUTHOR: return L"YourName";
        case TMI_COPYRIGHT: return L"Copyright (C) 2026";
        case TMI_VERSION: return L"1.0.3";
        case TMI_URL: return L"";
        default: return L"";
        }
    }
    // 接收主程序传递的颜色信息
    virtual void OnExtenedInfo(ExtendedInfoIndex index, const wchar_t* data) override {
        if (index == EI_LABEL_TEXT_COLOR && data) {
            unsigned int color = std::wcstoul(data, nullptr, 16);
            m_latency_item.SetLabelColor(color);
        }
    }
};

// ==========================================
// 3. 导出插件实例 (官方要求的规范签名)
// ==========================================
extern "C" __declspec(dllexport) ITMPlugin* TMPluginGetInstance()
{
    static CLatencyPlugin plugin;
    return &plugin;
}