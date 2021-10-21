// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_GTK_UTIL_H_
#define SHELL_BROWSER_UI_GTK_UTIL_H_

#include <gtk/gtk.h>

class SkBitmap;

namespace gtk_util {

const char* GettextPackage();
const char* GtkGettext(const char* str);

const char* GetCancelLabel();
const char* GetOpenLabel();
const char* GetSaveLabel();
const char* GetOkLabel();
const char* GetNoLabel();
const char* GetYesLabel();

// 将SkBitmap转换并复制到GdkPixbuf。注意：它使用BGRAToRGBA，因此。
// 这是一项昂贵的手术。返回的GdkPixbuf将具有。
// 1，调用者负责在完成后将其取消提取。
GdkPixbuf* GdkPixbufFromSkBitmap(const SkBitmap& bitmap);

}  // 命名空间gtk_util。

#endif  // Shell_Browser_UI_GTK_UTIL_H_
