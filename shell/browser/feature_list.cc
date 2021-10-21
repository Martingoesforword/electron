// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "electron/shell/browser/feature_list.h"

#include <string>

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/metrics/field_trial.h"
#include "components/spellcheck/common/spellcheck_features.h"
#include "content/public/common/content_features.h"
#include "electron/buildflags/buildflags.h"
#include "media/base/media_switches.h"
#include "net/base/features.h"
#include "services/network/public/cpp/features.h"

namespace electron {

void InitializeFeatureList() {
  auto* cmd_line = base::CommandLine::ForCurrentProcess();
  auto enable_features =
      cmd_line->GetSwitchValueASCII(::switches::kEnableFeatures);
  auto disable_features =
      cmd_line->GetSwitchValueASCII(::switches::kDisableFeatures);
  // 禁止创建具有逐个站点进程模式的备用呈现器进程，
  // 它会干扰我们对非沙箱模式的进程首选项跟踪。
  // 当我们的站点实例策略与铬保持一致时，可以重新启用。
  // 启用节点集成时。
  disable_features +=
      std::string(",") + features::kSpareRendererForSitePerProcess.name;

#if !BUILDFLAG(ENABLE_PICTURE_IN_PICTURE)
  disable_features += std::string(",") + media::kPictureInPicture.name;
#endif

#if defined(OS_WIN)
  // 禁用Windows的异步拼写检查建议，这会导致。
  // 要返回的空建议列表。
  disable_features +=
      std::string(",") + spellcheck::kWinRetrieveSuggestionsOnlyOnDemand.name;
#endif
  base::FeatureList::InitializeInstance(enable_features, disable_features);
}

void InitializeFieldTrials() {
  auto* cmd_line = base::CommandLine::ForCurrentProcess();
  auto force_fieldtrials =
      cmd_line->GetSwitchValueASCII(::switches::kForceFieldTrials);

  base::FieldTrialList::CreateTrialsFromString(force_fieldtrials);
}

}  // 命名空间电子
