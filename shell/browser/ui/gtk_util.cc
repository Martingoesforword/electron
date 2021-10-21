// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/ui/gtk_util.h"

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <stdint.h>

#include <string>

#include "base/no_destructor.h"
#include "base/strings/string_number_conversions.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkUnPreMultiply.h"
#include "ui/gtk/gtk_compat.h"  // 点名检查。

namespace gtk_util {

// 以下实用程序是从。
// Https://source.chromium.org/chromium/chromium/src/+/main:ui/gtk/select_file_dialog_impl_gtk.cc；l=43-74。

const char* GettextPackage() {
  static base::NoDestructor<std::string> gettext_package(
      "gtk" + base::NumberToString(gtk::GtkVersion().components()[0]) + "0");
  return gettext_package->c_str();
}

const char* GtkGettext(const char* str) {
  return g_dgettext(GettextPackage(), str);
}

const char* GetCancelLabel() {
  if (!gtk::GtkCheckVersion(4))
    return "gtk-cancel";  // 在GTK3中，这是GTK_STOCK_CANCEL。
  static const char* cancel = GtkGettext("_Cancel");
  return cancel;
}

const char* GetOpenLabel() {
  if (!gtk::GtkCheckVersion(4))
    return "gtk-open";  // 在GTK3中，这是GTK_STOCK_OPEN。
  static const char* open = GtkGettext("_Open");
  return open;
}

const char* GetSaveLabel() {
  if (!gtk::GtkCheckVersion(4))
    return "gtk-save";  // 在GTK3中，这是gtk_stock_save。
  static const char* save = GtkGettext("_Save");
  return save;
}

const char* GetOkLabel() {
  if (!gtk::GtkCheckVersion(4))
    return "gtk-ok";  // 在GTK3中，这是GTK_STOCK_OK。
  static const char* ok = GtkGettext("_Ok");
  return ok;
}

const char* GetNoLabel() {
  if (!gtk::GtkCheckVersion(4))
    return "gtk-no";  // 在GTK3中，这是GTK_STOCK_NO。
  static const char* no = GtkGettext("_No");
  return no;
}

const char* GetYesLabel() {
  if (!gtk::GtkCheckVersion(4))
    return "gtk-yes";  // 在GTK3中，这是GTK_STOCK_YES。
  static const char* yes = GtkGettext("_Yes");
  return yes;
}

GdkPixbuf* GdkPixbufFromSkBitmap(const SkBitmap& bitmap) {
  if (bitmap.isNull())
    return nullptr;

  int width = bitmap.width();
  int height = bitmap.height();

  GdkPixbuf* pixbuf =
      gdk_pixbuf_new(GDK_COLORSPACE_RGB,  // 唯一支持的色彩空间GTK。
                     TRUE,                // 有一个Alpha通道。
                     8, width, height);

  // Skitmap是预乘的，我们需要取消预乘。
  const int kBytesPerPixel = 4;
  uint8_t* divided = gdk_pixbuf_get_pixels(pixbuf);

  for (int y = 0, i = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      uint32_t pixel = bitmap.getAddr32(0, y)[x];

      int alpha = SkColorGetA(pixel);
      if (alpha != 0 && alpha != 255) {
        SkColor unmultiplied = SkUnPreMultiply::PMColorToColor(pixel);
        divided[i + 0] = SkColorGetR(unmultiplied);
        divided[i + 1] = SkColorGetG(unmultiplied);
        divided[i + 2] = SkColorGetB(unmultiplied);
        divided[i + 3] = alpha;
      } else {
        divided[i + 0] = SkColorGetR(pixel);
        divided[i + 1] = SkColorGetG(pixel);
        divided[i + 2] = SkColorGetB(pixel);
        divided[i + 3] = alpha;
      }
      i += kBytesPerPixel;
    }
  }

  return pixbuf;
}

}  // 命名空间gtk_util
