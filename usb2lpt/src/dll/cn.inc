// Translated by Yue Guan

// DIALOG(EX)
#define S10000	"模拟端口"
#define S10001	"模拟端口地址 (&A)："
#define S10002	"LPT增强模式 (&E)："
#define S10003	"用于IN和OUT指令的捕获方法"
#define S10004	"调试寄存器捕获 (&D)"
#define S10005	"(捕获所有IN、OUT指令)"
#define S10006	"执行分配，即使以前分配 (&F)"
#define S10007	"&READ_PORT_UCHAR / WRITE_PORT_UCHAR 转向 (&R)"
#define S10008	"(更快、更稳定)"
#define S10009	"工作选项"
#define S10010	"回写OUT指令 (&W)"
#define S10011	"(连接多个OUT指令给一个USB包)"
#define S10012	"设置延时发包时间 (&T)"
#define S10014	"回读输出寄存器映像 (&B)"
#define S10015	"更精确… &M"
#define S10016	"(返回最后IN指令值；当输出被外部硬件拉低时无效)"

#define S10100	"统计表"
#define S10101	"调试寄存器捕获"
#define S10102	"清零 (&R)"
#define S10103	"OUT指令："
#define S10104	"IN指令："
#define S10105	"模拟错误："
#define S10106	"(不支持IN/OUT指令，如OUTSB)"
#define S10107	"调试寄存器丢失："
#define S10108	"(未知驱动写向调试寄存器)"
#define S10109	"100ms内核定时器可检测到覆盖调试寄存器"
#define S10110	"设置："
#define S10114	"READ_PORT_UCHAR/WRITE_PORT_UCHAR转向"
#define S10115	"清零 (&Z)"
#define S10116	"过大的:"
#define S10117	"发出一个IN指令… (&E)"
#define S10118	"更新固件… (&U)"
#define S10119	"haftmann#software，开姆尼斯工业大学，\n电子与信息技术学院，\n测量与传感器技术学教授，2005-2009年"

#define S10200	"回读输出寄存器"
#define S10201	"数据口（基址+0）"
#define S10202	"控制口（基址+2）"
#define S10203	"其他口（ECP,EPP）"
#define S10204	"确定"
#define S10205	"取消"

#define S10302	"错误的设置可能损坏您的计算机或破坏硬盘数据！建议使用下拉菜单中提供的选项。"
#define S10304	"LPT 增强模式能够捕获更多端口地址。该功能不常用，选择SPP 即可。"
#define S10305	"这些设置可以以后在Windows设备管理器中设置。点击USB2LPT设备，打开属性。"

#define S10400	"发出一个IN指令"
#define S10401	"功能寄存器"
#define S10402	"(登记册的特点将保持持续的电可擦除只读存储器的USB2LPT装置。可以通过DeviceIoControl或基地)"
#define S10403	"打开收藏的数据端口（+0） (&D)"
#define S10404	"打开收藏的控制端口（+2） (&C)"
#define S10405	"关掉仰卧起坐（和5V在销25若然有线） (&P)"
#define S10406	"直接输入/输出模式（USB2LPT转换为I/O设备） (&M)"
#define S10407	"(弱5V的不是强大的3.3V字高级别产出)"
#define S10408	"(阿灰色标记方法：打开收藏家SPP的模式，推挽否则)"
#define S10409	"(刚刚节能)"
#define S10410	"(没有更多的反向控制和状态线！因此，任何有益的LPT仿真，获得通过DeviceIoControl。\n第4修订版有20个I/O线，另有17。\n方向是每比特和每引脚可选。)"
#define S10411	"设置和读取回 (&S)"
#define S10412	"扫描短路"
#define S10413	"编号"
#define S10414	"&USB描述符"
#define S10415	"固件日期"
#define S10416	"黯淡的蓝色&LED"
#define S10417	"启动值&ECR（操作模式）:"

#define S10500	"监控"
#define S10501	"数据口（+0） (&P)"
#define S10502	"阅读"
#define S10503	"港口地位（+1） (&S)"
#define S10504	"控制口（+2） (&C)"
#define S10506	"读取字节 (&R)"
#define S10507	"地址 (&A)"
#define S10508	"数据 (&D)"
#define S10509	"收件字节 (&W)"
#define S10512	"FIFO满"
#define S10513	"FIFO空"
#define S10514	"方向寄存器（USB2LPT具体）"

// STRINGTABLE
#define S16 "使用这个非标准端口地址会损坏您的计算机!"
#define S17 "端口地址不是有效16进制数！"
#define S18 "端口地址错误：太短，太长，不能被4整除或占用IDE地址！"
#define S19 "端口地址不能被8整除，无法模拟EPP模式！"
#define S20 "延时时间错误（不得超过1000ms）。"
#define S21 "设置模拟端口地址"
#define S22 "端口地址必须和应用软件中的设置一致。"
#define S23 "这固件不支持USB2LPT特定功能注册！"
#define S24 "无"	// 编号
#define S25 "这是什么"
#define S26 "&Bulk transfer (faster; good for XP and earlier)"
#define S27 "Interrupt transfer otherwise"
#define S28 "Activating bulk transfer renders USB2LPT defective under Windows Vista and newer!\nRemember to reset this bit before upgrading your system.\n\n继续吗？"
#define S29 "The debug register trap hook will be detected by PatchGuard some minutes later, which results in BSoD 0x109 when no countermeasure was taken.\n\n继续吗？"

// 固件更新
#define S32 "固件写入EEPROM的成功。循环发电设备，开始向新的固件。"
#define S33 "未知的错误而写固件的EEPROM。请不要断开装置，然后再试一次。该USB2LPT设备可能无法工作了！"
#define S34 "档案%s是不能接受为固件为USB2LPT设备！"
#define S35 "日期的实际（运行）固件：%s。最新的固件来写：%s。继续吗？"
#define S36 "未知"		// 固件日期（提前释放没有日期内）
#define S37 "固件\0*.iic"	// 过滤器打开文件的标准对话框
#define S38 "完善的功能。"
#define S39 "不正确的功能检测！运行Kurzschlusstest.EXE详情。"
#define S40 "你衡量高级别的所有17线？"
#define S41 "固件低速USB2LPT装置ATmega控制器无法更新这里，但是你可以设置一个旗启动内置的Boot Loader。在电源循环您必须更新使用bootloadHID.EXE。\n\n继续吗？"
#define S42 "This USB2LPT doesn't support firmware update via USB.\n\n(First EEPROM byte = 0x%02X)"

// 监测属性表
#define S48 "SPP(半字节模式)\0双向\0自动转入\0ECP\0EPP\0保留\0测试模式\0配置"
#define S49 "附加的\0保留预定的\0中断请求\0错误\0已连接\0全文完/纸张缺少\0承认\0忙"
#define S50 "转入/接受\0自动送料\0初始化\0选择输入\0中断允许\0数据方向，1=输入)\0不可应用的"
#define S51 "命令 (&O)\0地址 (&A)"	// 改变文字的单选按钮
#define S52 "普通\0半字节模式\0字节模式\0扩展能力端口模式\0增强并行端口模式"

// VERSIONINFO
#define I002	4,2
#define S003	"提供者工作表的属性"		// FileDescription
#define S004	"%d.%02d (%d年%d月);1;2;3;4/"	// FileVersion (awk-processed)
#define S005	"免费软件"			// LegalCopyright
#define S006	"h#s USB转LPT转换器"		// ProductName
#define S007	"内核模式驱动程序"		// FileDescription for .SYS driver

/* INF file strings (processed by makefile)
InstDisk="h#s USB转LPT转换器驱动盘"
DevDesc ="h#s USB转LPT转换器"
SvcDesc ="h#s USB转LPT转换器驱动程序(usb2lpt.sys)"
*/
