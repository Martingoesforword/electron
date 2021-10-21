// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/common/options_switches.h"

namespace electron {

namespace options {

const char kTitle[] = "title";
const char kIcon[] = "icon";
const char kFrame[] = "frame";
const char kShow[] = "show";
const char kCenter[] = "center";
const char kX[] = "x";
const char kY[] = "y";
const char kWidth[] = "width";
const char kHeight[] = "height";
const char kMinWidth[] = "minWidth";
const char kMinHeight[] = "minHeight";
const char kMaxWidth[] = "maxWidth";
const char kMaxHeight[] = "maxHeight";
const char kResizable[] = "resizable";
const char kMovable[] = "movable";
const char kMinimizable[] = "minimizable";
const char kMaximizable[] = "maximizable";
const char kFullScreenable[] = "fullscreenable";
const char kClosable[] = "closable";
const char kFullscreen[] = "fullscreen";
const char kTrafficLightPosition[] = "trafficLightPosition";
const char kRoundedCorners[] = "roundedCorners";

// 分别用作窗口的主题颜色和符号颜色的颜色。
// 控件覆盖(如果在Windows上启用)。
const char kOverlayButtonColor[] = "color";
const char kOverlaySymbolColor[] = "symbolColor";

// 窗口是否应显示在任务栏中。
const char kSkipTaskbar[] = "skipTaskbar";

// 从Kiosk模式开始，说明请参见Opera页面：
// Http://www.opera.com/support/mastering/kiosk/。
const char kKiosk[] = "kiosk";

const char kSimpleFullScreen[] = "simpleFullscreen";

// 使窗口位于所有其他窗口的顶部。
const char kAlwaysOnTop[] = "alwaysOnTop";

// 启用NSView以接受第一个鼠标事件。
const char kAcceptFirstMouse[] = "acceptFirstMouse";

// 窗口大小是否应包括窗口框架。
const char kUseContentSize[] = "useContentSize";

// 窗口缩放是否应为页面宽度。
const char kZoomToPageWidth[] = "zoomToPageWidth";

// 是否启用始终全屏显示标题文本。
const char kFullscreenWindowTitle[] = "fullscreenWindowTitle";

// 请求的窗口标题栏样式。
const char kTitleBarStyle[] = "titleBarStyle";

// 如果在MacOS上启用了本机选项卡，则窗口的跳转标识符。
const char kTabbingIdentifier[] = "tabbingIdentifier";

// 除非按下“Alt”，否则菜单栏将隐藏。
const char kAutoHideMenuBar[] = "autoHideMenuBar";

// 使窗口可以调整到大于屏幕的大小。
const char kEnableLargerThanScreen[] = "enableLargerThanScreen";

// 强制在Linux上使用黑暗主题。
const char kDarkTheme[] = "darkTheme";

// 窗口是否应该是透明的。
const char kTransparent[] = "transparent";

// 窗口类型提示。
const char kType[] = "type";

// 禁用自动隐藏光标。
const char kDisableAutoHideCursor[] = "disableAutoHideCursor";

// 使用MacOS的标准窗口，而不是纹理窗口。
const char kStandardWindow[] = "standardWindow";

// 默认浏览器窗口背景颜色。
const char kBackgroundColor[] = "backgroundColor";

// 窗口是否应该有阴影。
const char kHasShadow[] = "hasShadow";

// 浏览器窗口不透明。
const char kOpacity[] = "opacity";

// 窗口是否可以激活。
const char kFocusable[] = "focusable";

// Web首选项。
const char kWebPreferences[] = "webPreferences";

// 向浏览器窗口添加振动效果。
const char kVibrancyType[] = "vibrancy";

// 指定材质外观应如何反映窗口活动状态。
// MacOS操作系统。
const char kVisualEffectState[] = "visualEffectState";

// 哪个页面应该缩放的因素。
const char kZoomFactor[] = "zoomFactor";

// 将由来宾WebContents在其他脚本之前加载的脚本。
const char kPreloadScript[] = "preload";

const char kPreloadScripts[] = "preloadScripts";

// 比如--preload，但是传递的参数是一个URL。
const char kPreloadURL[] = "preloadURL";

// 启用节点集成。
const char kNodeIntegration[] = "nodeIntegration";

// 启用电子API和预加载脚本的上下文隔离。
const char kContextIsolation[] = "contextIsolation";

// Web运行时功能。
const char kExperimentalFeatures[] = "experimentalFeatures";

// 打开窗口的ID。
const char kOpenerID[] = "openerId";

// 启用橡皮筋效果。
const char kScrollBounce[] = "scrollBounce";

// 启用闪烁功能。
const char kEnableBlinkFeatures[] = "enableBlinkFeatures";

// 禁用闪烁功能。
const char kDisableBlinkFeatures[] = "disableBlinkFeatures";

// 在WebWorker中启用节点集成。
const char kNodeIntegrationInWorker[] = "nodeIntegrationInWorker";

// 启用Web视图标记。
const char kWebviewTag[] = "webviewTag";

const char kNativeWindowOpen[] = "nativeWindowOpen";

const char kCustomArgs[] = "additionalArguments";

const char kPlugins[] = "plugins";

const char kSandbox[] = "sandbox";

const char kWebSecurity[] = "webSecurity";

const char kAllowRunningInsecureContent[] = "allowRunningInsecureContent";

const char kOffscreen[] = "offscreen";

const char kNodeIntegrationInSubFrames[] = "nodeIntegrationInSubFrames";

// 激活HTML FullScreen API时禁用窗口大小调整。
const char kDisableHtmlFullscreenWindowResize[] =
    "disableHtmlFullscreenWindowResize";

// 启用JavaScript支持。
const char kJavaScript[] = "javascript";

// 启用映像支持。
const char kImages[] = "images";

// 图像动画策略。
const char kImageAnimationPolicy[] = "imageAnimationPolicy";

// 使TextArea元素可调整大小。
const char kTextAreasAreResizable[] = "textAreasAreResizable";

// 启用WebGL支持。
const char kWebGL[] = "webgl";

// 将文件或链接拖放到页面上是否会导致。
// 导航。
const char kNavigateOnDragDrop[] = "navigateOnDragDrop";

const char kHiddenPage[] = "hiddenPage";

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
const char kSpellcheck[] = "spellcheck";
#endif

const char kEnableWebSQL[] = "enableWebSQL";

const char kEnablePreferredSizeMode[] = "enablePreferredSizeMode";

const char ktitleBarOverlay[] = "titleBarOverlay";

}  // 命名空间选项。

namespace switches {

// 启用铬沙箱。
const char kEnableSandbox[] = "enable-sandbox";

// PPapi闪存路径。
const char kPpapiFlashPath[] = "ppapi-flash-path";

// PPapi Flash版本。
const char kPpapiFlashVersion[] = "ppapi-flash-version";

// 禁用HTTP缓存。
const char kDisableHttpCache[] = "disable-http-cache";

// 标准方案列表。
const char kStandardSchemes[] = "standard-schemes";

// 注册计划以处理服务人员。
const char kServiceWorkerSchemes[] = "service-worker-schemes";

// 将方案注册为安全方案。
const char kSecureSchemes[] = "secure-schemes";

// 将方案注册为绕过CSP。
const char kBypassCSPSchemes[] = "bypasscsp-schemes";

// 将方案注册为支持FETCH API。
const char kFetchSchemes[] = "fetch-schemes";

// 将方案注册为启用CORS。
const char kCORSSchemes[] = "cors-schemes";

// 将方案注册为流响应。
const char kStreamingSchemes[] = "streaming-schemes";

// 浏览器进程应用程序型号ID。
const char kAppUserModelId[] = "app-user-model-id";

// 应用程序路径
const char kAppPath[] = "app-path";

// 命令行可切换选项的版本。
const char kScrollBounce[] = "scroll-bounce";

// 命令开关传递给渲染器进程以控制节点集成。
const char kNodeIntegrationInWorker[] = "node-integration-in-worker";

// 广域选项。
// Widevine CDM二进制文件的路径。
const char kWidevineCdmPath[] = "widevine-cdm-path";
// 威德文清洁发展机制版本。
const char kWidevineCdmVersion[] = "widevine-cdm-version";

// 强制磁盘缓存使用的最大磁盘空间(以字节为单位)。
const char kDiskCacheSize[] = "disk-cache-size";

// 忽略每台主机6个连接的限制。
const char kIgnoreConnectionsLimit[] = "ignore-connections-limit";

// 包含启用了集成身份验证的服务器的白名单。
const char kAuthServerWhitelist[] = "auth-server-whitelist";

// 包含允许Kerberos委派的服务器的白名单。
const char kAuthNegotiateDelegateWhitelist[] =
    "auth-negotiate-delegate-whitelist";

// 如果设置，请将端口包括在生成的Kerberos SPN中。
const char kEnableAuthNegotiatePort[] = "enable-auth-negotiate-port";

// 如果设置，则对POSIX平台禁用NTLM v2。
const char kDisableNTLMv2[] = "disable-ntlm-v2";

const char kGlobalCrashKeys[] = "global-crash-keys";

const char kEnableWebSQL[] = "enable-websql";

}  // 命名空间开关。

}  // 命名空间电子
