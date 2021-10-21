// 版权所有(C)2016 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_RELAUNCHER_H_
#define SHELL_BROWSER_RELAUNCHER_H_

// 重新启动程序实现了跨平台的主浏览器应用程序重新启动。
// 当浏览器想要重新启动自己时，它不能简单地派生一个新的。
// 从内部处理并执行新浏览器。这就给我们打开了一扇窗。
// 在此期间，两个浏览器应用程序可能同时运行。如果。
// 发生这种情况时，每个人都会有一个不同的Dock图标，即。
// 如果用户希望Dock图标通过以下方式保持不变，则尤其糟糕。
// 从图标的上下文菜单中选择Keep in Dock。
// 
// 重新启动程序通过引入中间件来解决这个问题。
// 进程(“重新启动器”)位于原始浏览器(“父”)和。
// 更换浏览器(“重新启动”)。帮助器可执行文件用于。
// 重新启动进程；因为它是LSUIElement，所以没有Dock。
// 图标，并且根本看不到正在运行的应用程序。家长会。
// 启动一个重新启动程序进程，为它提供管道的“编写器”端。
// 保留的“Reader”结尾。当重新启动时，它将。
// 建立一个等待父级退出的kQueue，然后写入。
// 管子。在从管道中读取数据后，父进程可以自由退出。当。
// 重新启动器通过它的kqueue被通知父进程已经退出，它。
// 继续进行，启动了重新启动的进程。要同步的握手。
// 带重新启动器的父母是避免竞争所必需的：重新启动器。
// 需要确保它监控的是父进程，而不是其他进程。
// 考虑到PID重用，因此父级必须保持足够长的存活时间，以便。
// 重新启动程序以设置其kqueue。

#include "base/command_line.h"

#if defined(OS_WIN)
#include "base/process/process_handle.h"
#endif

namespace content {
struct MainFunctionParams;
}

namespace relauncher {

using CharType = base::CommandLine::CharType;
using StringType = base::CommandLine::StringType;
using StringVector = base::CommandLine::StringVector;

// 属性关联的帮助器应用程序重新启动应用程序。
// 当前在父浏览器进程中作为。
// 重新启动程序进程的可执行文件。|argv|是的argv样式的向量。
// 通常传递给execv的格式的命令行参数。Argv[0]为。
// 还可以找到重新启动的进程的路径。因为重新启动程序的过程。
// 最终将通过启动服务argv[0]启动重新启动的进程。
// 可以是可执行文件的路径名或.app的路径名。
// 捆绑包目录。调用程序应在RelaunchApp返回后不久退出。
// 成功了。成功时返回TRUE，尽管可能会发生一些失败。
// 在此函数返回TRUE之后，例如，如果它们出现在。
// 重新启动程序进程。如果重新启动确实失败，则返回FALSE。
bool RelaunchApp(const StringVector& argv);

// 与RelaunchApp相同，但使用|helper|作为重新启动程序的路径。
// 进程，并允许将其他参数提供给重新启动程序。
// Reluncher_args中的进程。与argv[0]不同，|helper|必须是路径名。
// 可执行文件。给定的帮助器路径必须来自相同版本的。
// Chrome作为运行的父浏览器进程，因为不能保证。
// 来自不同版本的父进程和重新启动程序进程将。
// 能够相互沟通。此变体可用于。
// 从另一个位置重新启动相同版本的Chrome，使用。
// 地点的助手。
bool RelaunchAppWithHelper(const base::FilePath& helper,
                           const StringVector& relauncher_args,
                           const StringVector& argv);

// 从ChromeMain进入重新启动程序进程的入口点。
int RelauncherMain(const content::MainFunctionParams& main_parameters);

namespace internal {

#if defined(OS_POSIX)
// 重新启动程序进程的写入端的“神奇”文件描述符。
// 管道出现在。选择以避免与stdin、stdout和。
// 标准。
extern const int kRelauncherSyncFD;
#endif

// 标识重新启动器进程的“type”参数(“--type=re uncher”)。
extern const CharType* kRelauncherTypeArg;

// 将打算用于重新启动器进程的参数与。
// 那些计划用于重新启动的进程。选择“-”而不是“--”
// 因为CommandLine将“--”解释为“开关结束”，但是。
// 出于许多目的，重新启动程序进程的CommandLine应该解释。
// 用于重新启动进程的参数，以获得正确的设置。
// 用于日志记录和user-data-dir等内容，以防它影响崩溃。
// 报道。
extern const CharType* kRelauncherArgSeparator;

#if defined(OS_WIN)
StringType GetWaitEventName(base::ProcessId pid);

StringType ArgvToCommandLineString(const StringVector& argv);
#endif

// 在重新启动程序过程中，执行必要的同步步骤。
// 通过设置一个kqueue来监视它是否退出，并编写一个。
// 字节发送到管道，然后等待kqueue上的退出通知。
// 如果任何操作失败，它会记录一条消息并立即返回。穿着那些。
// 情况下，可以假设父母出了问题。
// 无论如何，最好的恢复方法是尝试重新启动。
void RelauncherSynchronizeWithParent();

int LaunchProgram(const StringVector& relauncher_args,
                  const StringVector& argv);

}  // 命名空间内部。

}  // 命名空间重新启动器。

#endif  // SHELL_BROWSER_RELAUNCHER_H_
