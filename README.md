# TrafficMonitor-ProxyLatencyPlugin

为 TrafficMonitor 开发的代理节点实时延迟监控插件 (C++)。

## 功能特点

- 通过调用 Windows 原生网络接口（WinINet），向多个轻量级测速节点发起请求
- 从所有成功节点中取**最小延迟**，有效规避单点波动或故障导致的误判
- 任务栏实时显示当前科学上网代理节点的真实物理延迟
- 自动跟随系统代理设置（`INTERNET_OPEN_TYPE_PRECONFIG`），兼容 Clash、v2ray 等常见代理软件

## 测速站点

插件内置 6 个全球知名 CDN/厂商的轻量级测速端点，全部使用 `204 No Content` 或极小响应体，确保测速请求本身不会引入额外延迟：

| 站点 | 提供商 | 端点 |
|------|--------|------|
| Cloudflare | Cloudflare | `/generate_204` |
| Gstatic | Google | `/generate_204` |
| Connectivitycheck Gstatic | Google | `/generate_204` |
| Firefox Portal | Mozilla | `/success.txt` |
| Captive Apple | Apple | `/generate_204` |
| HiCloud | 华为 | `/generate_204` |

显示格式：`50 ms (4/6)` — 表示最小延迟 50ms，6 个站点中成功测通 4 个。

## 安装与使用

1. 在本仓库右侧的 **Releases** 页面下载最新的 `.dll` 插件文件。
2. 将下载的 `TrafficMonitorProxyPlugin.dll` 放入 TrafficMonitor 软件所在目录的 `plugins` 文件夹下（如果没有该文件夹请手动新建）。
3. 完全退出并重新启动 TrafficMonitor。
4. 在主界面或任务栏右键菜单中，进入"选项"或"显示设置"，勾选启用该插件即可。

## 编译说明

使用 Visual Studio 创建空 C++ 项目：

1. 新建一个 **DLL** 类型的项目
2. 将 `LatencyPlugin.cpp` 和 `PluginInterface.h` 添加到项目
3. 项目属性中确保：
   - 字符集设置为 **使用 Unicode 字符集**
   - C++ 语言标准 ≥ C++11
4. 编译生成 DLL

## 版本历史

- **v1.1** — 多站点测速：增加至 6 个测速端点，取最小延迟，提高准确性
- **v1.0** — 初始版本：单点 Cloudflare 测速