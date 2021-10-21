// 版权所有(C)2019 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_API_ELECTRON_API_NATIVE_THEME_H_
#define SHELL_BROWSER_API_ELECTRON_API_NATIVE_THEME_H_

#include "gin/handle.h"
#include "gin/wrappable.h"
#include "shell/browser/event_emitter_mixin.h"
#include "ui/native_theme/native_theme.h"
#include "ui/native_theme/native_theme_observer.h"

namespace electron {

namespace api {

class NativeTheme : public gin::Wrappable<NativeTheme>,
                    public gin_helper::EventEmitterMixin<NativeTheme>,
                    public ui::NativeThemeObserver {
 public:
  static gin::Handle<NativeTheme> Create(v8::Isolate* isolate);

  // 杜松子酒：：可包装的。
  static gin::WrapperInfo kWrapperInfo;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

 protected:
  NativeTheme(v8::Isolate* isolate,
              ui::NativeTheme* ui_theme,
              ui::NativeTheme* web_theme);
  ~NativeTheme() override;

  void SetThemeSource(ui::NativeTheme::ThemeSource override);
#if defined(OS_MAC)
  void UpdateMacOSAppearanceForOverrideValue(
      ui::NativeTheme::ThemeSource override);
#endif
  ui::NativeTheme::ThemeSource GetThemeSource() const;
  bool ShouldUseDarkColors();
  bool ShouldUseHighContrastColors();
  bool ShouldUseInvertedColorScheme();

  // UI：：NativeThemeViewer：
  void OnNativeThemeUpdated(ui::NativeTheme* theme) override;
  void OnNativeThemeUpdatedOnUI();

 private:
  ui::NativeTheme* ui_theme_;
  ui::NativeTheme* web_theme_;

  DISALLOW_COPY_AND_ASSIGN(NativeTheme);
};

}  // 命名空间API。

}  // 命名空间电子。

namespace gin {

template <>
struct Converter<ui::NativeTheme::ThemeSource> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const ui::NativeTheme::ThemeSource& val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     ui::NativeTheme::ThemeSource* out);
};

}  // 命名空间杜松子酒。

#endif  // SHELL_BROWSER_API_ELECTRON_API_NATIVE_THEME_H_
