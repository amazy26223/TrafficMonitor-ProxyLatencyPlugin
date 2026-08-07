# TrafficMonitor-ProxyLatencyPlugin
为 TrafficMonitor 开发的代理节点实时延迟监控插件 (C++)。

## ✨ 功能特点
通过调用 Windows 原生网络接口（WinINet），向极速测速节点发起轻量级请求，从而在任务栏实时显示当前科学上网代理节点的真实物理延迟。

## 📦 安装与使用
1. 在本仓库右侧的 **Releases** 页面下载最新的 `.dll` 插件文件。
2. 将下载的 `TrafficMonitorProxyPlugin.dll` 放入 TrafficMonitor 软件所在目录的 `plugins` 文件夹下（如果没有该文件夹请手动新建）。
3. 完全退出并重新启动 TrafficMonitor。
4. 在主界面或任务栏右键菜单中，进入“选项”或“显示设置”，勾选启用该插件即可。
