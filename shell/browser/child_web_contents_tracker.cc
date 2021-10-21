// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/child_web_contents_tracker.h"

namespace electron {

ChildWebContentsTracker::ChildWebContentsTracker(
    content::WebContents* web_contents) {}

ChildWebContentsTracker::~ChildWebContentsTracker() = default;

WEB_CONTENTS_USER_DATA_KEY_IMPL(ChildWebContentsTracker)

}  // 命名空间电子
