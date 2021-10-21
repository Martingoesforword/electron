// 版权所有(C)2021 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/common/logging.h"

#include <string>

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/environment.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "chrome/common/chrome_paths.h"
#include "content/public/common/content_switches.h"
#include "shell/common/electron_paths.h"

namespace logging {

constexpr base::StringPiece kLogFileName("ELECTRON_LOG_FILE");
constexpr base::StringPiece kElectronEnableLogging("ELECTRON_ENABLE_LOGGING");

base::FilePath GetLogFileName(const base::CommandLine& command_line) {
  std::string filename = command_line.GetSwitchValueASCII(switches::kLogFile);
  if (filename.empty())
    base::Environment::Create()->GetVar(kLogFileName, &filename);
  if (!filename.empty())
    return base::FilePath::FromUTF8Unsafe(filename);

  const base::FilePath log_filename(FILE_PATH_LITERAL("electron_debug.log"));
  base::FilePath log_path;

  if (base::PathService::Get(chrome::DIR_LOGS, &log_path)) {
    log_path = log_path.Append(log_filename);
    return log_path;
  } else {
    // 路径服务出错，仅在某处使用某个默认文件。
    return log_filename;
  }
}

bool HasExplicitLogFile(const base::CommandLine& command_line) {
  std::string filename = command_line.GetSwitchValueASCII(switches::kLogFile);
  if (filename.empty())
    base::Environment::Create()->GetVar(kLogFileName, &filename);
  return !filename.empty();
}

LoggingDestination DetermineLoggingDestination(
    const base::CommandLine& command_line,
    bool is_preinit) {
  bool enable_logging = false;
  std::string logging_destination;
  if (command_line.HasSwitch(::switches::kEnableLogging)) {
    enable_logging = true;
    logging_destination =
        command_line.GetSwitchValueASCII(switches::kEnableLogging);
  } else {
    auto env = base::Environment::Create();
    if (env->HasVar(kElectronEnableLogging)) {
      enable_logging = true;
      env->GetVar(kElectronEnableLogging, &logging_destination);
    }
  }
  if (!enable_logging)
    return LOG_NONE;

  // --enable-将日志记录到stderr，--enable-log=将文件日志记录到文件。
  // 注意：这与Chromium不同，Chromium使用--enable-log将日志记录到一个文件中。
  // 和--enable-log=stderr记录到stderr，因为这就是Electron。
  // 曾经工作过，所以为了不让任何依赖于。
  // --Enable-Logging Logging to stderr，我们通过以下方式保留旧行为。
  // 默认设置。
  // 如果将--log-file或Electronics_log_file与一起指定。
  // --ENABLE-LOGGING，返回log_to_file。
  // 如果我们处于初始化前阶段，在JS运行之前，我们希望避免。
  // 记录到用户数据目录中的默认日志文件，
  // 因为我们无法准确确定用户数据目录。
  // 在JS运行之前。相反，除非有明确的文件名，否则应登录到stderr。
  // 给你的。
  if (HasExplicitLogFile(command_line) ||
      (logging_destination == "file" && !is_preinit))
    return LOG_TO_FILE;
  return LOG_TO_SYSTEM_DEBUG_LOG | LOG_TO_STDERR;
}

void InitElectronLogging(const base::CommandLine& command_line,
                         bool is_preinit) {
  const std::string process_type =
      command_line.GetSwitchValueASCII(::switches::kProcessType);
  LoggingDestination logging_dest =
      DetermineLoggingDestination(command_line, is_preinit);
  LogLockingState log_locking_state = LOCK_LOG_FILE;
  base::FilePath log_path;

  if (command_line.HasSwitch(::switches::kLoggingLevel) &&
      GetMinLogLevel() >= 0) {
    std::string log_level =
        command_line.GetSwitchValueASCII(::switches::kLoggingLevel);
    int level = 0;
    if (base::StringToInt(log_level, &level) && level >= 0 &&
        level < LOGGING_NUM_SEVERITIES) {
      SetMinLogLevel(level);
    } else {
      DLOG(WARNING) << "Bad log level: " << log_level;
    }
  }

  // 除非我们需要，否则不要解析日志路径。否则我们会留下一个空档。
  // Windows上沙箱锁定后的ALPC句柄。
  if ((logging_dest & LOG_TO_FILE) != 0) {
    log_path = GetLogFileName(command_line);
  } else {
    log_locking_state = DONT_LOCK_LOG_FILE;
  }

  // 在Windows上，日志文件名中包含非规范正斜杠会导致。
  // 有关沙盒过滤器的问题，请参阅https://crbug.com/859676。
  log_path = log_path.NormalizePathSeparators();

  LoggingSettings settings;
  settings.logging_dest = logging_dest;
  settings.log_file_path = log_path.value().c_str();
  settings.lock_log = log_locking_state;
  // 如果我们记录到与--log-file一起传递的显式文件，我们不希望。
  // 在第二次初始化时删除日志文件。
  settings.delete_old =
      process_type.empty() && (is_preinit || !HasExplicitLogFile(command_line))
          ? DELETE_OLD_LOG_FILE
          : APPEND_TO_OLD_LOG_FILE;
  bool success = InitLogging(settings);
  if (!success) {
    PLOG(FATAL) << "Failed to init logging";
  }

  SetLogItems(true /* PID。*/, false, true /* 时间戳。*/, false);
}

}  // 命名空间日志记录
