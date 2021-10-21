// 版权所有(C)2018 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/api/gpu_info_enumerator.h"

#include <utility>

namespace electron {

GPUInfoEnumerator::GPUInfoEnumerator()
    : value_stack(), current(std::make_unique<base::DictionaryValue>()) {}

GPUInfoEnumerator::~GPUInfoEnumerator() = default;

void GPUInfoEnumerator::AddInt64(const char* name, int64_t value) {
  current->SetInteger(name, value);
}

void GPUInfoEnumerator::AddInt(const char* name, int value) {
  current->SetInteger(name, value);
}

void GPUInfoEnumerator::AddString(const char* name, const std::string& value) {
  if (!value.empty())
    current->SetString(name, value);
}

void GPUInfoEnumerator::AddBool(const char* name, bool value) {
  current->SetBoolean(name, value);
}

void GPUInfoEnumerator::AddTimeDeltaInSecondsF(const char* name,
                                               const base::TimeDelta& value) {
  current->SetInteger(name, value.InMilliseconds());
}

void GPUInfoEnumerator::AddBinary(const char* name,
                                  const base::span<const uint8_t>& value) {
  current->Set(name, std::make_unique<base::Value>(value));
}

void GPUInfoEnumerator::BeginGPUDevice() {
  value_stack.push(std::move(current));
  current = std::make_unique<base::DictionaryValue>();
}

void GPUInfoEnumerator::EndGPUDevice() {
  auto& top_value = value_stack.top();
  // GPUDevice可以是多个。因此，创建一个所有人的列表。
  // 第一个是活动的GPU设备。
  if (top_value->HasKey(kGPUDeviceKey)) {
    base::ListValue* list;
    top_value->GetList(kGPUDeviceKey, &list);
    list->Append(std::move(current));
  } else {
    auto gpus = std::make_unique<base::ListValue>();
    gpus->Append(std::move(current));
    top_value->SetList(kGPUDeviceKey, std::move(gpus));
  }
  current = std::move(top_value);
  value_stack.pop();
}

void GPUInfoEnumerator::BeginVideoDecodeAcceleratorSupportedProfile() {
  value_stack.push(std::move(current));
  current = std::make_unique<base::DictionaryValue>();
}

void GPUInfoEnumerator::EndVideoDecodeAcceleratorSupportedProfile() {
  auto& top_value = value_stack.top();
  top_value->SetDictionary(kVideoDecodeAcceleratorSupportedProfileKey,
                           std::move(current));
  current = std::move(top_value);
  value_stack.pop();
}

void GPUInfoEnumerator::BeginVideoEncodeAcceleratorSupportedProfile() {
  value_stack.push(std::move(current));
  current = std::make_unique<base::DictionaryValue>();
}

void GPUInfoEnumerator::EndVideoEncodeAcceleratorSupportedProfile() {
  auto& top_value = value_stack.top();
  top_value->SetDictionary(kVideoEncodeAcceleratorSupportedProfileKey,
                           std::move(current));
  current = std::move(top_value);
  value_stack.pop();
}

void GPUInfoEnumerator::BeginImageDecodeAcceleratorSupportedProfile() {
  value_stack.push(std::move(current));
  current = std::make_unique<base::DictionaryValue>();
}

void GPUInfoEnumerator::EndImageDecodeAcceleratorSupportedProfile() {
  auto& top_value = value_stack.top();
  top_value->SetDictionary(kImageDecodeAcceleratorSupportedProfileKey,
                           std::move(current));
  current = std::move(top_value);
  value_stack.pop();
}

void GPUInfoEnumerator::BeginAuxAttributes() {
  value_stack.push(std::move(current));
  current = std::make_unique<base::DictionaryValue>();
}

void GPUInfoEnumerator::EndAuxAttributes() {
  auto& top_value = value_stack.top();
  top_value->SetDictionary(kAuxAttributesKey, std::move(current));
  current = std::move(top_value);
  value_stack.pop();
}

void GPUInfoEnumerator::BeginOverlayInfo() {
  value_stack.push(std::move(current));
  current = std::make_unique<base::DictionaryValue>();
}

void GPUInfoEnumerator::EndOverlayInfo() {
  auto& top_value = value_stack.top();
  top_value->SetDictionary(kOverlayInfo, std::move(current));
  current = std::move(top_value);
  value_stack.pop();
}

std::unique_ptr<base::DictionaryValue> GPUInfoEnumerator::GetDictionary() {
  return std::move(current);
}

}  // 命名空间电子
