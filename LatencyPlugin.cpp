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
#include <thread>
#include <atomic>
#include <limits>
// ==========================================
// 新增：引入 Windows 自带的网络库和时间库
#include <wininet.h>
#pragma comment(lib, "wininet.lib") // 告诉编译器自动链接网络库
#include <chrono>
// ==========================================

// ==========================================
// 测速站点列表
// 全部使用轻量级 204/成功页面端点，仅需建立连接即可完成测速
// 多站点取最小值，有效规避单点波动或故障导致的误判，大幅提高准确性
// ==========================================
static const wchar_t* TEST_URLS[] = {
    L"http://cp.cloudflare.com/generate_204",             // Cloudflare 全球 CDN 测速节点
    L"http://www.gstatic.com/generate_204",               // Google 静态资源测速节点
    L"http://connectivitycheck.gstatic.com/generate_204", // Google 连接检查节点
    L"http://detectportal.firefox.com/success.txt",       // Firefox 连通性检测
    L"http://captive.apple.com/generate_204",             // Apple 网络连通性检查
    L"http://connectivitycheck.platform.hicloud.com/generate_204", // 华为连通性检查
};
static const int TEST_URL_COUNT = sizeof(TEST_URLS) / sizeof(TEST_URLS[0]);

// ==========================================
// 1. 数据显示项类 (负责在任务栏上显示数据)
// ==========================================
class CLatencyItem : public IPluginItem
{
private:
    std::wstring m_item_name = L"代理延迟";
    std::wstring m_item_value = L"检测中...";
    std::atomic<bool> m_is_updating{ false };

    // 测量到单个站点的延迟 (毫秒)，失败返回 -1
    long long MeasureLatency(HINTERNET hInternet, const wchar_t* url) {
        auto start_time = std::chrono::high_resolution_clock::now();
        HINTERNET hUrl = InternetOpenUrl(hInternet, url, NULL, 0, INTERNET_FLAG_RELOAD, 0);
        if (hUrl) {
            auto end_time = std::chrono::high_resolution_clock::now();
            InternetCloseHandle(hUrl);
            return std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        }
        return -1;
    }

public:
    virtual const wchar_t* GetItemName() const override { return m_item_name.c_str(); }
    virtual const wchar_t* GetItemId() const override { return L"proxy_latency_item_01"; }
    virtual const wchar_t* GetItemLableText() const override { return L"延迟: "; }
    virtual const wchar_t* GetItemValueText() const override { return m_item_value.c_str(); }
    virtual const wchar_t* GetItemValueSampleText() const override { return L"999 ms"; }

    // 多站点并发测速逻辑
    void UpdateLatencyAsync() {
        if (m_is_updating.exchange(true)) return; // 如果上一轮测速还没结束，就跳过

        std::thread([this]() {
            HINTERNET hInternet = InternetOpen(L"TM_Plugin", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
            if (hInternet) {
                long long min_latency = std::numeric_limits<long long>::max();
                int success_count = 0;

                for (int i = 0; i < TEST_URL_COUNT; i++) {
                    long long delay = MeasureLatency(hInternet, TEST_URLS[i]);
                    if (delay >= 0) {
                        success_count++;
                        if (delay < min_latency) {
                            min_latency = delay;
                        }
                    }
                }

                if (success_count > 0) {
                    // 显示最小值 + 成功站点数，例如 "50 ms (4/6)"
                    m_item_value = std::to_wstring(min_latency) + L" ms (" 
                                 + std::to_wstring(success_count) + L"/" 
                                 + std::to_wstring(TEST_URL_COUNT) + L")";
                } else {
                    m_item_value = L"超时";
                }
                InternetCloseHandle(hInternet);
            } else {
                m_item_value = L"网络错误";
            }
            m_is_updating = false; // 测速完成，允许下一次测速
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
        case TMI_DESCRIPTION: return L"多站点并发测速，显示当前代理节点的最小延迟";
        case TMI_AUTHOR: return L"YourName";
        case TMI_COPYRIGHT: return L"Copyright (C) 2026";
        case TMI_VERSION: return L"1.1";
        case TMI_URL: return L"";
        default: return L"";
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