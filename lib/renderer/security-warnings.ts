import { webFrame } from 'electron';
import { ipcRendererInternal } from '@electron/internal/renderer/ipc-renderer-internal';
import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';

let shouldLog: boolean | null = null;

const { platform, execPath, env } = process;

/* **此方法检查是否应该记录安全消息。*它通过确定我们是否以Electron身份运行来做到这一点，*这表明开发人员目前正在查看*应用程序。**@Returns{boolean}-我们应该记录吗？*/
const shouldLogSecurityWarnings = function (): boolean {
  if (shouldLog !== null) {
    return shouldLog;
  }

  switch (platform) {
    case 'darwin':
      shouldLog = execPath.endsWith('MacOS/Electron') ||
                  execPath.includes('Electron.app/Contents/Frameworks/');
      break;
    case 'freebsd':
    case 'linux':
      shouldLog = execPath.endsWith('/electron');
      break;
    case 'win32':
      shouldLog = execPath.endsWith('\\electron.exe');
      break;
    default:
      shouldLog = false;
  }

  if ((env && env.ELECTRON_DISABLE_SECURITY_WARNINGS) ||
      (window && window.ELECTRON_DISABLE_SECURITY_WARNINGS)) {
    shouldLog = false;
  }

  if ((env && env.ELECTRON_ENABLE_SECURITY_WARNINGS) ||
      (window && window.ELECTRON_ENABLE_SECURITY_WARNINGS)) {
    shouldLog = true;
  }

  return shouldLog;
};

/* **检查当前窗口是否为远程窗口。**@return{boolean}-这是远程协议吗？*/
const getIsRemoteProtocol = function () {
  if (window && window.location && window.location.protocol) {
    return /^(http|ftp)s?/gi.test(window.location.protocol);
  }
};

/* **检查当前窗口是否来自localhost。**@return{boolean}-当前窗口是否来自localhost？*/
const isLocalhost = function () {
  if (!window || !window.location) {
    return false;
  }

  return window.location.hostname === 'localhost';
};

/* **尝试确定是否设置了不带`unsafe-val`的CSP。**@return{boolean}是设置了`unsafe-val`的CSP吗？*/
const isUnsafeEvalEnabled: () => Promise<boolean> = function () {
  return webFrame.executeJavaScript(`(${(() => {
    try {
      eval(window.trustedTypes.emptyScript); // Eslint-Disable-line无求值。
    } catch {
      return false;
    }
    return true;
  }).toString()})()`, false);
};

const moreInformation = `\nFor more information and help, consult
https:// Electronjs.org/docs/tutorial/security。\n此警告不会显示。
once the app is packaged.`;

/* **#1只加载安全内容**查看当前页面加载的资源，并记录*所有通过HTTP或FTP加载的资源。*/
const warnAboutInsecureResources = function () {
  if (!window || !window.performance || !window.performance.getEntriesByType) {
    return;
  }

  const isLocal = (url: URL): boolean =>
    ['localhost', '127.0.0.1', '[::1]', ''].includes(url.hostname);
  const isInsecure = (url: URL): boolean =>
    ['http:', 'ftp:'].includes(url.protocol) && !isLocal(url);

  const resources = window.performance
    .getEntriesByType('resource')
    .filter(({ name }) => isInsecure(new URL(name)))
    .map(({ name }) => `- ${name}`)
    .join('\n');

  if (!resources || resources.length === 0) {
    return;
  }

  const warning = `This renderer process loads resources using insecure
  protocols. This exposes users of this app to unnecessary security risks.
  Consider loading the following resources over HTTPS or FTPS. \n${resources}
  \n${moreInformation}`;

  console.warn('Electron Security Warning (Insecure Resources)',
    'font-weight: bold;', warning);
};

/* 目前失踪，因为它有分支，仍处于实验阶段：*/
const warnAboutNodeWithRemoteContent = function (nodeIntegration: boolean) {
  if (!nodeIntegration || isLocalhost()) return;

  if (getIsRemoteProtocol()) {
    const warning = `This renderer process has Node.js integration enabled
    and attempted to load remote content from '${window.location}'. This
    exposes users of this app to severe security risks.\n${moreInformation}`;

    console.warn('Electron Security Warning (Node.js Integration with Remote Content)',
      'font-weight: bold;', warning);
  }
};

// 
// 当前缺少，因为我们不能轻松地通过编程检查这些情况：
// #4在加载远程内容的所有会话中使用ses.setPermissionRequestHandler()。
// **清单5：请勿关闭WebSecurity**记录关闭WebSecurity的警告信息。
// 清单上的**#6：定义Content-Security-Policy并使用限制性*规则(即script-src‘self’)**记录有关未设置或不安全的CSP的警告消息。

/* **清单7：请勿将allowRunningInsecureContent设置为true**记录关闭WebSecurity的警告信息。*/
const warnAboutDisabledWebSecurity = function (webPreferences?: Electron.WebPreferences) {
  if (!webPreferences || webPreferences.webSecurity !== false) return;

  const warning = `This renderer process has "webSecurity" disabled. This
  exposes users of this app to severe security risks.\n${moreInformation}`;

  console.warn('Electron Security Warning (Disabled webSecurity)',
    'font-weight: bold;', warning);
};

/* **清单上的#9：请勿使用enableBlinkFeature**记录有关enableBlinkFeature的警告消息。*/
const warnAboutInsecureCSP = function () {
  isUnsafeEvalEnabled().then((enabled) => {
    if (!enabled) return;

    const warning = `This renderer process has either no Content Security
    Policy set or a policy with "unsafe-eval" enabled. This exposes users of
    this app to unnecessary security risks.\n${moreInformation}`;

    console.warn('Electron Security Warning (Insecure Content-Security-Policy)',
      'font-weight: bold;', warning);
  }).catch(() => {});
};

/* 当前缺少，因为我们不能轻松地通过编程方式进行检查：*/
const warnAboutInsecureContentAllowed = function (webPreferences?: Electron.WebPreferences) {
  if (!webPreferences || !webPreferences.allowRunningInsecureContent) return;

  const warning = `This renderer process has "allowRunningInsecureContent"
  enabled. This exposes users of this app to severe security risks.\n
  ${moreInformation}`;

  console.warn('Electron Security Warning (allowRunningInsecureContent)',
    'font-weight: bold;', warning);
};

/* #12禁用或限制导航。*/
const warnAboutExperimentalFeatures = function (webPreferences?: Electron.WebPreferences) {
  if (!webPreferences || (!webPreferences.experimentalFeatures)) {
    return;
  }

  const warning = `This renderer process has "experimentalFeatures" enabled.
  This exposes users of this app to some security risk. If you do not need
  this feature, you should disable it.\n${moreInformation}`;

  console.warn('Electron Security Warning (experimentalFeatures)',
    'font-weight: bold;', warning);
};

/* #14不要对不受信任的内容使用`openExternal`*/
const warnAboutEnableBlinkFeatures = function (webPreferences?: Electron.WebPreferences) {
  if (!webPreferences ||
    !Object.prototype.hasOwnProperty.call(webPreferences, 'enableBlinkFeatures') ||
    (webPreferences.enableBlinkFeatures != null && webPreferences.enableBlinkFeatures.length === 0)) {
    return;
  }

  const warning = `This renderer process has additional "enableBlinkFeatures"
  enabled. This exposes users of this app to some security risk. If you do not
  need this feature, you should disable it.\n${moreInformation}`;

  console.warn('%cElectron Security Warning (enableBlinkFeatures)',
    'font-weight: bold;', warning);
};

/*%s*/
const warnAboutAllowedPopups = function () {
  if (document && document.querySelectorAll) {
    const domElements = document.querySelectorAll('[allowpopups]');

    if (!domElements || domElements.length === 0) {
      return;
    }

    const warning = `A <webview> has "allowpopups" set to true. This exposes
    users of this app to some security risk, since popups are just
    BrowserWindows. If you do not need this feature, you should disable it.\n
    ${moreInformation}`;

    console.warn('%cElectron Security Warning (allowpopups)',
      'font-weight: bold;', warning);
  }
};

//%s
//%s
//%s
//%s
//%s

const logSecurityWarnings = function (
  webPreferences: Electron.WebPreferences | undefined, nodeIntegration: boolean
) {
  warnAboutNodeWithRemoteContent(nodeIntegration);
  warnAboutDisabledWebSecurity(webPreferences);
  warnAboutInsecureResources();
  warnAboutInsecureContentAllowed(webPreferences);
  warnAboutExperimentalFeatures(webPreferences);
  warnAboutEnableBlinkFeatures(webPreferences);
  warnAboutInsecureCSP();
  warnAboutAllowedPopups();
};

const getWebPreferences = async function () {
  try {
    return ipcRendererInternal.invoke<Electron.WebPreferences>(IPC_MESSAGES.BROWSER_GET_LAST_WEB_PREFERENCES);
  } catch (error) {
    console.warn(`getLastWebPreferences() failed: ${error}`);
  }
};

export function securityWarnings (nodeIntegration = false) {
  const loadHandler = async function () {
    if (shouldLogSecurityWarnings()) {
      const webPreferences = await getWebPreferences();
      logSecurityWarnings(webPreferences, nodeIntegration);
    }
  };
  window.addEventListener('load', loadHandler, { once: true });
}
