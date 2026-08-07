#include "pch.h" 
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

// ==========================================
// 新增：引入 Windows 自带的网络库和时间库
#include <wininet.h>
#pragma comment(lib, "wininet.lib") // 告诉编译器自动链接网络库
#include <chrono>
// ==========================================

// ==========================================
// 1. 数据显示项类 (负责在任务栏上显示数据)
// ==========================================
class CLatencyItem : public IPluginItem
{
private:
    std::wstring m_item_name = L"代理延迟";
    std::wstring m_item_value = L"检测中...";
    std::atomic<bool> m_is_updating{ false };

public:
    virtual const wchar_t* GetItemName() const override { return m_item_name.c_str(); }
    virtual const wchar_t* GetItemId() const override { return L"proxy_latency_item_01"; }
    virtual const wchar_t* GetItemLableText() const override { return L"延迟: "; }
    virtual const wchar_t* GetItemValueText() const override { return m_item_value.c_str(); }
    virtual const wchar_t* GetItemValueSampleText() const override { return L"999 ms"; }

    // 修改：真实的测速逻辑
    void UpdateLatencyAsync() {
        if (m_is_updating) return; // 如果上一轮测速还没结束，就跳过
        m_is_updating = true;

        std::thread([this]() {
            // 记录开始时间
            auto start_time = std::chrono::high_resolution_clock::now();

            // 初始化网络连接 (INTERNET_OPEN_TYPE_PRECONFIG 会自动走 Flclash 代理)
            HINTERNET hInternet = InternetOpen(L"TM_Plugin", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
            if (hInternet) {
                // 请求 Cloudflare 的 204 空白页面测速
                HINTERNET hUrl = InternetOpenUrl(hInternet, L"http://cp.cloudflare.com/generate_204", NULL, 0, INTERNET_FLAG_RELOAD, 0);

                if (hUrl) {
                    // 如果请求成功，记录结束时间并计算差值
                    auto end_time = std::chrono::high_resolution_clock::now();
                    auto delay = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

                    m_item_value = std::to_wstring(delay) + L" ms";
                    InternetCloseHandle(hUrl);
                }
                else {
                    m_item_value = L"超时";
                }
                InternetCloseHandle(hInternet);
            }
            else {
                m_item_value = L"网络错误";
            }

            m_is_updating = false; // 测速完成，允许下一次测速
            }).detach();
    }
};

// ... 下面的 CLatencyPlugin 类和导出实例部分保持原样，不需要任何改动 ...

// ==========================================
// 2. 主插件类 (继承真实的 ITMPlugin 基类)
// ==========================================
class CLatencyPlugin : public ITMPlugin
{
private:
    CLatencyItem m_latency_item;

public:
    // 获取插件包含的显示项
    virtual IPluginItem* GetItem(int index) override {
        if (index == 0) return &m_latency_item;
        return nullptr;
    }

    // 主程序定时触发数据更新请求
    virtual void DataRequired() override {
        m_latency_item.UpdateLatencyAsync();
    }

    // 集中处理插件的所有基本信息
    virtual const wchar_t* GetInfo(PluginInfoIndex index) override {
        switch (index)
        {
        case TMI_NAME: return L"代理节点延迟监控";
        case TMI_DESCRIPTION: return L"显示当前代理软件的节点延迟信息";
        case TMI_AUTHOR: return L"YourName";
        case TMI_COPYRIGHT: return L"Copyright (C) 2026";
        case TMI_VERSION: return L"1.0";
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