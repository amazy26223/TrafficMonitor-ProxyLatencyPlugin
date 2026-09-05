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

## 下载编译好的 DLL

在 [Releases](https://github.com/amazy26223/TrafficMonitor-ProxyLatencyPlugin/releases) 页面下载最新版本：

- `TrafficMonitorProxyPlugin_x64.dll` — 64 位系统
- `TrafficMonitorProxyPlugin_x86.dll` — 32 位系统

## 编译说明

项目目录中提供了编译脚本，或使用 Visual Studio 手动编译：

1. 新建一个 **DLL** 类型的项目
2. 将 `LatencyPlugin.cpp` 和 `PluginInterface.h` 添加到项目
3. 项目属性中确保：
   - 字符集设置为 **使用 Unicode 字符集**
   - 在 `C/C++` → `预处理器` 中添加 `NOMINMAX`、`UNICODE`、`_UNICODE`
   - C++ 语言标准 ≥ C++17
4. 编译生成 DLL

也可直接使用项目中的编译脚本：
```powershell
# 编译 x64 版本
.\compile.ps1

# 编译 x86 版本
.\compile_x86.ps1
```

## 测速 URL 配置

插件支持通过配置文件自定义测速站点。在 DLL 同级目录下创建 `proxy_latency_urls.txt`，每行一个 URL（以 `#` 开头的行为注释）：

```
# 从代理软件日志中，找一个实际走代理节点的 URL
http://cp.cloudflare.com/generate_204
# 也可以添加其他站点，插件会取中位数作为最终结果
```

如果文件不存在，默认使用 `http://cp.cloudflare.com/generate_204`。

## 版本命名规范

采用标准语义化版本（SemVer）：`主版本.次版本.修订号`

| 版本 | 说明 |
|------|------|
| `1.0.x` | 快速迭代阶段，修订号递增 |
| `1.x.0` | 次版本增加 = 新增功能 |
| `2.0.0` | 主版本增加 = 重大重构 |

每次迭代只需递增修订号（如 `1.0.1` → `1.0.2` → `1.0.3`），简单清晰。

## 版本历史

- **v1.0.2** — 修复延迟过低问题：改用中位数算法，新增配置文件自定义测速 URL
- **v1.0.1** — 多站点测速：增加至 6 个测速端点，取最小延迟
- **v1.0.0** — 初始版本：单点 Cloudflare 测速