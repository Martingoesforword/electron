/* **用于解析浏览器API中使用的逗号分隔的键值对的实用程序。*例如：“x=100，y=200，宽度=500，高度=500”*/
import { BrowserWindowConstructorOptions } from 'electron';

type RequiredBrowserWindowConstructorOptions = Required<BrowserWindowConstructorOptions>;
type IntegerBrowserWindowOptionKeys = {
  [K in keyof RequiredBrowserWindowConstructorOptions]:
    RequiredBrowserWindowConstructorOptions[K] extends number ? K : never
}[keyof RequiredBrowserWindowConstructorOptions];

// 这可以是键的数组，但是对象允许我们添加编译时。
// 检查是否未将整数属性添加到。
// 此模块不知道的BrowserWindowConstructorOptions。
const keysOfTypeNumberCompileTimeCheck: { [K in IntegerBrowserWindowOptionKeys] : true } = {
  x: true,
  y: true,
  width: true,
  height: true,
  minWidth: true,
  maxWidth: true,
  minHeight: true,
  maxHeight: true,
  opacity: true
};
// 注意：`top`/`left`是浏览器的特例，我们稍后会对其进行转换。
// 到y/x。
const keysOfTypeNumber = ['top', 'left', ...Object.keys(keysOfTypeNumberCompileTimeCheck)];

/* **请注意，当类型已知*不是整数时，我们只允许“0”和“1”布尔转换。**YES/NO/1/0的强制表示根据规范尽最大努力：*https://html.spec.whatwg.org/multipage/window-object.html#concept-window-open-features-parse-boolean。*/
type CoercedValue = string | number | boolean;
function coerce (key: string, value: string): CoercedValue {
  if (keysOfTypeNumber.includes(key)) {
    return parseInt(value, 10);
  }

  switch (value) {
    case 'true':
    case '1':
    case 'yes':
    case undefined:
      return true;
    case 'false':
    case '0':
    case 'no':
      return false;
    default:
      return value;
  }
}

export function parseCommaSeparatedKeyValue (source: string) {
  const parsed = {} as { [key: string]: any };
  for (const keyValuePair of source.split(',')) {
    const [key, value] = keyValuePair.split('=').map(str => str.trim());
    if (key) { parsed[key] = coerce(key, value); }
  }

  return parsed;
}

export function parseWebViewWebPreferences (preferences: string) {
  return parseCommaSeparatedKeyValue(preferences);
}

const allowedWebPreferences = ['zoomFactor', 'nodeIntegration', 'javascript', 'contextIsolation', 'webviewTag'] as const;
type AllowedWebPreference = (typeof allowedWebPreferences)[number];

/* **解析window.open()中使用的格式的功能字符串。*/
export function parseFeatures (features: string) {
  const parsed = parseCommaSeparatedKeyValue(features);

  const webPreferences: { [K in AllowedWebPreference]?: any } = {};
  allowedWebPreferences.forEach((key) => {
    if (parsed[key] === undefined) return;
    webPreferences[key] = parsed[key];
    delete parsed[key];
  });

  if (parsed.left !== undefined) parsed.x = parsed.left;
  if (parsed.top !== undefined) parsed.y = parsed.top;

  return {
    options: parsed as Omit<BrowserWindowConstructorOptions, 'webPreferences'>,
    webPreferences
  };
}
