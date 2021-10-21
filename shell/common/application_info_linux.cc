// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/common/application_info.h"

#include <gio/gdesktopappinfo.h>
#include <gio/gio.h>

#include <string>

#include "base/environment.h"
#include "base/logging.h"
#include "electron/electron_version.h"
#include "shell/common/platform_util.h"
#include "ui/gtk/gtk_util.h"

namespace {

GDesktopAppInfo* get_desktop_app_info() {
  GDesktopAppInfo* ret = nullptr;

  std::string desktop_id;
  if (platform_util::GetDesktopName(&desktop_id))
    ret = g_desktop_app_info_new(desktop_id.c_str());

  return ret;
}

}  // 命名空间。

namespace electron {

std::string GetApplicationName() {
  // 尝试#1：app.setName()中设置的字符串。
  std::string ret = OverriddenApplicationName();

  // 尝试#2：来自.ktop文件的[Desktop]部分的‘name’条目。
  if (ret.empty()) {
    GDesktopAppInfo* info = get_desktop_app_info();
    if (info != nullptr) {
      char* str = g_desktop_app_info_get_string(info, "Name");
      g_clear_object(&info);
      if (str != nullptr)
        ret = str;
      g_clear_pointer(&str, g_free);
    }
  }

  // 尝试3：电子的名字。
  if (ret.empty()) {
    ret = ELECTRON_PRODUCT_NAME;
  }

  return ret;
}

std::string GetApplicationVersion() {
  std::string ret;

  // 确保ELEMENT_PRODUCT_NAME和GetApplicationVersion匹配。
  if (GetApplicationName() == ELECTRON_PRODUCT_NAME)
    ret = ELECTRON_VERSION_STRING;

  // 尝试使用app.setVersion()中设置的字符串。
  if (ret.empty())
    ret = OverriddenApplicationVersion();

  // 没有已知的版本号；返回一些安全的后备。
  if (ret.empty()) {
    LOG(WARNING) << "No version found. Was app.setVersion() called?";
    ret = "0.0";
  }

  return ret;
}

}  // 命名空间电子
