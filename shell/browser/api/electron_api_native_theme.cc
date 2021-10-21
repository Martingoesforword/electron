// 版权所有(C)2019 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/api/electron_api_native_theme.h"

#include <string>

#include "base/task/post_task.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "gin/handle.h"
#include "shell/common/gin_converters/std_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"
#include "ui/gfx/color_utils.h"
#include "ui/native_theme/native_theme.h"

namespace electron {

namespace api {

gin::WrapperInfo NativeTheme::kWrapperInfo = {gin::kEmbedderNativeGin};

NativeTheme::NativeTheme(v8::Isolate* isolate,
                         ui::NativeTheme* ui_theme,
                         ui::NativeTheme* web_theme)
    : ui_theme_(ui_theme), web_theme_(web_theme) {
  ui_theme_->AddObserver(this);
}

NativeTheme::~NativeTheme() {
  ui_theme_->RemoveObserver(this);
}

void NativeTheme::OnNativeThemeUpdatedOnUI() {
  Emit("updated");
}

void NativeTheme::OnNativeThemeUpdated(ui::NativeTheme* theme) {
  base::PostTask(FROM_HERE, {content::BrowserThread::UI},
                 base::BindOnce(&NativeTheme::OnNativeThemeUpdatedOnUI,
                                base::Unretained(this)));
}

void NativeTheme::SetThemeSource(ui::NativeTheme::ThemeSource override) {
  ui_theme_->set_theme_source(override);
  web_theme_->set_theme_source(override);
#if defined(OS_MAC)
  // 更新此新替代值的MacOS外观设置。
  UpdateMacOSAppearanceForOverrideValue(override);
#endif
  // TODO(MarshallOfSound)：更新所有现有浏览器窗口以使用GTK暗色。
  // 主旋律。
}

ui::NativeTheme::ThemeSource NativeTheme::GetThemeSource() const {
  return ui_theme_->theme_source();
}

bool NativeTheme::ShouldUseDarkColors() {
  return ui_theme_->ShouldUseDarkColors();
}

bool NativeTheme::ShouldUseHighContrastColors() {
  return ui_theme_->UserHasContrastPreference();
}

#if defined(OS_MAC)
const CFStringRef WhiteOnBlack = CFSTR("whiteOnBlack");
const CFStringRef UniversalAccessDomain = CFSTR("com.apple.universalaccess");
#endif

// TODO(MarshallOfSound)：Linux实现。
bool NativeTheme::ShouldUseInvertedColorScheme() {
#if defined(OS_MAC)
  CFPreferencesAppSynchronize(UniversalAccessDomain);
  Boolean keyExistsAndHasValidFormat = false;
  Boolean is_inverted = CFPreferencesGetAppBooleanValue(
      WhiteOnBlack, UniversalAccessDomain, &keyExistsAndHasValidFormat);
  if (!keyExistsAndHasValidFormat)
    return false;
  return is_inverted;
#else
  return ui_theme_->GetPlatformHighContrastColorScheme() ==
         ui::NativeTheme::PlatformHighContrastColorScheme::kDark;
#endif
}

// 静电。
gin::Handle<NativeTheme> NativeTheme::Create(v8::Isolate* isolate) {
  ui::NativeTheme* ui_theme = ui::NativeTheme::GetInstanceForNativeUi();
  ui::NativeTheme* web_theme = ui::NativeTheme::GetInstanceForWeb();
  return gin::CreateHandle(isolate,
                           new NativeTheme(isolate, ui_theme, web_theme));
}

gin::ObjectTemplateBuilder NativeTheme::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin_helper::EventEmitterMixin<NativeTheme>::GetObjectTemplateBuilder(
             isolate)
      .SetProperty("shouldUseDarkColors", &NativeTheme::ShouldUseDarkColors)
      .SetProperty("themeSource", &NativeTheme::GetThemeSource,
                   &NativeTheme::SetThemeSource)
      .SetProperty("shouldUseHighContrastColors",
                   &NativeTheme::ShouldUseHighContrastColors)
      .SetProperty("shouldUseInvertedColorScheme",
                   &NativeTheme::ShouldUseInvertedColorScheme);
}

const char* NativeTheme::GetTypeName() {
  return "NativeTheme";
}

}  // 命名空间API。

}  // 命名空间电子。

namespace {

using electron::api::NativeTheme;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin::Dictionary dict(isolate, exports);
  dict.Set("nativeTheme", NativeTheme::Create(isolate));
}

}  // 命名空间。

namespace gin {

v8::Local<v8::Value> Converter<ui::NativeTheme::ThemeSource>::ToV8(
    v8::Isolate* isolate,
    const ui::NativeTheme::ThemeSource& val) {
  switch (val) {
    case ui::NativeTheme::ThemeSource::kForcedDark:
      return ConvertToV8(isolate, "dark");
    case ui::NativeTheme::ThemeSource::kForcedLight:
      return ConvertToV8(isolate, "light");
    case ui::NativeTheme::ThemeSource::kSystem:
    default:
      return ConvertToV8(isolate, "system");
  }
}

bool Converter<ui::NativeTheme::ThemeSource>::FromV8(
    v8::Isolate* isolate,
    v8::Local<v8::Value> val,
    ui::NativeTheme::ThemeSource* out) {
  std::string theme_source;
  if (ConvertFromV8(isolate, val, &theme_source)) {
    if (theme_source == "dark") {
      *out = ui::NativeTheme::ThemeSource::kForcedDark;
    } else if (theme_source == "light") {
      *out = ui::NativeTheme::ThemeSource::kForcedLight;
    } else if (theme_source == "system") {
      *out = ui::NativeTheme::ThemeSource::kSystem;
    } else {
      return false;
    }
    return true;
  }
  return false;
}

}  // 命名空间杜松子酒

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_common_native_theme, Initialize)
