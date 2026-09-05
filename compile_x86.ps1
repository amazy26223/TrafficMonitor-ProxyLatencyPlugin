$vcDir = "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.51.36231"
$winSdkDir = "C:\Program Files (x86)\Windows Kits\10"
$sdkVer = "10.0.26100.0"
$srcDir = "C:\Users\Atom\AppData\Roaming\TRAE SOLO CN\ModularData\ai-agent\work-mode-projects\6a9ba9785aed04d2d8f5fbee\TrafficMonitor-ProxyLatencyPlugin"

$cl = "$vcDir\bin\Hostx64\x86\cl.exe"

$args = @(
    "/LD", "/EHsc", "/std:c++17", "/utf-8",
    "/I", "$vcDir\include",
    "/I", "$vcDir\atlmfc\include",
    "/I", "$winSdkDir\Include\$sdkVer\ucrt",
    "/I", "$winSdkDir\Include\$sdkVer\um",
    "/I", "$winSdkDir\Include\$sdkVer\shared",
    "/I", "$srcDir",
    "LatencyPlugin.cpp",
    "/link",
    "/LIBPATH:$vcDir\lib\x86",
    "/LIBPATH:$vcDir\atlmfc\lib\x86",
    "/LIBPATH:$winSdkDir\Lib\$sdkVer\ucrt\x86",
    "/LIBPATH:$winSdkDir\Lib\$sdkVer\um\x86",
    "/DLL",
    "/OUT:TrafficMonitorProxyPlugin_x86.dll",
    "/NOLOGO",
    "/MACHINE:X86"
)

& $cl $args