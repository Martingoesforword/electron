// 版权所有(C)2016年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/ui/views/win_caption_button.h"

#include <utility>

#include "base/i18n/rtl.h"
#include "base/numerics/safe_conversions.h"
#include "chrome/browser/ui/frame/window_frame_util.h"
#include "chrome/grit/theme_resources.h"
#include "shell/browser/ui/views/win_frame_view.h"
#include "shell/common/color_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/theme_provider.h"
#include "ui/gfx/animation/tween.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gfx/scoped_canvas.h"

namespace electron {

WinCaptionButton::WinCaptionButton(PressedCallback callback,
                                   WinFrameView* frame_view,
                                   ViewID button_type,
                                   const std::u16string& accessible_name)
    : views::Button(std::move(callback)),
      frame_view_(frame_view),
      button_type_(button_type) {
  SetAnimateOnStateChange(true);
  // 默认情况下不可聚焦，仅用于辅助功能。
  SetFocusBehavior(FocusBehavior::ACCESSIBLE_ONLY);
  SetAccessibleName(accessible_name);
}

gfx::Size WinCaptionButton::CalculatePreferredSize() const {
  // TODO(Bsep)：此函数中的大小适用于1X设备比例，不。
  // 匹配HiDPI上的Windows按钮大小。
  int height = WindowFrameUtil::kWindows10GlassCaptionButtonHeightRestored;
  int base_width = WindowFrameUtil::kWindows10GlassCaptionButtonWidth;
  return gfx::Size(base_width + GetBetweenButtonSpacing(), height);
}

void WinCaptionButton::OnPaintBackground(gfx::Canvas* canvas) {
  // 绘制按钮的背景(该按钮的半透明矩形。
  // 在您悬停或按下按钮时出现)。

  const SkColor bg_color = frame_view_->window()->overlay_button_color();
  const SkAlpha theme_alpha = SkColorGetA(bg_color);

  gfx::Rect bounds = GetContentsBounds();
  bounds.Inset(0, 0, 0, 0);

  canvas->FillRect(bounds, SkColorSetA(bg_color, theme_alpha));

  SkColor base_color;
  SkAlpha hovered_alpha, pressed_alpha;
  if (button_type_ == VIEW_ID_CLOSE_BUTTON) {
    base_color = SkColorSetRGB(0xE8, 0x11, 0x23);
    hovered_alpha = SK_AlphaOPAQUE;
    pressed_alpha = 0x98;
  } else {
    // 匹配本地按钮。
    base_color = frame_view_->GetReadableFeatureColor(bg_color);
    hovered_alpha = 0x1A;
    pressed_alpha = 0x33;

    if (theme_alpha > 0) {
      // 主题按钮略微增加了不透明度以使其突出。
      // 与视觉繁忙的框架图像形成对比。
      constexpr float kAlphaScale = 1.3f;
      hovered_alpha = base::ClampRound<SkAlpha>(hovered_alpha * kAlphaScale);
      pressed_alpha = base::ClampRound<SkAlpha>(pressed_alpha * kAlphaScale);
    }
  }

  SkAlpha alpha;
  if (GetState() == STATE_PRESSED)
    alpha = pressed_alpha;
  else
    alpha = gfx::Tween::IntValueBetween(hover_animation().GetCurrentValue(),
                                        SK_AlphaTRANSPARENT, hovered_alpha);
  canvas->FillRect(bounds, SkColorSetA(base_color, alpha));
}

void WinCaptionButton::PaintButtonContents(gfx::Canvas* canvas) {
  PaintSymbol(canvas);
}

int WinCaptionButton::GetBetweenButtonSpacing() const {
  const int display_order_index = GetButtonDisplayOrderIndex();
  return display_order_index == 0
             ? 0
             : WindowFrameUtil::kWindows10GlassCaptionButtonVisualSpacing;
}

int WinCaptionButton::GetButtonDisplayOrderIndex() const {
  int button_display_order = 0;
  switch (button_type_) {
    case VIEW_ID_MINIMIZE_BUTTON:
      button_display_order = 0;
      break;
    case VIEW_ID_MAXIMIZE_BUTTON:
    case VIEW_ID_RESTORE_BUTTON:
      button_display_order = 1;
      break;
    case VIEW_ID_CLOSE_BUTTON:
      button_display_order = 2;
      break;
    default:
      NOTREACHED();
      return 0;
  }

  // 如果我们处于RTL模式，则颠倒顺序。
  if (base::i18n::IsRTL())
    button_display_order = 2 - button_display_order;

  return button_display_order;
}

namespace {

// Canvas：：DrawRect的笔划可能超出|RECT|的界限，因此这将绘制一个。
// 矩形插入，以便将结果限制为|Rect|的大小。
void DrawRect(gfx::Canvas* canvas,
              const gfx::Rect& rect,
              const cc::PaintFlags& flags) {
  gfx::RectF rect_f(rect);
  float stroke_half_width = flags.getStrokeWidth() / 2;
  rect_f.Inset(stroke_half_width, stroke_half_width);
  canvas->DrawRect(rect_f, flags);
}

}  // 命名空间。

void WinCaptionButton::PaintSymbol(gfx::Canvas* canvas) {
  SkColor symbol_color = frame_view_->window()->overlay_symbol_color();

  if (button_type_ == VIEW_ID_CLOSE_BUTTON &&
      hover_animation().is_animating()) {
    symbol_color = gfx::Tween::ColorValueBetween(
        hover_animation().GetCurrentValue(), symbol_color, SK_ColorWHITE);
  } else if (button_type_ == VIEW_ID_CLOSE_BUTTON &&
             (GetState() == STATE_HOVERED || GetState() == STATE_PRESSED)) {
    symbol_color = SK_ColorWHITE;
  }

  gfx::ScopedCanvas scoped_canvas(canvas);
  const float scale = canvas->UndoDeviceScaleFactor();

  const int symbol_size_pixels = std::round(10 * scale);
  gfx::RectF bounds_rect(GetContentsBounds());
  bounds_rect.Scale(scale);
  gfx::Rect symbol_rect(gfx::ToEnclosingRect(bounds_rect));
  symbol_rect.ClampToCenteredSize(
      gfx::Size(symbol_size_pixels, symbol_size_pixels));

  cc::PaintFlags flags;
  flags.setAntiAlias(false);
  flags.setColor(symbol_color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  // 每当我们达到一个新的积分比例时，笔划宽度就会上升一个像素。
  const int stroke_width = std::floor(scale);
  flags.setStrokeWidth(stroke_width);

  switch (button_type_) {
    case VIEW_ID_MINIMIZE_BUTTON: {
      const int y = symbol_rect.CenterPoint().y();
      const gfx::Point p1 = gfx::Point(symbol_rect.x(), y);
      const gfx::Point p2 = gfx::Point(symbol_rect.right(), y);
      canvas->DrawLine(p1, p2, flags);
      return;
    }

    case VIEW_ID_MAXIMIZE_BUTTON:
      DrawRect(canvas, symbol_rect, flags);
      return;

    case VIEW_ID_RESTORE_BUTTON: {
      // 左下角(“前面”)方块。
      const int separation = std::floor(2 * scale);
      symbol_rect.Inset(0, separation, separation, 0);
      DrawRect(canvas, symbol_rect, flags);

      // 右上角(“后面”)方块。
      canvas->ClipRect(symbol_rect, SkClipOp::kDifference);
      symbol_rect.Offset(separation, -separation);
      DrawRect(canvas, symbol_rect, flags);
      return;
    }

    case VIEW_ID_CLOSE_BUTTON: {
      flags.setAntiAlias(true);
      // 关闭按钮的X被透明像素的“光环”包围。
      // 当X为白色时，透明像素需要更亮一点。
      // 才能看得见。
      const float stroke_halo =
          stroke_width * (symbol_color == SK_ColorWHITE ? 0.1f : 0.05f);
      flags.setStrokeWidth(stroke_width + stroke_halo);

      // TODO(Bsep)：这有时会以小数位数设备比例绘制未对齐的图形。
      // 因为按钮的原点不一定与像素对齐。
      canvas->ClipRect(symbol_rect);
      SkPath path;
      path.moveTo(symbol_rect.x(), symbol_rect.y());
      path.lineTo(symbol_rect.right(), symbol_rect.bottom());
      path.moveTo(symbol_rect.right(), symbol_rect.y());
      path.lineTo(symbol_rect.x(), symbol_rect.bottom());
      canvas->DrawPath(path, flags);
      return;
    }

    default:
      NOTREACHED();
      return;
  }
}
}  // 命名空间电子
