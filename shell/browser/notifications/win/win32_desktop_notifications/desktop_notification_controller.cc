// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "shell/browser/notifications/win/win32_desktop_notifications/desktop_notification_controller.h"

#include <windowsx.h>
#include <utility>

#include "base/check.h"
#include "shell/browser/notifications/win/win32_desktop_notifications/common.h"
#include "shell/browser/notifications/win/win32_desktop_notifications/toast.h"

namespace electron {

HBITMAP CopyBitmap(HBITMAP bitmap) {
  HBITMAP ret = NULL;

  BITMAP bm;
  if (bitmap && GetObject(bitmap, sizeof(bm), &bm)) {
    HDC hdc_screen = GetDC(NULL);
    ret = CreateCompatibleBitmap(hdc_screen, bm.bmWidth, bm.bmHeight);
    ReleaseDC(NULL, hdc_screen);

    if (ret) {
      HDC hdc_src = CreateCompatibleDC(NULL);
      HDC hdc_dst = CreateCompatibleDC(NULL);
      SelectBitmap(hdc_src, bitmap);
      SelectBitmap(hdc_dst, ret);
      BitBlt(hdc_dst, 0, 0, bm.bmWidth, bm.bmHeight, hdc_src, 0, 0, SRCCOPY);
      DeleteDC(hdc_dst);
      DeleteDC(hdc_src);
    }
  }

  return ret;
}

const TCHAR DesktopNotificationController::class_name_[] =
    TEXT("DesktopNotificationController");

HINSTANCE DesktopNotificationController::RegisterWndClasses() {
  // 我们保留了一个静态的‘module’变量，它具有双重用途：
  // 1.存储注册窗口类的HINSTANCE。
  // 可以传递给`CreateWindow`。
  // 2.指示我们是否已尝试注册，以便。
  // 我们不会重复两次(即使注册失败，我们也不会重试，
  // 因为这是没有意义的)。
  static HMODULE module = NULL;

  if (!module) {
    if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                              GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                          reinterpret_cast<LPCWSTR>(&RegisterWndClasses),
                          &module)) {
      Toast::Register(module);

      WNDCLASSEX wc = {sizeof(wc)};
      wc.lpfnWndProc = &WndProc;
      wc.lpszClassName = class_name_;
      wc.cbWndExtra = sizeof(DesktopNotificationController*);
      wc.hInstance = module;

      RegisterClassEx(&wc);
    }
  }

  return module;
}

DesktopNotificationController::DesktopNotificationController(
    unsigned maximum_toasts) {
  instances_.reserve(maximum_toasts);
}

DesktopNotificationController::~DesktopNotificationController() {
  for (auto&& inst : instances_)
    DestroyToast(&inst);
  if (hwnd_controller_)
    DestroyWindow(hwnd_controller_);
  ClearAssets();
}

LRESULT CALLBACK DesktopNotificationController::WndProc(HWND hwnd,
                                                        UINT message,
                                                        WPARAM wparam,
                                                        LPARAM lparam) {
  switch (message) {
    case WM_CREATE: {
      auto*& cs = reinterpret_cast<const CREATESTRUCT*&>(lparam);
      SetWindowLongPtr(hwnd, 0, (LONG_PTR)cs->lpCreateParams);
    } break;

    case WM_TIMER:
      if (wparam == TimerID_Animate) {
        Get(hwnd)->AnimateAll();
      }
      return 0;

    case WM_DISPLAYCHANGE: {
      auto* inst = Get(hwnd);
      inst->ClearAssets();
      inst->AnimateAll();
    } break;

    case WM_SETTINGCHANGE:
      if (wparam == SPI_SETWORKAREA) {
        Get(hwnd)->AnimateAll();
      }
      break;
  }

  return DefWindowProc(hwnd, message, wparam, lparam);
}

void DesktopNotificationController::StartAnimation() {
  DCHECK(hwnd_controller_);

  if (!is_animating_ && hwnd_controller_) {
    // 注意：15ms比我们需要的60fps要短，但是因为。
    // 计时器不准确，我们必须要求更高的帧速率。
    // 至少要拿到60。

    SetTimer(hwnd_controller_, TimerID_Animate, 15, nullptr);
    is_animating_ = true;
  }
}

HFONT DesktopNotificationController::GetCaptionFont() {
  InitializeFonts();
  return caption_font_;
}

HFONT DesktopNotificationController::GetBodyFont() {
  InitializeFonts();
  return body_font_;
}

void DesktopNotificationController::InitializeFonts() {
  if (!body_font_) {
    NONCLIENTMETRICS metrics = {sizeof(metrics)};
    if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &metrics, 0)) {
      auto base_height = metrics.lfMessageFont.lfHeight;

      HDC hdc = GetDC(NULL);
      auto base_dpi_y = GetDeviceCaps(hdc, LOGPIXELSY);
      ReleaseDC(NULL, hdc);

      ScreenMetrics scr;

      metrics.lfMessageFont.lfHeight =
          (LONG)ScaleForDpi(base_height * 1.1f, scr.dpi_y, base_dpi_y);
      body_font_ = CreateFontIndirect(&metrics.lfMessageFont);

      if (caption_font_)
        DeleteFont(caption_font_);
      metrics.lfMessageFont.lfHeight =
          (LONG)ScaleForDpi(base_height * 1.4f, scr.dpi_y, base_dpi_y);
      caption_font_ = CreateFontIndirect(&metrics.lfMessageFont);
    }
  }
}

void DesktopNotificationController::ClearAssets() {
  if (caption_font_) {
    DeleteFont(caption_font_);
    caption_font_ = NULL;
  }
  if (body_font_) {
    DeleteFont(body_font_);
    body_font_ = NULL;
  }
}

void DesktopNotificationController::AnimateAll() {
  // 注意：此功能可刷新所有吐司的位置和大小。
  // 所有目前的状况。动画时间只是其中一个变量。
  // 影响着他们。屏幕分辨率是另一个原因。

  bool keep_animating = false;

  if (!instances_.empty()) {
    RECT work_area;
    if (SystemParametersInfo(SPI_GETWORKAREA, 0, &work_area, 0)) {
      ScreenMetrics metrics;
      POINT origin = {work_area.right,
                      work_area.bottom - metrics.Y(toast_margin_)};

      auto* hdwp = BeginDeferWindowPos(static_cast<int>(instances_.size()));

      for (auto&& inst : instances_) {
        if (!inst.hwnd)
          continue;

        auto* notification = Toast::Get(inst.hwnd);
        hdwp = notification->Animate(hdwp, origin);
        if (!hdwp)
          break;
        keep_animating |= notification->IsAnimationActive();
      }

      if (hdwp)
        EndDeferWindowPos(hdwp);
    }
  }

  if (!keep_animating) {
    DCHECK(hwnd_controller_);
    if (hwnd_controller_)
      KillTimer(hwnd_controller_, TimerID_Animate);
    is_animating_ = false;
  }

  // 清除已取消的通知并折叠堆栈。
  // 突出显示的项目。
  if (!instances_.empty()) {
    auto is_alive = [](ToastInstance& inst) {
      return inst.hwnd && IsWindowVisible(inst.hwnd);
    };

    auto is_highlighted = [](ToastInstance& inst) {
      return inst.hwnd && Toast::Get(inst.hwnd)->IsHighlighted();
    };

    for (auto it = instances_.begin();; ++it) {
      // 查找下一个突出显示的项目。
      auto it2 = find_if(it, instances_.end(), is_highlighted);

      // 折叠突出显示项目前面的堆栈。
      it = stable_partition(it, it2, is_alive);

      // 清除已死的物品。
      for_each(it, it2, [this](auto&& inst) { DestroyToast(&inst); });

      if (it2 == instances_.end()) {
        instances_.erase(it, it2);
        break;
      }

      it = move(it2);
    }
  }

  // 设置新的烤面包位置。
  if (!instances_.empty()) {
    ScreenMetrics metrics;
    auto margin = metrics.Y(toast_margin_);

    int target_pos = 0;
    for (auto&& inst : instances_) {
      if (inst.hwnd) {
        auto* toast = Toast::Get(inst.hwnd);

        if (toast->IsHighlighted())
          target_pos = toast->GetVerticalPosition();
        else
          toast->SetVerticalPosition(target_pos);

        target_pos += toast->GetHeight() + margin;
      }
    }
  }

  // 从队列中创建新的祝酒词。
  CheckQueue();
}

DesktopNotificationController::Notification
DesktopNotificationController::AddNotification(std::u16string caption,
                                               std::u16string body_text,
                                               HBITMAP image) {
  auto data = std::make_shared<NotificationData>();
  data->controller = this;
  data->caption = move(caption);
  data->body_text = move(body_text);
  data->image = CopyBitmap(image);

  // 将新通知入队。
  Notification ret{*queue_.insert(queue_.end(), move(data))};
  CheckQueue();
  return ret;
}

void DesktopNotificationController::CloseNotification(
    const Notification& notification) {
  // 将其从队列中删除。
  auto it = find(queue_.begin(), queue_.end(), notification.data_);
  if (it != queue_.end()) {
    (*it)->controller = nullptr;
    queue_.erase(it);
    this->OnNotificationClosed(notification);
    return;
  }

  // 取消活动吐司。
  auto* hwnd = GetToast(notification.data_.get());
  if (hwnd) {
    auto* toast = Toast::Get(hwnd);
    toast->Dismiss();
  }
}

void DesktopNotificationController::CheckQueue() {
  while (instances_.size() < instances_.capacity() && !queue_.empty()) {
    CreateToast(move(queue_.front()));
    queue_.pop_front();
  }
}

void DesktopNotificationController::CreateToast(
    std::shared_ptr<NotificationData>&& data) {
  auto* hinstance = RegisterWndClasses();
  auto* hwnd = Toast::Create(hinstance, data);
  if (hwnd) {
    int toast_pos = 0;
    if (!instances_.empty()) {
      auto& item = instances_.back();
      DCHECK(item.hwnd);

      ScreenMetrics scr;
      auto* toast = Toast::Get(item.hwnd);
      toast_pos = toast->GetVerticalPosition() + toast->GetHeight() +
                  scr.Y(toast_margin_);
    }

    instances_.push_back({hwnd, std::move(data)});

    if (!hwnd_controller_) {
      // 注意：我们不能使用仅消息窗口，因为我们需要。
      // 接收系统通知。
      hwnd_controller_ = CreateWindow(class_name_, nullptr, 0, 0, 0, 0, 0, NULL,
                                      NULL, hinstance, this);
    }

    auto* toast = Toast::Get(hwnd);
    toast->PopUp(toast_pos);
  }
}

HWND DesktopNotificationController::GetToast(
    const NotificationData* data) const {
  auto it =
      find_if(instances_.cbegin(), instances_.cend(), [data](auto&& inst) {
        if (!inst.hwnd)
          return false;
        auto toast = Toast::Get(inst.hwnd);
        return data == toast->GetNotification().get();
      });

  return (it != instances_.cend()) ? it->hwnd : NULL;
}

void DesktopNotificationController::DestroyToast(ToastInstance* inst) {
  if (inst->hwnd) {
    auto data = Toast::Get(inst->hwnd)->GetNotification();

    DestroyWindow(inst->hwnd);
    inst->hwnd = NULL;

    Notification notification(data);
    OnNotificationClosed(notification);
  }
}

DesktopNotificationController::Notification::Notification() = default;
DesktopNotificationController::Notification::Notification(
    const DesktopNotificationController::Notification&) = default;

DesktopNotificationController::Notification::Notification(
    const std::shared_ptr<NotificationData>& data)
    : data_(data) {
  DCHECK(data != nullptr);
}

DesktopNotificationController::Notification::~Notification() = default;

bool DesktopNotificationController::Notification::operator==(
    const Notification& other) const {
  return data_ == other.data_;
}

void DesktopNotificationController::Notification::Close() {
  // 当不指向有效实例时，没有必要调用此函数。
  DCHECK(data_);

  if (data_->controller)
    data_->controller->CloseNotification(*this);
}

void DesktopNotificationController::Notification::Set(std::u16string caption,
                                                      std::u16string body_text,
                                                      HBITMAP image) {
  // 当不指向有效实例时，没有必要调用此函数。
  DCHECK(data_);

  // 通知已关闭时不执行任何操作。
  if (!data_->controller)
    return;

  if (data_->image)
    DeleteBitmap(data_->image);

  data_->caption = move(caption);
  data_->body_text = move(body_text);
  data_->image = CopyBitmap(image);

  auto* hwnd = data_->controller->GetToast(data_.get());
  if (hwnd) {
    auto* toast = Toast::Get(hwnd);
    toast->ResetContents();
  }

  // 内容的更改会影响所有吐司的大小和位置。
  data_->controller->StartAnimation();
}

DesktopNotificationController::ToastInstance::ToastInstance(
    HWND hwnd,
    std::shared_ptr<NotificationData> data) {
  this->hwnd = hwnd;
  this->data = std::move(data);
}
DesktopNotificationController::ToastInstance::~ToastInstance() = default;
DesktopNotificationController::ToastInstance::ToastInstance(ToastInstance&&) =
    default;

}  // 命名空间电子
