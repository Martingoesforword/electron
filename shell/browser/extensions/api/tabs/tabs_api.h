// 版权所有(C)2019 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_EXTENSIONS_API_TABS_TABS_API_H_
#define SHELL_BROWSER_EXTENSIONS_API_TABS_TABS_API_H_

#include <string>

#include "extensions/browser/api/execute_code_function.h"
#include "extensions/browser/extension_function.h"
#include "extensions/common/extension_resource.h"

class GURL;

namespace extensions {

// 实现接口调用tab.ecuteScript和tab.insert tCSS。
class ExecuteCodeInTabFunction : public ExecuteCodeFunction {
 public:
  ExecuteCodeInTabFunction();

 protected:
  ~ExecuteCodeInTabFunction() override;

  // 初始化|Execute_tab_id_|和|Details_|。
  InitResult Init() override;
  bool CanExecuteScriptOnPage(std::string* error) override;
  ScriptExecutor* GetScriptExecutor(std::string* error) override;
  bool IsWebView() const override;
  const GURL& GetWebViewSrc() const override;

 private:
  // 执行代码的选项卡的ID。
  int execute_tab_id_;
};

class TabsExecuteScriptFunction : public ExecuteCodeInTabFunction {
 protected:
  bool ShouldInsertCSS() const override;
  bool ShouldRemoveCSS() const override;

 private:
  ~TabsExecuteScriptFunction() override {}

  DECLARE_EXTENSION_FUNCTION("tabs.executeScript", TABS_EXECUTESCRIPT)
};

class TabsGetFunction : public ExtensionFunction {
  ~TabsGetFunction() override {}
  ResponseAction Run() override;
  DECLARE_EXTENSION_FUNCTION("tabs.get", TABS_GET)
};

class TabsSetZoomFunction : public ExtensionFunction {
 private:
  ~TabsSetZoomFunction() override {}

  ResponseAction Run() override;

  DECLARE_EXTENSION_FUNCTION("tabs.setZoom", TABS_SETZOOM)
};

class TabsGetZoomFunction : public ExtensionFunction {
 private:
  ~TabsGetZoomFunction() override {}

  ResponseAction Run() override;

  DECLARE_EXTENSION_FUNCTION("tabs.getZoom", TABS_GETZOOM)
};

class TabsSetZoomSettingsFunction : public ExtensionFunction {
 private:
  ~TabsSetZoomSettingsFunction() override {}

  ResponseAction Run() override;

  DECLARE_EXTENSION_FUNCTION("tabs.setZoomSettings", TABS_SETZOOMSETTINGS)
};

class TabsGetZoomSettingsFunction : public ExtensionFunction {
 private:
  ~TabsGetZoomSettingsFunction() override {}

  ResponseAction Run() override;

  DECLARE_EXTENSION_FUNCTION("tabs.getZoomSettings", TABS_GETZOOMSETTINGS)
};

class TabsUpdateFunction : public ExtensionFunction {
 public:
  TabsUpdateFunction();

 protected:
  ~TabsUpdateFunction() override {}
  bool UpdateURL(const std::string& url, int tab_id, std::string* error);
  ResponseValue GetResult();

  content::WebContents* web_contents_;

 private:
  ResponseAction Run() override;
  void OnExecuteCodeFinished(const std::string& error,
                             const GURL& on_url,
                             const base::ListValue& script_result);

  DECLARE_EXTENSION_FUNCTION("tabs.update", TABS_UPDATE)
};
}  // 命名空间扩展。

#endif  // Shell_Browser_Extensions_API_Tabs_Tabs_API_H_
