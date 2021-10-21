// 版权所有2014程昭。版权所有。
// 此源代码的使用受麻省理工学院许可管理，该许可可在。
// 许可证文件。

#include "shell/common/gin_helper/persistent_dictionary.h"

namespace gin_helper {

PersistentDictionary::PersistentDictionary() = default;

PersistentDictionary::PersistentDictionary(v8::Isolate* isolate,
                                           v8::Local<v8::Object> object)
    : isolate_(isolate), handle_(isolate, object) {}

PersistentDictionary::PersistentDictionary(const PersistentDictionary& other)
    : isolate_(other.isolate_),
      handle_(isolate_, v8::Local<v8::Object>::New(isolate_, other.handle_)) {}

PersistentDictionary::~PersistentDictionary() = default;

PersistentDictionary& PersistentDictionary::operator=(
    const PersistentDictionary& other) {
  isolate_ = other.isolate_;
  handle_.Reset(isolate_, v8::Local<v8::Object>::New(isolate_, other.handle_));
  return *this;
}

v8::Local<v8::Object> PersistentDictionary::GetHandle() const {
  return v8::Local<v8::Object>::New(isolate_, handle_);
}

}  // 命名空间gin_helper
