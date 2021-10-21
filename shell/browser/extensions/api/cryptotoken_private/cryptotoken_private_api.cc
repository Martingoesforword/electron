// 版权所有2014年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/extensions/api/cryptotoken_private/cryptotoken_private_api.h"

#include <stddef.h>
#include <memory>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
// #包含“chrome/browser/extensions/extension_tab_util.h”
// #包含“chrome/browser/permissions/attestation_permission_request.h”
// #include“Chrome/Browser/Profiles/profile.h”
#include "chrome/common/chrome_features.h"
#include "chrome/common/pref_names.h"
// #包括。
// “components/page_load_metrics/browser/metrics_web_contents_observer.h”
// #包含“components/permissions/permission_request_manager.h”
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "crypto/sha2.h"
#include "device/fido/filter.h"
#include "extensions/browser/extension_api_frame_id_map.h"
#include "extensions/common/error_utils.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "url/origin.h"

#if defined(OS_WIN)
#include "device/fido/features.h"
#include "device/fido/win/webauthn_api.h"
#endif  // 已定义(OS_WIN)。

namespace extensions {

namespace api {

namespace {

const char kGoogleDotCom[] = "google.com";
constexpr const char* kGoogleGstaticAppIds[] = {
    "https:// Www.gstatic.com/securitykey/Origins.json“，
    "https:// Www.gstatic.com/securitykey/a/google.com/origins.json“}；

// ContainsAppIdByHash返回TRUE当且仅当。
// |list|equals|hash|的元素。
bool ContainsAppIdByHash(const base::ListValue& list,
                         const std::vector<uint8_t>& hash) {
  if (hash.size() != crypto::kSHA256Length) {
    return false;
  }

  for (const auto& i : list.GetList()) {
    const std::string& s = i.GetString();
    if (s.find('/') == std::string::npos) {
      // 没有斜杠表示这是Webauthn RP ID，而不是U2F AppID。
      continue;
    }

    if (crypto::SHA256HashString(s).compare(
            0, crypto::kSHA256Length,
            reinterpret_cast<const char*>(hash.data()),
            crypto::kSHA256Length) == 0) {
      return true;
    }
  }

  return false;
}

content::RenderFrameHost* RenderFrameHostForTabAndFrameId(
    content::BrowserContext* const browser_context,
    const int tab_id,
    const int frame_id) {
  auto* contents = electron::api::WebContents::FromID(tab_id);
  if (!contents || !contents->web_contents()) {
    return nullptr;
  }
  return ExtensionApiFrameIdMap::GetRenderFrameHostById(
      contents->web_contents(), frame_id);
}

}  // 命名空间。

// Void CryptokenRegisterProfilePrefs(。
// User_prefs：：PrefRegistrySynCable*注册表){。
// Registry-&gt;RegisterListPref(prefs：：kSecurityKeyPermitAttestation)；
// }。

CryptotokenPrivateCanOriginAssertAppIdFunction::
    CryptotokenPrivateCanOriginAssertAppIdFunction() = default;

ExtensionFunction::ResponseAction
CryptotokenPrivateCanOriginAssertAppIdFunction::Run() {
  std::unique_ptr<cryptotoken_private::CanOriginAssertAppId::Params> params =
      cryptotoken_private::CanOriginAssertAppId::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  const GURL origin_url(params->security_origin);
  if (!origin_url.is_valid()) {
    return RespondNow(Error(extensions::ErrorUtils::FormatErrorMessage(
        "Security origin * is not a valid URL", params->security_origin)));
  }
  const GURL app_id_url(params->app_id_url);
  if (!app_id_url.is_valid()) {
    return RespondNow(Error(extensions::ErrorUtils::FormatErrorMessage(
        "appId * is not a valid URL", params->app_id_url)));
  }

  if (origin_url == app_id_url) {
    return RespondNow(OneArgument(base::Value(true)));
  }

  // 获取两者的eTLD+1。
  const std::string origin_etldp1 =
      net::registry_controlled_domains::GetDomainAndRegistry(
          origin_url,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
  if (origin_etldp1.empty()) {
    return RespondNow(Error(extensions::ErrorUtils::FormatErrorMessage(
        "Could not find an eTLD for origin *", params->security_origin)));
  }
  const std::string app_id_etldp1 =
      net::registry_controlled_domains::GetDomainAndRegistry(
          app_id_url,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
  if (app_id_etldp1.empty()) {
    return RespondNow(Error(extensions::ErrorUtils::FormatErrorMessage(
        "Could not find an eTLD for appId *", params->app_id_url)));
  }
  if (origin_etldp1 == app_id_etldp1) {
    return RespondNow(OneArgument(base::Value(true)));
  }
  // 出于遗留目的，允许google.com源断言某些。
  // Gstatic.com应用程序。
  // TODO(Juanlang)：删除遗留约束时删除。
  if (origin_etldp1 == kGoogleDotCom) {
    for (const char* id : kGoogleGstaticAppIds) {
      if (params->app_id_url == id)
        return RespondNow(OneArgument(base::Value(true)));
    }
  }
  return RespondNow(OneArgument(base::Value(false)));
}

CryptotokenPrivateIsAppIdHashInEnterpriseContextFunction::
    CryptotokenPrivateIsAppIdHashInEnterpriseContextFunction() {}

ExtensionFunction::ResponseAction
CryptotokenPrivateIsAppIdHashInEnterpriseContextFunction::Run() {
  std::unique_ptr<cryptotoken_private::IsAppIdHashInEnterpriseContext::Params>
      params(
          cryptotoken_private::IsAppIdHashInEnterpriseContext::Params::Create(
              args()));
  EXTENSION_FUNCTION_VALIDATE(params);

#if 0
  Profile* const profile = Profile::FromBrowserContext(browser_context());
  const PrefService* const prefs = profile->GetPrefs();
  const base::ListValue* const permit_attestation =
      prefs->GetList(prefs::kSecurityKeyPermitAttestation);
#endif
  const base::ListValue permit_attestation;

  return RespondNow(ArgumentList(
      cryptotoken_private::IsAppIdHashInEnterpriseContext::Results::Create(
          ContainsAppIdByHash(permit_attestation, params->app_id_hash))));
}

CryptotokenPrivateCanAppIdGetAttestationFunction::
    CryptotokenPrivateCanAppIdGetAttestationFunction() {}

ExtensionFunction::ResponseAction
CryptotokenPrivateCanAppIdGetAttestationFunction::Run() {
  std::unique_ptr<cryptotoken_private::CanAppIdGetAttestation::Params> params =
      cryptotoken_private::CanAppIdGetAttestation::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  return RespondNow(Error("API not supported in Electron"));
#if 0
  const GURL origin_url(params->options.origin);
  if (!origin_url.is_valid()) {
    return RespondNow(Error(extensions::ErrorUtils::FormatErrorMessage(
        "Security origin * is not a valid URL", params->options.origin)));
  }
  const url::Origin origin(url::Origin::Create(origin_url));

  const std::string& app_id = params->options.app_id;

  // 如果企业策略允许该AppID，则没有权限。
  // 将显示提示符。
  // 配置文件*常量配置文件=Profile：：FromBrowserContext(browser_context())；
  // Const PrefService*const prefs=Profile-&gt;GetPrefs()；
  // 常量BASE：：ListValue*常量PERMIT_ATTESTATION=。
  // Prefs-&gt;GetList(prefs：：kSecurityKeyPermitAttestation)；

  // 对于(常量自动输入：PERMIT_ATTESTATION-&gt;GetList()){。
  // If(entry.GetString()==app_id)。
  // Return RespondNow(OneArgument(base：：value(True)；
  // }。

  // 如果原点被阻止，则拒绝认证。
  if (device::fido_filter::Evaluate(
          device::fido_filter::Operation::MAKE_CREDENTIAL, origin.Serialize(),
          /* 设备=。*/absl::nullopt, /* ID=。*/absl::nullopt) ==
      device::fido_filter::Action::NO_ATTESTATION) {
    return RespondNow(OneArgument(base::Value(false)));
  }

  // 如果禁用提示，则允许证明，因为这是历史记录。
  // 行为。
  if (!base::FeatureList::IsEnabled(
          ::features::kSecurityKeyAttestationPrompt)) {
    return RespondNow(OneArgument(base::Value(true)));
  }

#if defined(OS_WIN)
  // 如果请求是由某个版本的Windows WebAuthn API处理的。
  // 显示证明权限提示的窗口，不显示另一个。
  // 一。
  // 
  // 请注意，这没有考虑到。
  // WinWebAuthnApi已被FidoDiscoveryFactory覆盖禁用，
  // 这可以在测试中完成，也可以通过虚拟验证器WebDriver完成。
  // 原料药。
  if (base::FeatureList::IsEnabled(device::kWebAuthUseNativeWinApi) &&
      device::WinWebAuthnApi::GetDefault()->IsAvailable() &&
      device::WinWebAuthnApi::GetDefault()->Version() >=
          WEBAUTHN_API_VERSION_2) {
    return RespondNow(OneArgument(base::Value(true)));
  }
#endif  // 已定义(OS_WIN)。

  // 否则，显示权限提示并传回用户的决定。
  const GURL app_id_url(app_id);
  EXTENSION_FUNCTION_VALIDATE(app_id_url.is_valid());

  auto* contents = electron::api::WebContents::FromID(params->options.tab_id);
  if (!contents || !contents->web_contents()) {
    return RespondNow(Error("cannot find specified tab"));
  }

  // 权限：：PermissionRequestManager*PERMISSION_REQUEST_MANAGER=。
  // Permissions：：PermissionRequestManager：：FromWebContents(web_contents)；
  // Nullptr；
  // 如果(！Permission_Request_MANAGER){。
  return RespondNow(Error("no PermissionRequestManager"));
  // }。

  // //创建的Attestation PermissionRequest一旦完成就会自行删除。
  // Permission_Request_Manager-&gt;AddRequest(。
  // Web_Contents-&gt;GetMainFrame()，//扩展API针对特定的。
  // 选项卡，
  // //因此选择当前要。
  // //处理请求。
  // NewAttestation PermissionRequest(。
  // 起源，
  // Base：：BindOnce(。
  // &CryptotokenPrivateCanAppIdGetAttestationFunction：：Complete，
  // 这)；
  // Return RespondLater()；
#endif
}

void CryptotokenPrivateCanAppIdGetAttestationFunction::Complete(bool result) {
  Respond(OneArgument(base::Value(result)));
}

ExtensionFunction::ResponseAction
CryptotokenPrivateRecordRegisterRequestFunction::Run() {
  auto params =
      cryptotoken_private::RecordRegisterRequest::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  content::RenderFrameHost* frame = RenderFrameHostForTabAndFrameId(
      browser_context(), params->tab_id, params->frame_id);
  if (!frame) {
    return RespondNow(Error("cannot find specified tab or frame"));
  }

  // Page_load_metrics：：MetricsWebContentsObserver：：RecordFeatureUsage(。
  // Frame，blink：：mojom：：WebFeature：：kU2FCryptotokenRegister)；
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
CryptotokenPrivateRecordSignRequestFunction::Run() {
  auto params = cryptotoken_private::RecordSignRequest::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  content::RenderFrameHost* frame = RenderFrameHostForTabAndFrameId(
      browser_context(), params->tab_id, params->frame_id);
  if (!frame) {
    return RespondNow(Error("cannot find specified tab or frame"));
  }

  // Page_load_metrics：：MetricsWebContentsObserver：：RecordFeatureUsage(
  // Frame，blink：：mojom：：WebFeature：：kU2FCryptotokenSign)；
  return RespondNow(NoArguments());
}

}  // 命名空间API。
}  // 命名空间扩展
