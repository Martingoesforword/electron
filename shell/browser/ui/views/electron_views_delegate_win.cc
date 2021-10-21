// 版权所有(C)2017年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证铬档案里找到的。

#include "shell/browser/ui/views/electron_views_delegate.h"

#include <dwmapi.h>
#include <shellapi.h>

#include <utility>

#include "base/bind.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"

namespace {

bool MonitorHasAutohideTaskbarForEdge(UINT edge, HMONITOR monitor) {
  APPBARDATA taskbar_data = {sizeof(APPBARDATA), NULL, 0, edge};
  taskbar_data.hWnd = ::GetForegroundWindow();

  // MSDN记录了一个ABM_GETAUTOHIDEBAREX，据说它需要一个监视器。
  // RECT并返回该监视器上的自动隐藏条。这听起来很不错。
  // 多监视器系统的想法。不幸的是，它似乎在。
  // 至少在某些情况下(错误地返回NULL)，并且几乎没有。
  // 在线文档或使用它的其他示例代码，这些文档或示例代码建议以下方法。
  // 解决这个问题。我们的工作如下：
  // 1.使用ABM_GETAUTOHIDEBAR消息。如果它起作用，即返回有效的。
  // 窗户，我们完事了。
  // 2.如果ABM_GETAUTOHIDEBAR消息不起作用，我们查询自动隐藏。
  // 任务栏的状态，然后检索其位置。该调用返回。
  // 任务栏所在的边缘。如果它与我们的边缘匹配。
  // 都在找，我们就完了。
  // 注意：此调用旋转嵌套的Run循环。
  HWND taskbar = reinterpret_cast<HWND>(
      SHAppBarMessage(ABM_GETAUTOHIDEBAR, &taskbar_data));
  if (!::IsWindow(taskbar)) {
    APPBARDATA taskbar_data = {sizeof(APPBARDATA), 0, 0, 0};
    unsigned int taskbar_state = SHAppBarMessage(ABM_GETSTATE, &taskbar_data);
    if (!(taskbar_state & ABS_AUTOHIDE))
      return false;

    taskbar_data.hWnd = ::FindWindow(L"Shell_TrayWnd", NULL);
    if (!::IsWindow(taskbar_data.hWnd))
      return false;

    SHAppBarMessage(ABM_GETTASKBARPOS, &taskbar_data);
    if (taskbar_data.uEdge == edge)
      taskbar = taskbar_data.hWnd;
  }

  // 这里存在潜在的竞争条件：
  // 1.最大化的镀铬窗口全屏显示。
  // 2.切换回最大化。
  // 3.在此过程中，窗口会收到一条WM_NCCACLSIZE消息，要求我们。
  // 获取自动隐藏状态。
  // 4.调用工作线程。它调用API来获取自动隐藏。
  // 州政府。在Windows 7之前的Windows版本上，任务栏可以。
  // 轻松地总是处于领先或不领先的地位。
  // 这意味着我们只想查找最上面的任务栏。
  // 位设置。但是，这在以下情况下会导致问题：
  // 主线程仍在从全屏切换的过程中。
  // 在这种情况下，任务栏可能尚未设置最顶位。
  // 5.主线程恢复并且不为任务栏留出空间，并且。
  // 因此，它在悬停时不会弹出。
  // 
  // 要解决上述第4点，最好不要检查WS_EX_TOPMOST。
  // 任务栏上的窗口样式，从最上面的Windows 7开始。
  // 样式始终是固定的。我们不再支持XP和Vista。
  if (::IsWindow(taskbar)) {
    if (MonitorFromWindow(taskbar, MONITOR_DEFAULTTONEAREST) == monitor)
      return true;
    // 在某些情况下，例如当自动隐藏任务栏位于。
    // 辅助监视器，则上面的Monitor/FromWindow调用无法返回。
    // 正确的监视器任务栏已打开。我们退回到Monitor FromPoint。
    // 在这种情况下的光标位置，这似乎工作得很好。
    POINT cursor_pos = {0};
    GetCursorPos(&cursor_pos);
    if (MonitorFromPoint(cursor_pos, MONITOR_DEFAULTTONEAREST) == monitor)
      return true;
  }
  return false;
}

int GetAppbarAutohideEdgesOnWorkerThread(HMONITOR monitor) {
  DCHECK(monitor);

  int edges = 0;
  if (MonitorHasAutohideTaskbarForEdge(ABE_LEFT, monitor))
    edges |= views::ViewsDelegate::EDGE_LEFT;
  if (MonitorHasAutohideTaskbarForEdge(ABE_TOP, monitor))
    edges |= views::ViewsDelegate::EDGE_TOP;
  if (MonitorHasAutohideTaskbarForEdge(ABE_RIGHT, monitor))
    edges |= views::ViewsDelegate::EDGE_RIGHT;
  if (MonitorHasAutohideTaskbarForEdge(ABE_BOTTOM, monitor))
    edges |= views::ViewsDelegate::EDGE_BOTTOM;
  return edges;
}

}  // 命名空间。

namespace electron {

HICON ViewsDelegate::GetDefaultWindowIcon() const {
  // 使用当前exe的图标作为默认窗口图标。
  return LoadIcon(GetModuleHandle(NULL),
                  MAKEINTRESOURCE(1 /* IDR_大型机。*/));
}

HICON ViewsDelegate::GetSmallWindowIcon() const {
  return GetDefaultWindowIcon();
}

bool ViewsDelegate::IsWindowInMetro(gfx::NativeWindow window) const {
  return false;
}

int ViewsDelegate::GetAppbarAutohideEdges(HMONITOR monitor,
                                          base::OnceClosure callback) {
  // 使用edge_Bottom初始化贴图。这一点很重要，就好像我们返回一个。
  // 初始值为0(无自动隐藏边)，然后我们将全屏显示。
  // Windows将自动从生成的应用程序栏中删除WS_EX_TOPMOST。
  // 在我们看来，没有自动隐藏的边缘。通过返回至少一条边。
  // 在我们弄清楚真正的自动隐藏之前，我们最初不会全屏显示。
  // 边缘。
  if (!appbar_autohide_edge_map_.count(monitor))
    appbar_autohide_edge_map_[monitor] = EDGE_BOTTOM;

  // 我们使用SHAppBarMessage API来获取任务栏自动隐藏状态。本接口。
  // 旋转可能导致调用者重新进入的模式循环。为了避免。
  // 我们在工作线程中检索任务栏状态。
  if (monitor && !in_autohide_edges_callback_) {
    // TODO(Robliao)：一旦支持，就用.WithCOM()注释此任务。
    // Https://crbug.com/662122。
    base::ThreadPool::PostTaskAndReplyWithResult(
        FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_BLOCKING},
        base::BindOnce(&GetAppbarAutohideEdgesOnWorkerThread, monitor),
        base::BindOnce(&ViewsDelegate::OnGotAppbarAutohideEdges,
                       weak_factory_.GetWeakPtr(), std::move(callback), monitor,
                       appbar_autohide_edge_map_[monitor]));
  }
  return appbar_autohide_edge_map_[monitor];
}

void ViewsDelegate::OnGotAppbarAutohideEdges(base::OnceClosure callback,
                                             HMONITOR monitor,
                                             int returned_edges,
                                             int edges) {
  appbar_autohide_edge_map_[monitor] = edges;
  if (returned_edges == edges)
    return;

  base::AutoReset<bool> in_callback_setter(&in_autohide_edges_callback_, true);
  std::move(callback).Run();
}

}  // 命名空间电子
