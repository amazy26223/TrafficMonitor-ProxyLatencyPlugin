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
#include <fstream>
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
// 设置文件名
// ==========================================
static const wchar_t* SETTINGS_FILE = L"proxy_latency_settings.ini";

// 获取 DLL 目录
static std::wstring GetDllDir() {
    wchar_t dllPath[MAX_PATH];
    GetModuleFileNameW(NULL, dllPath, MAX_PATH);
    std::wstring path = dllPath;
    auto pos = path.rfind(L'\\');
    if (pos != std::wstring::npos)
        path = path.substr(0, pos + 1);
    return path;
}

// 默认设置
static const int DEFAULT_FONT_SIZE = 18;
static const int DEFAULT_INTERVAL_MS = 3000;

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

    // 可配置设置
    int m_font_size = DEFAULT_FONT_SIZE;
    int m_measure_interval_ms = DEFAULT_INTERVAL_MS;

    // 从配置文件加载测速 URL
    void LoadUrls() {
        m_urls.clear();
        std::wstring configPath = GetDllDir() + L"proxy_latency_urls.txt";

        FILE* file = NULL;
        if (_wfopen_s(&file, configPath.c_str(), L"r,ccs=UTF-8") == 0 && file) {
            wchar_t line[1024];
            while (fgetws(line, 1024, file)) {
                size_t len = wcslen(line);
                while (len > 0 && (line[len - 1] == L'\n' || line[len - 1] == L'\r'))
                    line[--len] = L'\0';
                if (len > 0 && line[0] != L'#')
                    m_urls.push_back(line);
            }
            fclose(file);
        }

        if (m_urls.empty()) {
            for (int i = 0; i < DEFAULT_URL_COUNT; i++)
                m_urls.push_back(DEFAULT_URLS[i]);
        }
    }

    // 加载设置
    void LoadSettings() {
        std::wstring path = GetDllDir() + SETTINGS_FILE;
        FILE* file = NULL;
        if (_wfopen_s(&file, path.c_str(), L"r") == 0 && file) {
            int val;
            if (fgetws(m_item_value.data(), 0, file) == NULL) { /* ignore */ }
            rewind(file);
            if (fwscanf_s(file, L"font_size=%d\n", &val) == 1)
                m_font_size = (std::max)(10, (std::min)(val, 48));
            if (fwscanf_s(file, L"interval_ms=%d\n", &val) == 1)
                m_measure_interval_ms = (std::max)(1000, (std::min)(val, 30000));
            fclose(file);
        }
    }

    void SaveSettings() {
        std::wstring path = GetDllDir() + SETTINGS_FILE;
        FILE* file = NULL;
        if (_wfopen_s(&file, path.c_str(), L"w") == 0 && file) {
            fwprintf_s(file, L"font_size=%d\n", m_font_size);
            fwprintf_s(file, L"interval_ms=%d\n", m_measure_interval_ms);
            fclose(file);
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

    // 根据延迟值返回颜色 (0x00BBGGRR)
    unsigned int GetLatencyColor(long long latency) const {
        if (latency < 0) return 0x888888;       // 超时/错误 → 灰色
        if (latency >= 5000) return 0x3333DD;   // 5000ms+ → 红色
        // 0~5000ms: 绿色渐变，从 0x00B400 到 0x003200
        int g = 180 - static_cast<int>(latency * 130 / 5000);
        return static_cast<unsigned int>(g << 8);
    }

public:
    CLatencyItem() {
        LoadUrls();
        LoadSettings();
    }

    // 设置访问接口
    int GetFontSize() const { return m_font_size; }
    int GetIntervalMs() const { return m_measure_interval_ms; }
    void SetFontSize(int s) { m_font_size = (std::max)(10, (std::min)(s, 48)); SaveSettings(); }
    void SetIntervalMs(int ms) { m_measure_interval_ms = (std::max)(1000, (std::min)(ms, 30000)); SaveSettings(); }

    virtual const wchar_t* GetItemName() const override { return m_item_name.c_str(); }
    virtual const wchar_t* GetItemId() const override { return L"proxy_latency_item_01"; }
    virtual const wchar_t* GetItemLableText() const override { return L""; }
    virtual const wchar_t* GetItemValueText() const override { return m_item_value.c_str(); }
    virtual const wchar_t* GetItemValueSampleText() const override { return L"999 ms"; }

    // ==========================================
    // 自定义绘制 — 实现颜色数字
    // ==========================================
    virtual bool IsCustomDraw() const override { return true; }

    virtual int GetItemWidth() const override { return 65; }

    virtual int GetItemWidthEx(void* hDC) const override {
        return 65;
    }

    // 自定义绘制 — 旧 API（使用 HDC）
    virtual void DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode) override {
        HDC dc = (HDC)hDC;
        int font_size = (std::max)(m_font_size, h - 1);

        HFONT hFont = CreateFontW(font_size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        HFONT hBoldFont = CreateFontW(font_size, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        SetBkMode(dc, TRANSPARENT);

        int label_len = (int)wcslen(GetItemLableText());
        SIZE label_size{};
        SelectObject(dc, hFont);
        GetTextExtentPoint32W(dc, GetItemLableText(), label_len, &label_size);

        SetTextColor(dc, m_label_color);
        SelectObject(dc, hFont);
        RECT label_rc = { x, y, x + w, y + h };
        DrawTextW(dc, GetItemLableText(), -1, &label_rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

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
        int font_size = (std::max)(m_font_size, h - 1);
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

    // 多站点测速逻辑
    void UpdateLatencyAsync() {
        auto now = std::chrono::steady_clock::now();
        if (now - m_last_measure_time < std::chrono::milliseconds(m_measure_interval_ms))
            return;
        m_last_measure_time = now;

        if (m_is_updating.exchange(true)) return;

        std::thread([this]() {
            HINTERNET hInternet = InternetOpenW(L"TM_Plugin", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
            if (hInternet) {
                std::vector<long long> latencies;
                int total = static_cast<int>(m_urls.size());

                for (int i = 0; i < total; i++) {
                    long long delay = MeasureLatency(hInternet, m_urls[i].c_str());
                    if (delay >= 0)
                        latencies.push_back(delay);
                }

                if (!latencies.empty()) {
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
// 2. 设置对话框
// ==========================================
// 控件 ID
#define IDC_FONT_SIZE 1001
#define IDC_INTERVAL  1002

static INT_PTR CALLBACK SettingsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    static CLatencyItem* pItem = nullptr;

    switch (msg) {
    case WM_INITDIALOG: {
        pItem = reinterpret_cast<CLatencyItem*>(lParam);
        // 设置字体大小
        wchar_t buf[16];
        wsprintfW(buf, L"%d", pItem->GetFontSize());
        SetDlgItemTextW(hDlg, IDC_FONT_SIZE, buf);
        wsprintfW(buf, L"%d", pItem->GetIntervalMs() / 1000);
        SetDlgItemTextW(hDlg, IDC_INTERVAL, buf);
        return TRUE;
    }
    case WM_COMMAND: {
        if (LOWORD(wParam) == IDOK) {
            wchar_t buf[16];

            GetDlgItemTextW(hDlg, IDC_FONT_SIZE, buf, 16);
            int fs = _wtoi(buf);
            if (fs >= 10 && fs <= 48) pItem->SetFontSize(fs);

            GetDlgItemTextW(hDlg, IDC_INTERVAL, buf, 16);
            int sec = _wtoi(buf);
            if (sec >= 1 && sec <= 30) pItem->SetIntervalMs(sec * 1000);

            EndDialog(hDlg, IDOK);
            return TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        break;
    }
    }
    return FALSE;
}

// 创建设置对话框模板（内存中构建）
static HGLOBAL CreateSettingsTemplate() {
    // 计算所需缓冲区大小
    // DLGTEMPLATE: 6 WORDs = 12 bytes
    // 2 WORDs (menu, class) = 4 bytes
    // Title: L"延迟监控设置" = 14 bytes (7 chars)
    // 6 controls * (DLGITEMTEMPLATE + text + padding)
    // 大致估算: 1024 bytes 足够
    const int BUF_SIZE = 2048;
    HGLOBAL hMem = GlobalAlloc(GPTR, BUF_SIZE);
    if (!hMem) return NULL;

    BYTE* buf = (BYTE*)GlobalLock(hMem);
    ZeroMemory(buf, BUF_SIZE);
    int offset = 0;

    // --- DLGTEMPLATE header ---
    DLGTEMPLATE* dlg = (DLGTEMPLATE*)(buf + offset);
    dlg->style = DS_CENTER | DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU;
    dlg->dwExtendedStyle = 0;
    dlg->cdit = 6;  // 6 controls
    dlg->x = 0;
    dlg->y = 0;
    dlg->cx = 240;
    dlg->cy = 140;
    offset += sizeof(DLGTEMPLATE);

    // --- No menu ---
    *(WORD*)(buf + offset) = 0; offset += 2;
    // --- No class ---
    *(WORD*)(buf + offset) = 0; offset += 2;
    // --- Title ---
    wcscpy_s((wchar_t*)(buf + offset), 16, L"延迟监控设置");
    offset += (int)wcslen(L"延迟监控设置") * 2 + 2;

    // 对齐到 DWORD
    offset = (offset + 3) & ~3;

    // 定义控件
    struct CtrlDef {
        DWORD style;
        DWORD exStyle;
        short x, y, cx, cy;
        WORD id;
        WORD classAtom;  // 0x0080=button, 0x0081=edit, 0x0082=static
        const wchar_t* text;
    };

    // 使用 0xFFFF + atom 格式来标识控件类
    CtrlDef ctrls[] = {
        // 静态文本 "字体大小:"
        { WS_CHILD | WS_VISIBLE | SS_RIGHT, 0, 10, 12, 70, 20, 0xFFFF, 0x0082, L"字体大小:" },
        // 编辑框 (字体大小值)
        { WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER, 0, 90, 12, 50, 20, IDC_FONT_SIZE, 0x0081, L"" },
        // 静态文本 "测速间隔(秒):"
        { WS_CHILD | WS_VISIBLE | SS_RIGHT, 0, 10, 42, 70, 20, 0xFFFF, 0x0082, L"间隔(秒):" },
        // 编辑框 (间隔值)
        { WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER, 0, 90, 42, 50, 20, IDC_INTERVAL, 0x0081, L"" },
        // 确定按钮
        { WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 0, 55, 80, 55, 25, IDOK, 0x0080, L"确定" },
        // 取消按钮
        { WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 130, 80, 55, 25, IDCANCEL, 0x0080, L"取消" },
    };

    for (int i = 0; i < 6; i++) {
        // 对齐到 DWORD
        offset = (offset + 3) & ~3;

        DLGITEMTEMPLATE* item = (DLGITEMTEMPLATE*)(buf + offset);
        item->style = ctrls[i].style;
        item->dwExtendedStyle = ctrls[i].exStyle;
        item->x = ctrls[i].x;
        item->y = ctrls[i].y;
        item->cx = ctrls[i].cx;
        item->cy = ctrls[i].cy;
        item->id = ctrls[i].id;
        offset += sizeof(DLGITEMTEMPLATE);

        // Class: 0xFFFF followed by atom WORD
        *(WORD*)(buf + offset) = 0xFFFF; offset += 2;
        *(WORD*)(buf + offset) = ctrls[i].classAtom; offset += 2;

        // Text
        int textLen = (int)wcslen(ctrls[i].text);
        wcscpy_s((wchar_t*)(buf + offset), textLen + 1, ctrls[i].text);
        offset += textLen * 2 + 2;

        // cbExtra
        *(WORD*)(buf + offset) = 0; offset += 2;
    }

    GlobalUnlock(hMem);
    return hMem;
}

// ==========================================
// 3. 主插件类
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
        case TMI_DESCRIPTION: return L"多站点测速，支持自定义测速 URL，颜色显示，可调字体和间隔";
        case TMI_AUTHOR: return L"YourName";
        case TMI_COPYRIGHT: return L"Copyright (C) 2026";
        case TMI_VERSION: return L"1.0.3";
        case TMI_URL: return L"";
        default: return L"";
        }
    }
    virtual OptionReturn ShowOptionsDialog(void* hParent) override {
        HWND hWnd = (HWND)hParent;
        HGLOBAL hTemplate = CreateSettingsTemplate();
        if (!hTemplate) return OR_OPTION_UNCHANGED;

        INT_PTR ret = DialogBoxIndirectParamW(
            GetModuleHandleW(NULL),
            (LPDLGTEMPLATE)GlobalLock(hTemplate),
            hWnd,
            SettingsDlgProc,
            (LPARAM)&m_latency_item
        );

        GlobalUnlock(hTemplate);
        GlobalFree(hTemplate);

        return (ret == IDOK) ? OR_OPTION_CHANGED : OR_OPTION_UNCHANGED;
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
// 4. 导出插件实例 (官方要求的规范签名)
// ==========================================
extern "C" __declspec(dllexport) ITMPlugin* TMPluginGetInstance()
{
    static CLatencyPlugin plugin;
    return &plugin;
}