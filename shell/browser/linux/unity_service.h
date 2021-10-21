// 版权所有2013年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_LINUX_UNITY_SERVICE_H_
#define SHELL_BROWSER_LINUX_UNITY_SERVICE_H_

namespace unity {

// 返回Unity当前是否正在运行。
bool IsRunning();

// 如果Unity正在运行，则在坞站图标中设置下载计数器。任何值。
// 如果不是0，则显示徽章。
void SetDownloadCount(int count);

// 如果Unity正在运行，则在坞站图标中设置下载进度条。任何。
// 介于0.0和1.0(独占)之间的值显示进度条。
void SetProgressFraction(float percentage);

}  // 命名空间统一性。

#endif  // Shell_Browser_Linux_Unity_SERVICE_H_
