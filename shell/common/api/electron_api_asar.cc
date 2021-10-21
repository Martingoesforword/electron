// 版权所有(C)2014 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include <vector>

#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "gin/wrappable.h"
#include "shell/common/asar/archive.h"
#include "shell/common/asar/asar_util.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"
#include "shell/common/node_util.h"

namespace {

class Archive : public gin::Wrappable<Archive> {
 public:
  static gin::Handle<Archive> Create(v8::Isolate* isolate,
                                     const base::FilePath& path) {
    auto archive = std::make_unique<asar::Archive>(path);
    if (!archive->Init())
      return gin::Handle<Archive>();
    return gin::CreateHandle(isolate, new Archive(isolate, std::move(archive)));
  }

  // 杜松子酒：：可包装的。
  static gin::WrapperInfo kWrapperInfo;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override {
    return gin::ObjectTemplateBuilder(isolate)
        .SetMethod("getFileInfo", &Archive::GetFileInfo)
        .SetMethod("stat", &Archive::Stat)
        .SetMethod("readdir", &Archive::Readdir)
        .SetMethod("realpath", &Archive::Realpath)
        .SetMethod("copyFileOut", &Archive::CopyFileOut)
        .SetMethod("getFdAndValidateIntegrityLater", &Archive::GetFD);
  }

  const char* GetTypeName() override { return "Archive"; }

 protected:
  Archive(v8::Isolate* isolate, std::unique_ptr<asar::Archive> archive)
      : archive_(std::move(archive)) {}

  // 读取文件的偏移量和大小。
  v8::Local<v8::Value> GetFileInfo(v8::Isolate* isolate,
                                   const base::FilePath& path) {
    asar::Archive::FileInfo info;
    if (!archive_ || !archive_->GetFileInfo(path, &info))
      return v8::False(isolate);
    gin_helper::Dictionary dict(isolate, v8::Object::New(isolate));
    dict.Set("size", info.size);
    dict.Set("unpacked", info.unpacked);
    dict.Set("offset", info.offset);
    if (info.integrity.has_value()) {
      gin_helper::Dictionary integrity(isolate, v8::Object::New(isolate));
      asar::HashAlgorithm algorithm = info.integrity.value().algorithm;
      switch (algorithm) {
        case asar::HashAlgorithm::SHA256:
          integrity.Set("algorithm", "SHA256");
          break;
        case asar::HashAlgorithm::NONE:
          CHECK(false);
          break;
      }
      integrity.Set("hash", info.integrity.value().hash);
      dict.Set("integrity", integrity);
    }
    return dict.GetHandle();
  }

  // 返回fs.stat(Path)的假结果。
  v8::Local<v8::Value> Stat(v8::Isolate* isolate, const base::FilePath& path) {
    asar::Archive::Stats stats;
    if (!archive_ || !archive_->Stat(path, &stats))
      return v8::False(isolate);
    gin_helper::Dictionary dict(isolate, v8::Object::New(isolate));
    dict.Set("size", stats.size);
    dict.Set("offset", stats.offset);
    dict.Set("isFile", stats.is_file);
    dict.Set("isDirectory", stats.is_directory);
    dict.Set("isLink", stats.is_link);
    return dict.GetHandle();
  }

  // 返回目录下的所有文件。
  v8::Local<v8::Value> Readdir(v8::Isolate* isolate,
                               const base::FilePath& path) {
    std::vector<base::FilePath> files;
    if (!archive_ || !archive_->Readdir(path, &files))
      return v8::False(isolate);
    return gin::ConvertToV8(isolate, files);
  }

  // 返回解析了符号链接的文件的路径。
  v8::Local<v8::Value> Realpath(v8::Isolate* isolate,
                                const base::FilePath& path) {
    base::FilePath realpath;
    if (!archive_ || !archive_->Realpath(path, &realpath))
      return v8::False(isolate);
    return gin::ConvertToV8(isolate, realpath);
  }

  // 将文件复制到临时文件中并返回新路径。
  v8::Local<v8::Value> CopyFileOut(v8::Isolate* isolate,
                                   const base::FilePath& path) {
    base::FilePath new_path;
    if (!archive_ || !archive_->CopyFileOut(path, &new_path))
      return v8::False(isolate);
    return gin::ConvertToV8(isolate, new_path);
  }

  // 返回文件描述符。
  int GetFD() const {
    if (!archive_)
      return -1;
    return archive_->GetUnsafeFD();
  }

 private:
  std::unique_ptr<asar::Archive> archive_;

  DISALLOW_COPY_AND_ASSIGN(Archive);
};

// 静电。
gin::WrapperInfo Archive::kWrapperInfo = {gin::kEmbedderNativeGin};

void InitAsarSupport(v8::Isolate* isolate, v8::Local<v8::Value> require) {
  // 评估asar_bundle.js。
  std::vector<v8::Local<v8::String>> asar_bundle_params = {
      node::FIXED_ONE_BYTE_STRING(isolate, "require")};
  std::vector<v8::Local<v8::Value>> asar_bundle_args = {require};
  electron::util::CompileAndCall(
      isolate->GetCurrentContext(), "electron/js2c/asar_bundle",
      &asar_bundle_params, &asar_bundle_args, nullptr);
}

v8::Local<v8::Value> SplitPath(v8::Isolate* isolate,
                               const base::FilePath& path) {
  gin_helper::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);
  base::FilePath asar_path, file_path;
  if (asar::GetAsarArchivePath(path, &asar_path, &file_path, true)) {
    dict.Set("isAsar", true);
    dict.Set("asarPath", asar_path);
    dict.Set("filePath", file_path);
  } else {
    dict.Set("isAsar", false);
  }
  return dict.GetHandle();
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  gin_helper::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("createArchive", &Archive::Create);
  dict.SetMethod("splitPath", &SplitPath);
  dict.SetMethod("initAsarSupport", &InitAsarSupport);
}

}  // 命名空间

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_common_asar, Initialize)
