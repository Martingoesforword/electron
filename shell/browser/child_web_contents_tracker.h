// 版权所有(C)2017 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_CHILD_WEB_CONTENTS_TRACKER_H_
#define SHELL_BROWSER_CHILD_WEB_CONTENTS_TRACKER_H_

#include <string>

#include "content/public/browser/web_contents_user_data.h"

namespace electron {

// ChildWebContentsTracker跟踪子WebContents。
// 由本机`window.open()`创建。
struct ChildWebContentsTracker
    : public content::WebContentsUserData<ChildWebContentsTracker> {
  ~ChildWebContentsTracker() override;

  GURL url;
  std::string frame_name;
  content::Referrer referrer;
  std::string raw_features;
  scoped_refptr<network::ResourceRequestBody> body;

 private:
  explicit ChildWebContentsTracker(content::WebContents* web_contents);
  friend class content::WebContentsUserData<ChildWebContentsTracker>;

  WEB_CONTENTS_USER_DATA_KEY_DECL();

  DISALLOW_COPY_AND_ASSIGN(ChildWebContentsTracker);
};

}  // 命名空间电子。

#endif  // Shell_Browser_Child_Web_Contents_Tracker_H_
