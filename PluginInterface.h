#pragma once
class IPluginDrawer;
//插件显示项目的接口
class IPluginItem
{
public:
    /**
     * @brief   获取显示项目的名称
     * @return  const wchar_t*
     */
    virtual const wchar_t* GetItemName() const = 0;
    /**
     * @brief   获取显示项目的唯一ID
     * @return  const wchar_t*
     */
    virtual const wchar_t* GetItemId() const = 0;
    /**
     * @brief   获取项目标签的文本
     * @return  const wchar_t*
     */
    virtual const wchar_t* GetItemLableText() const = 0;
    /**
     * @brief   获取项目数值的文本
     * @detail  由于此函数可能会被频繁调用，因此不要在这里获取监控数据，
     *  而是在ITMPlugin::DataRequired函数中获取数据后保存起来，然后在这里返回获取的数值
     * @return  const wchar_t*
     */
    virtual const wchar_t* GetItemValueText() const = 0;
    /**
     * @brief   获取项目数值的示例文本
     * @detail  此函数返回的字符串的长度会用于计算显示区域的宽度
     * @return  const wchar_t*
     */
    virtual const wchar_t* GetItemValueSampleText() const = 0;
    /**
     * @brief   显示区域是否由插件自行绘制
     * @detail
     *  如果返回false，则根据GetItemLableText和GetItemValueText返回的文本由主程序绘制显示区域，重写DrawItem函数将不起作用。
     *  如果重写此函数并返回true，则必须重写DrawItem函数并在里面添加绘制显示区域的代码，
     *  此时GetItemLableText、GetItemValueText和GetItemValueSampleText的返回值将被主程序忽略
     * @return  bool
     */
    virtual bool IsCustomDraw() const { return false; }
    /**
     * @brief   获取显示区域的宽度
     * @detail
     *  只有当CustomDraw()函数返回true时重写此函数才有效。
     *  返回的值为DPI为96（100%）时的宽度，主程序会根据当前系统DPI的设置自动按比例放大，
     *  因此你不需要为不同的DPI设置返回不同的值。
     *  需要注意的是，这里的返回值代表了自绘区域所需要的最小宽度，DrawItem函数中的参数w的值可能会大于这个值
     * @return  int
     */
    virtual int GetItemWidth() const { return 0; }
    /**
     * @brief   自定义绘制显示区域的函数，只有当CustomDraw()函数返回true时重写此函数才有效
     * @param   void * hDC 绘图的上下文句柄
     * @param   int x 绘图的矩形区域
     * @param   int y
     * @param   int w
     * @param   int h
     * @param   bool dark_mode 深色模式为true，浅色模式为false
     * @return  void
     */
    virtual void DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode) {}
    /**
     * @brief   获取显示区域的宽度
     * @detail
     *  只有当CustomDraw()函数返回true时重写此函数才有效。
     *  此函数和GetItemWidth不同，插件可以根据参数hDC来计算需要的宽度，
     *  它返回的是实际的宽度，主程序不会根据当前系统的DPI对返回值进行放大。
     *  需要注意的是，这里的返回值代表了自绘区域所需要的最小宽度，DrawItem函数中的参数w的值可能会大于这个值
     * @param   void * hDC 绘图的上下文句柄
     * @return  int
     */
    virtual int GetItemWidthEx(void* hDC) const { return 0; }
    /** 鼠标事件的类型 */
    enum MouseEventType
    {
        MT_LCLICKED,    /**< 点击了鼠标左键 */
        MT_RCLICKED,    /**< 点击了鼠标右键 */
        MT_DBCLICKED,   /**< 双击了鼠标左键 */
        MT_WHEEL_UP,    /**< 向上滚动了鼠标滚轮 */
        MT_WHEEL_DOWN,  /**< 向下滚动了鼠标滚轮 */
    };
    enum MouseEventFlag
    {
        MF_TASKBAR_WND = 1 << 0,        /**< 是否为任务栏窗口的鼠标事件 */
    };
    /**
     * @brief   当插件显示区域有鼠标事件时由主程序调用
     * @param   MouseEventType type 鼠标事件的类型
     * @param   int x 鼠标指针所在的x坐标
     * @param   int y 鼠标指针所在的y坐标
     * @param   void* hWnd 产生此鼠标事件的窗口的句柄（主窗口或任务栏窗口）
     * @param   int flag 为若干MouseEventFlag枚举常量的组合
     * @return  int 如果返回非0，则主程序认为插件已经对此鼠标事件作出了全部的响应，主程序将不会再对此鼠标事件做额外的响应。
     *   例如当type为MT_RCLICKED时，如果程序返回0，则会弹出主程序提供的右键菜单；而返回非0时，主程序不会再做任何处理。
     */
    virtual int OnMouseEvent(MouseEventType type, int x, int y, void* hWnd, int flag) { return 0; }
    enum KeyboardEventFlag
    {
        KF_TASKBAR_WND = 1 << 0,        /**< 是否为任务栏窗口的键盘事件 */
    };
    /**
     * @brief   当插件显示区域有键盘事件时由主程序调用
     * @param   int key 按下的键
     * @param   bool ctrl 是否按下了Ctrl键
     * @param   bool shift 是否按下了Shift键
     * @param   bool alt 是否按下了Alt键
     * @param   void* hWnd 产生此键盘事件的窗口的句柄（主窗口或任务栏窗口）
     * @param   int flag 为若干KeyboardEventFlag枚举常量的组合
     * @return  int 如果返回非0，则主程序认为插件已经对此键盘事件作出了全部的响应，主程序将不会再对此键盘事件做额外的响应。
     */
    virtual int OnKeboardEvent(int key, bool ctrl, bool shift, bool alt, void* hWnd, int flag) { return 0; }
    enum ItemInfoType
    {
    };
    //预留的接口
    virtual void* OnItemInfo(ItemInfoType, void* para1, void* para2) { return 0; }
    /**
     * @brief   是否在在任务栏中显示此项目的资源占用图
     * @return  1：显示，0：不显示
     */
    virtual int IsDrawResourceUsageGraph() const { return 0; }
    /**
     * @brief   获取资源占用图的值。当IsDrawResourceUsageGraphType返回值不为0时有效
     * @return  float 资源占用图的值，范围为0.0~1.0。
     */
    virtual float GetResourceUsageGraphValue() const { return 0.0; }
    /**
     * @brief   自定义绘制显示区域的函数，只有当CustomDraw()函数返回true时重写此函数才有效
     * @param   IPluginDrawer* pDrawer 用于插件绘图接口的的指针
     * @param   int x 绘图的矩形区域
     * @param   int y
     * @param   int w
     * @param   int h
     * @param   bool dark_mode 深色模式为true，浅色模式为false
     * @return  bool
     */
    virtual bool DrawItemEx(IPluginDrawer* pDrawer, int x, int y, int w, int h, bool dark_mode) { return false; }
    /**
     * @brief   是否在任务中独占双行
     * @return  1：是，0：否
     */
    virtual int IsDoubleLineExclusive() const { return 0; }
};
class ITrafficMonitor;
///////////////////////////////////////////////////////////////////////////////////////////////////////
//插件接口
class ITMPlugin
{
public:
    /**
     * @brief   插件接口的版本，仅当修改了插件接口时才会修改这里的返回值。
     * @attention 插件开发者不应该修改这里的返回值，也不应该重写此虚函数。
     * @return  int
     */
    virtual int GetAPIVersion() const { return 8; }
    /**
     * @brief   获取插件显示项目的对象
     * @detail  一个插件dll可以提供多个实现IPluginItem接口的对象，对应多个显示项目。
     *  当index的值大于或等于0且小于IPluginItem接口的对象的个数时，返回对象的IPluginItem接口的指针，其他情况应该返回空指针。
     *  例如插件提供两个显示项目，则当index等于0或1时返回对应IPluginItem接口的对象，其他值时必须返回空指针。
     * @param   int index 对象的索引
     * @return  IPluginItem* 插件显示项目的对象
     */
    virtual IPluginItem* GetItem(int index) = 0;
    /**
     * @brief   主程序会每隔一定时间调用此函数，插件需要在函数里获取一次监控的数据
     */
    virtual void DataRequired() = 0;
    /** 选项设置对话框的返回值 */
    enum OptionReturn
    {
        OR_OPTION_CHANGED,          /**< 选项设置对话框中更改了选项设置 */
        OR_OPTION_UNCHANGED,        /**< 选项设置对话框中未更改选项设置 */
        OR_OPTION_NOT_PROVIDED      /**< 未提供选项设置对话框 */
    };
    /**
     * @brief   主程序调用此函数以打开插件的选项设置对话框
     * @detail  此函数不一定要重写。如果插件提供了选项设置界面，则应该重写此函数，并在最后返回OR_OPTION_CHANGED或OR_OPTION_UNCHANGED。
     * @param   void * hParent 父窗口的句柄
     *  返回值为OR_OPTION_NOT_PRVIDED则认为插件不提供选项设置对话框。
     * @return  ITMPlugin::OptionReturn
     */
    virtual OptionReturn ShowOptionsDialog(void* hParent) { return OR_OPTION_NOT_PROVIDED; }
    /** 插件信息的索引 */
    enum PluginInfoIndex
    {
        TMI_NAME,           /**< 名称 */
        TMI_DESCRIPTION,    /**< 描述 */
        TMI_AUTHOR,         /**< 作者 */
        TMI_COPYRIGHT,      /**< 版权 */
        TMI_VERSION,        /**< 版本 */
        TMI_URL,            /**< 主页 */
        TMI_MAX             /**< 插件信息的最大值 */
    };
    /**
     * @brief   获取此插件的信息，根据index的值返回对应的信息
     */
    virtual const wchar_t* GetInfo(PluginInfoIndex index) = 0;
    /** 主程序的监控信息 */
    struct MonitorInfo
    {
        unsigned long long up_speed{};
        unsigned long long down_speed{};
        int cpu_usage{};
        int memory_usage{};
        int gpu_usage{};
        int hdd_usage{};
        int cpu_temperature{};
        int gpu_temperature{};
        int hdd_temperature{};
        int main_board_temperature{};
        int cpu_freq{};
    };
    /**
     * @brief   主程序调用此函数以向插件传递所有获取到的监控信息
     */
    virtual void OnMonitorInfo(const MonitorInfo& monitor_info) {}
    /**
     * @brief   获取插件要在鼠标提示中显示的文本
     */
    virtual const wchar_t* GetTooltipInfo() { return L""; }
    enum ExtendedInfoIndex
    {
        EI_LABEL_TEXT_COLOR,    //绘图的标签文本颜色
        EI_VALUE_TEXT_COLOR,    //绘图的数值文本颜色
        EI_DRAW_TASKBAR_WND,    //是否绘制任务栏窗口
        //主窗口选项设置
        EI_NAIN_WND_NET_SPEED_SHORT_MODE,   //网速显示简洁模式
        EI_MAIN_WND_SPERATE_WITH_SPACE,     //数值和单位使用空格分隔
        EI_MAIN_WND_UNIT_BYTE,              //网速单位是否使用B（字节）
        EI_MAIN_WND_UNIT_SELECT,            //网速单位选择（0：自动，1：固定为KB/s，2：固定为MB/s）
        EI_MAIN_WND_NOT_SHOW_UNIT,          //不显示网速单位
        EI_MAIN_WND_NOT_SHOW_PERCENT,       //不显示百分号
        //任务栏窗口设置
        EI_TASKBAR_WND_NET_SPEED_SHORT_MODE,    //网速显示简洁模式
        EI_TASKBAR_WND_SPERATE_WITH_SPACE,      //数值和单位使用空格分隔
        EI_TASKBAR_WND_VALUE_RIGHT_ALIGN,       //数值右对齐
        EI_TASKBAR_WND_NET_SPEED_WIDTH,         //网速数据宽度
        EI_TASKBAR_WND_UNIT_BYTE,               //网速单位是否使用B（字节）
        EI_TASKBAR_WND_UNIT_SELECT,             //网速单位选择（0：自动，1：固定为KB/s，2：固定为MB/s）
        EI_TASKBAR_WND_NOT_SHOW_UNIT,           //不显示网速单位
        EI_TASKBAR_WND_NOT_SHOW_PERCENT,        //不显示百分号
        EI_CONFIG_DIR,                      //配置文件的目录
    };
    /**
     * @brief   主程序调用此函数以向插件传递更多信息
     * @param   ExtendedInfoIndex index 信息的索引，用于区分向插件传递的信息
     * @param   const wchar_t* data 传递的数据
     * @return  void
     */
    virtual void OnExtenedInfo(ExtendedInfoIndex index, const wchar_t* data) {}
    /**
     * @brief   获取插件的图标，HICON格式
     */
    virtual void* GetPluginIcon() { return nullptr; }
    /**
     * @brief   获取插件的命令的数量
     * @return  int 插件的命令的数量
     */
    virtual int GetCommandCount() { return 0; }
    /**
     * @brief   获取插件的命令名称
     * @param   int command_index 插件命令的索引（从0开始，小于GetCommandCount()的返回值）
     * @return  wchar_t* 插件命令的名称
     */
    virtual const wchar_t* GetCommandName(int command_index) { return nullptr; }
    /**
     * @brief   获取插件的命令图标
     * @param   int command_index 插件命令的索引（从0开始，小于GetCommandCount()的返回值）
     * @return  void* 插件命令的图标，HICON格式
     */
    virtual void* GetCommandIcon(int command_index) { return nullptr; }
    /**
     * @brief   执行一个插件命令时由框架调用
     * @param   int command_index 插件命令的索引（从0开始，小于GetCommandCount()的返回值）
     * @param   void* hWnd 发送此命令的窗口句柄
     * @param   void* para 预留参数
     */
    virtual void OnPluginCommand(int command_index, void* hWnd, void* para) {}
    /**
    * @brief   获取插件命令是否处于勾选状态
    * @param   int command_index 插件命令的索引（从0开始，小于GetCommandCount()的返回值）
    * @return  1：已勾选；0：未勾选
    */
    virtual int IsCommandChecked(int command_index) { return false; }
    /**
     * @brief   插件初始化
     * @detail  当插件被加载时被调用，传递ITrafficMonitor接口的指针。插件可以保存此指针以调用ITrafficMonitor接口中的函数
     * @param   pApp
     */
    virtual void OnInitialize(ITrafficMonitor* pApp) {}
};
///////////////////////////////////////////////////////////////////////////////////////////////////////
//主程序接口
class ITrafficMonitor
{
public:
    /**
     * @brief   获取此接口的版本。
     * @attention 插件在调用ITrafficMonitor中的函数时需要先判断接口的版本
     * @return  int
     */
    virtual int GetAPIVersion() = 0;
    /**
     * @brief   获取TrafficMonitor的版本。
     * @return  const wchar_t*
     */
    virtual const wchar_t* GetVersion() = 0;
    /** 主程序的所有监控信息 */
    enum MonitorItem
    {
        MI_UP,                  /**< 上传速度 */
        MI_DOWN,                /**< 下载速度 */
        MI_CPU,                 /**< CPU利用率 */
        MI_MEMORY,              /**< 内存利用率 */
        MI_GPU_USAGE,           /**< 显卡利用率 */
        MI_CPU_TEMP,            /**< CPU温度 */
        MI_GPU_TEMP,            /**< 显卡温度 */
        MI_HDD_TEMP,            /**< 硬盘温度 */
        MI_MAIN_BOARD_TEMP,     /**< 主板温度 */
        MI_HDD_USAGE,           /**< 硬盘利用率 */
        MI_CPU_FREQ,            /**< CPU频率 */
        MI_TODAY_UP_TRAFFIC,    /**< 今日上传流量 */
        MI_TODAY_DOWN_TRAFFIC   /**< 今日下载流量 */
    };
    /**
     * @brief   获取一个主程序的监控信息的数值
     *          （ITMPlugin::OnMonitorInfo将被弃用）
     * @param   item 要获取监控信息的项目
     * @return  获取到监控信息
     */
    virtual double GetMonitorValue(MonitorItem item) = 0;
    /**
     * @brief   以字符串的形式获取一个主程序监控信息的数值
     * @param   item 要获取监控信息的项目
     * @param   is_main_window 是否为主窗口的数值
     * @return  获取到监控信息
     */
    virtual const wchar_t* GetMonitorValueString(MonitorItem item, int is_main_window = false) = 0;
    /**
     * @brief   显示一个通知消息
     * @param   strMsg 要显示的通知消息
     */
    virtual void ShowNotifyMessage(const wchar_t* strMsg) = 0;
    /**
     * @brief   获取当前语言id
     * @return  当前主程序的语言id
     */
    virtual unsigned short GetLanguageId() const = 0;
    /**
     * @brief   获取配置文件目录
     * @return  配置文件目录
     */
    virtual const wchar_t* GetPluginConfigDir() const = 0;
    /** 主程序DPI类型 */
    enum DPIType
    {
        DPI_MAIN_WND,   /**< 主窗口的DPI */
        DPI_TASKBAR     /**< 任务栏窗口的DPI */
    };
    /**
     * @brief   获取主程序DPI
     * @return  主程序DPI
     */
    virtual int GetDPI(DPIType type) const = 0;
    /**
     * @brief   获取当前系统主题颜色
     * @return  COLORREF格式的颜色值
     */
    virtual unsigned int GetThemeColor() const = 0;
    /**
     * @brief   获取当前语言的字符串资源
     * @param   str_id 字符串资源id
     * @param   str_section 字符串资源在ini文件中的段名称
     * @return
     */
    virtual const wchar_t* GetStringRes(const wchar_t* key, const wchar_t* section) = 0;
    /**
     * @brief   获取主窗口的句柄
     * @return
     */
    virtual void* GetMainWindowHwnd() = 0;
    /**
     * @brief   获取任务栏窗口的句柄。不显示任务栏窗口时，返回值为空
     * @return
     */
    virtual void* GetTaskbarWindowHwnd() = 0;
    /**
     * API Version
     *      1       新增GetMainWindowHwnd、GetTaskbarWindowHwnd函数
     */
};
//插件绘图接口
class IPluginDrawer
{
public:
    /**
     * @brief   获取此接口的版本。
     * @return  int
     */
    virtual int GetAPIVersion() = 0;
    // 拉伸模式
    enum StretchMode
    {
        STRETCH_MODE_STRETCH,   //拉伸
        STRETCH_MODE_TILE,      //平铺
    };
    /**
     * @brief   绘制文本
     * @param   int x 文本的x坐标
     * @param   int y 文本的y坐标
     * @param   int w 文本的宽度
     * @param   int h 文本的高度
     * @param   const wchar_t* text 要绘制的文本
     * @param   const wchar_t* font_name 字体名称
     * @param   int font_size 字体大小
     * @param   bool bold 是否加粗
     * @param   bool italic 是否斜体
     * @param   unsigned int color 字体颜色，COLORREF格式
     * @param   int format 文本的绘制格式，参考DrawText函数的格式，见说明部分
     * @return  void
     */
    virtual void DrawText(int x, int y, int w, int h, const wchar_t* text, const wchar_t* font_name, int font_size, bool bold, bool italic, unsigned int color, int format) = 0;
    /**
     * @brief   获取文本的范围
     * @param   const wchar_t* text 要输入的文本
     * @param   const wchar_t* font_name 字体名称
     * @param   int font_size 字体大小
     * @param   bool bold 是否加粗
     * @param   bool italic 是否斜体
     * @param   int* width 返回文本的宽度
     * @param   int* height 返回文本的宽度
     * @return  void
     */
    virtual void GetTextExtent(const wchar_t* text, const wchar_t* font_name, int font_size, bool bold, bool italic, int* width, int* height) = 0;
    /**
     * @brief   填充矩形
     * @param   int x 矩形的x坐标
     * @param   int y 矩形的y坐标
     * @param   int w 矩形的宽度
     * @param   int h 矩形的高度
     * @param   unsigned int color 矩形的填充颜色，COLORREF格式
     * @return  void
     */
    virtual void FillRect(int x, int y, int w, int h, unsigned int color) = 0;
    /**
     * @brief   绘制矩形边框
     * @param   int x 矩形的x坐标
     * @param   int y 矩形的y坐标
     * @param   int w 矩形的宽度
     * @param   int h 矩形的高度
     * @param   unsigned int color 矩形边框的颜色，COLORREF格式
     * @param   int width 矩形边框的宽度
     * @return  void
     */
    virtual void DrawRect(int x, int y, int w, int h, unsigned int color, int width) = 0;
    /**
     * @brief   绘制圆角矩形边框
     * @param   int x 矩形的x坐标
     * @param   int y 矩形的y坐标
     * @param   int w 矩形的宽度
     * @param   int h 矩形的高度
     * @param   int corner 圆角弧度
     * @param   unsigned int color 矩形边框的颜色，COLORREF格式
     * @param   int width 矩形边框的宽度
     * @return  void
     */
    virtual void DrawRoundRect(int x, int y, int w, int h, int corner, unsigned int color, int width) = 0;
    /**
     * @brief   绘制椭圆
     * @param   int x 椭圆的x坐标
     * @param   int y 椭圆的y坐标
     * @param   int w 椭圆的宽度
     * @param   int h 椭圆的高度
     * @param   unsigned int color 椭圆的边框颜色，COLORREF格式
     * @param   int width 椭圆边框的宽度
     * @return  void
     */
    virtual void DrawEllipse(int x, int y, int w, int h, unsigned int color, int width) = 0;
    /**
     * @brief   填充椭圆
     * @param   int x 椭圆的x坐标
     * @param   int y 椭圆的y坐标
     * @param   int w 椭圆的宽度
     * @param   int h 椭圆的高度
     * @param   unsigned int color 椭圆的填充颜色，COLORREF格式
     * @return  void
     */
    virtual void FillEllipse(int x, int y, int w, int h, unsigned int color) = 0;
    /**
     * @brief   绘制线条
     * @param   int start_x 线条起点的x坐标
     * @param   int start_y 线条起点的y坐标
     * @param   int end_x 线条终点的x坐标
     * @param   int end_y 线条终点的y坐标
     * @param   unsigned int color 线条的颜色，COLORREF格式
     * @param   int width 线条的宽度
     * @return  void
     */
    virtual void DrawLine(int start_x, int start_y, int end_x, int end_y, unsigned int color, int width) = 0;
    /**
     * @brief   绘制图片
     * @param   int x 图片的x坐标
     * @param   int y 图片的y坐标
     * @param   int w 图片的宽度
     * @param   int h 图片的高度
     * @param   void* hBitmap 位图的句柄
     * @param   int stretch_mode 拉伸模式，为StretchMode枚举值
     * @return  void
     */
    virtual void DrawImage(int x, int y, int w, int h, void* hBitmap, int stretch_mode) = 0;
    /**
     * @brief   获取当前DPI
     * @return  int
     */
    virtual int GetDPI() = 0;
};