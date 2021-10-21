// 版权所有(C)2020 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/common/crash_keys.h"

#include <deque>
#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/environment.h"
#include "base/no_destructor.h"
#include "base/strings/string_split.h"
#include "components/crash/core/common/crash_key.h"
#include "content/public/common/content_switches.h"
#include "electron/buildflags/buildflags.h"
#include "electron/fuses.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/electron_constants.h"
#include "shell/common/node_includes.h"
#include "shell/common/options_switches.h"
#include "shell/common/process_util.h"
#include "third_party/crashpad/crashpad/client/annotation.h"

namespace electron {

namespace crash_keys {

namespace {

#if defined(OS_LINUX)
// Breakpad有一个有缺陷的计算组块数量的系统。
// 我们添加127个字节来强制额外的块。
constexpr size_t kMaxCrashKeyValueSize = 20479;
#else
constexpr size_t kMaxCrashKeyValueSize = 20320;
#endif

static_assert(kMaxCrashKeyValueSize < crashpad::Annotation::kValueMaxSize,
              "max crash key value length above what crashpad supports");

using ExtraCrashKeys =
    std::deque<crash_reporter::CrashKeyString<kMaxCrashKeyValueSize>>;
ExtraCrashKeys& GetExtraCrashKeys() {
  static base::NoDestructor<ExtraCrashKeys> extra_keys;
  return *extra_keys;
}

std::deque<std::string>& GetExtraCrashKeyNames() {
  static base::NoDestructor<std::deque<std::string>> crash_key_names;
  return *crash_key_names;
}

}  // 命名空间。

constexpr uint32_t kMaxCrashKeyNameLength = 40;
#if defined(OS_LINUX)
static_assert(kMaxCrashKeyNameLength <=
                  crash_reporter::internal::kCrashKeyStorageKeySize,
              "max crash key name length above what breakpad supports");
#else
static_assert(kMaxCrashKeyNameLength <= crashpad::Annotation::kNameMaxLength,
              "max crash key name length above what crashpad supports");
#endif

void SetCrashKey(const std::string& key, const std::string& value) {
  // Chrome DCHECK()如果我们尝试设置名称长度超过以下值的批注，则执行Chrome DCHECK()。
  // 最大限度的。
  if (key.size() >= kMaxCrashKeyNameLength) {
    node::Environment* env =
        node::Environment::GetCurrent(JavascriptEnvironment::GetIsolate());
    EmitWarning(env,
                "The crash key name, \"" + key + "\", is longer than " +
                    std::to_string(kMaxCrashKeyNameLength) +
                    " bytes, ignoring it.",
                "electron");
    return;
  }

  auto& crash_key_names = GetExtraCrashKeyNames();

  auto iter = std::find(crash_key_names.begin(), crash_key_names.end(), key);
  if (iter == crash_key_names.end()) {
    crash_key_names.emplace_back(key);
    GetExtraCrashKeys().emplace_back(crash_key_names.back().c_str());
    iter = crash_key_names.end() - 1;
  }
  GetExtraCrashKeys()[iter - crash_key_names.begin()].Set(value);
}

void ClearCrashKey(const std::string& key) {
  const auto& crash_key_names = GetExtraCrashKeyNames();

  auto iter = std::find(crash_key_names.begin(), crash_key_names.end(), key);
  if (iter != crash_key_names.end()) {
    GetExtraCrashKeys()[iter - crash_key_names.begin()].Clear();
  }
}

void GetCrashKeys(std::map<std::string, std::string>* keys) {
  const auto& crash_key_names = GetExtraCrashKeyNames();
  const auto& crash_keys = GetExtraCrashKeys();
  int i = 0;
  for (const auto& key : crash_key_names) {
    const auto& value = crash_keys[i++];
    if (value.is_set()) {
      keys->emplace(key, value.value());
    }
  }
}

namespace {
bool IsRunningAsNode() {
#if BUILDFLAG(ENABLE_RUN_AS_NODE)
  if (!electron::fuses::IsRunAsNodeEnabled())
    return false;

  return base::Environment::Create()->HasVar(electron::kRunAsNode);
#else
  return false;
#endif
}
}  // 命名空间。

void SetCrashKeysFromCommandLine(const base::CommandLine& command_line) {
#if defined(OS_LINUX)
  if (command_line.HasSwitch(switches::kGlobalCrashKeys)) {
    std::vector<std::pair<std::string, std::string>> global_crash_keys;
    base::SplitStringIntoKeyValuePairs(
        command_line.GetSwitchValueASCII(switches::kGlobalCrashKeys), '=', ',',
        &global_crash_keys);
    for (const auto& pair : global_crash_keys) {
      SetCrashKey(pair.first, pair.second);
    }
  }
#endif

  // 注意：这与//Components/Crash的‘ptype’键是多余的。
  // 报告；它的存在是为了向后兼容。
  static crash_reporter::CrashKeyString<16> process_type_key("process_type");
  if (IsRunningAsNode()) {
    process_type_key.Set("node");
  } else {
    std::string process_type =
        command_line.GetSwitchValueASCII(::switches::kProcessType);
    if (process_type.empty()) {
      process_type_key.Set("browser");
    } else {
      process_type_key.Set(process_type);
    }
  }
}

void SetPlatformCrashKey() {
  // TODO(Nornagon)：使用‘Plat’键是多余的。
  // //Components/Crash已经包含。把它拿开。
  static crash_reporter::CrashKeyString<8> platform_key("platform");
#if defined(OS_WIN)
  platform_key.Set("win32");
#elif defined(OS_MAC)
  platform_key.Set("darwin");
#elif defined(OS_LINUX)
  platform_key.Set("linux");
#else
  platform_key.Set("unknown");
#endif
}

}  // 命名空间CRASH_KEYS。

}  // 命名空间电子
