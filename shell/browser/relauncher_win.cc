// 版权所有(C)2016 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/relauncher.h"

#include <windows.h>

#include "base/logging.h"
#include "base/process/launch.h"
#include "base/strings/stringprintf.h"
#include "base/win/scoped_handle.h"
#include "sandbox/win/src/nt_internals.h"
#include "sandbox/win/src/win_utils.h"
#include "ui/base/win/shell.h"

namespace relauncher {

namespace internal {

namespace {

const CharType* kWaitEventName = L"ElectronRelauncherWaitEvent";

HANDLE GetParentProcessHandle(base::ProcessHandle handle) {
  NtQueryInformationProcessFunction NtQueryInformationProcess = nullptr;
  ResolveNTFunctionPtr("NtQueryInformationProcess", &NtQueryInformationProcess);
  if (!NtQueryInformationProcess) {
    LOG(ERROR) << "Unable to get NtQueryInformationProcess";
    return NULL;
  }

  PROCESS_BASIC_INFORMATION pbi;
  LONG status =
      NtQueryInformationProcess(handle, ProcessBasicInformation, &pbi,
                                sizeof(PROCESS_BASIC_INFORMATION), NULL);
  if (!NT_SUCCESS(status)) {
    LOG(ERROR) << "NtQueryInformationProcess failed";
    return NULL;
  }

  return ::OpenProcess(PROCESS_ALL_ACCESS, TRUE,
                       pbi.InheritedFromUniqueProcessId);
}

StringType AddQuoteForArg(const StringType& arg) {
  // 我们遵循CommandLineToArgvW的报价规则。
  // Http://msdn.microsoft.com/en-us/library/17w5ykft.aspx。
  std::wstring quotable_chars(L" \\\"");
  if (arg.find_first_of(quotable_chars) == std::wstring::npos) {
    // 不需要报价。
    return arg;
  }

  std::wstring out;
  out.push_back(L'"');
  for (size_t i = 0; i < arg.size(); ++i) {
    if (arg[i] == '\\') {
      // 找出这一系列反斜杠的范围。
      size_t start = i, end = start + 1;
      for (; end < arg.size() && arg[end] == '\\'; ++end) {
      }
      size_t backslash_count = end - start;

      // 只有在运行后跟双引号的情况下，反斜杠才是转义。
      // 由于我们还将以双引号结束字符串，因此我们转义为。
      // 可以是双引号，也可以是字符串末尾。
      if (end == arg.size() || arg[end] == '"') {
        // 引用一下，我们需要输出2倍的反斜杠。
        backslash_count *= 2;
      }
      for (size_t j = 0; j < backslash_count; ++j)
        out.push_back('\\');

      // 在结束前将i前进到1，以平衡循环中的i++。
      i = end - 1;
    } else if (arg[i] == '"') {
      out.push_back('\\');
      out.push_back('"');
    } else {
      out.push_back(arg[i]);
    }
  }
  out.push_back('"');

  return out;
}

}  // 命名空间。

StringType GetWaitEventName(base::ProcessId pid) {
  return base::StringPrintf(L"%ls-%d", kWaitEventName, static_cast<int>(pid));
}

StringType ArgvToCommandLineString(const StringVector& argv) {
  StringType command_line;
  for (const StringType& arg : argv) {
    if (!command_line.empty())
      command_line += L' ';
    command_line += AddQuoteForArg(arg);
  }
  return command_line;
}

void RelauncherSynchronizeWithParent() {
  base::Process process = base::Process::Current();
  base::win::ScopedHandle parent_process(
      GetParentProcessHandle(process.Handle()));

  // 通知父进程它现在可以退出。
  StringType name = internal::GetWaitEventName(process.Pid());
  base::win::ScopedHandle wait_event(
      CreateEvent(NULL, TRUE, FALSE, name.c_str()));
  ::SetEvent(wait_event.Get());

  // 等待父进程退出。
  WaitForSingleObject(parent_process.Get(), INFINITE);
}

int LaunchProgram(const StringVector& relauncher_args,
                  const StringVector& argv) {
  base::LaunchOptions options;
  base::Process process =
      base::LaunchProcess(ArgvToCommandLineString(argv), options);
  return process.IsValid() ? 0 : 1;
}

}  // 命名空间内部。

}  // 命名空间重新启动器
