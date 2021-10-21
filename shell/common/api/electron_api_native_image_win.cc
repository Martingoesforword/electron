// 版权所有(C)2020 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/common/api/electron_api_native_image.h"

#include <windows.h>

#include <thumbcache.h>
#include <wrl/client.h>

#include "base/win/scoped_com_initializer.h"
#include "shell/common/gin_converters/image_converter.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/skia_util.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/icon_util.h"
#include "ui/gfx/image/image_skia.h"

namespace electron {

namespace api {

// 静电。
v8::Local<v8::Promise> NativeImage::CreateThumbnailFromPath(
    v8::Isolate* isolate,
    const base::FilePath& path,
    const gfx::Size& size) {
  base::win::ScopedCOMInitializer scoped_com_initializer;

  gin_helper::Promise<gfx::Image> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();
  HRESULT hr;

  if (size.IsEmpty()) {
    promise.RejectWithErrorMessage("size must not be empty");
    return handle;
  }

  // 创建IShellItem。
  Microsoft::WRL::ComPtr<IShellItem> pItem;
  std::wstring image_path = path.value();
  hr = SHCreateItemFromParsingName(image_path.c_str(), nullptr,
                                   IID_PPV_ARGS(&pItem));

  if (FAILED(hr)) {
    promise.RejectWithErrorMessage(
        "failed to create IShellItem from the given path");
    return handle;
  }

  // 初始化缩略图缓存。
  Microsoft::WRL::ComPtr<IThumbnailCache> pThumbnailCache;
  hr = CoCreateInstance(CLSID_LocalThumbnailCache, nullptr, CLSCTX_INPROC,
                        IID_PPV_ARGS(&pThumbnailCache));
  if (FAILED(hr)) {
    promise.RejectWithErrorMessage(
        "failed to acquire local thumbnail cache reference");
    return handle;
  }

  // 填充IShellBitmap。
  Microsoft::WRL::ComPtr<ISharedBitmap> pThumbnail;
  WTS_CACHEFLAGS flags;
  WTS_THUMBNAILID thumbId;
  hr = pThumbnailCache->GetThumbnail(pItem.Get(), size.width(),
                                     WTS_FLAGS::WTS_NONE, &pThumbnail, &flags,
                                     &thumbId);

  if (FAILED(hr)) {
    promise.RejectWithErrorMessage(
        "failed to get thumbnail from local thumbnail cache reference");
    return handle;
  }

  // 初始化HBITMAP。
  HBITMAP hBitmap = NULL;
  hr = pThumbnail->GetSharedBitmap(&hBitmap);
  if (FAILED(hr)) {
    promise.RejectWithErrorMessage("failed to extract bitmap from thumbnail");
    return handle;
  }

  // 将HBITMAP转换为gfx：：Image。
  BITMAP bitmap;
  if (!GetObject(hBitmap, sizeof(bitmap), &bitmap)) {
    promise.RejectWithErrorMessage("could not convert HBITMAP to BITMAP");
    return handle;
  }

  ICONINFO icon_info;
  icon_info.fIcon = TRUE;
  icon_info.hbmMask = hBitmap;
  icon_info.hbmColor = hBitmap;

  base::win::ScopedHICON icon(CreateIconIndirect(&icon_info));
  SkBitmap skbitmap = IconUtil::CreateSkBitmapFromHICON(icon.get());
  gfx::ImageSkia image_skia =
      gfx::ImageSkia::CreateFromBitmap(skbitmap, 1.0 /* 比例因子。*/);
  gfx::Image gfx_image = gfx::Image(image_skia);
  promise.Resolve(gfx_image);
  return handle;
}

}  // 命名空间API。

}  // 命名空间电子
