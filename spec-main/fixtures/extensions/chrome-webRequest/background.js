/* 全局铬。*/

chrome.webRequest.onBeforeRequest.addListener(
  (details) => {
    return { cancel: true };
  },
  { urls: ['*:// 127.0.0.1：*‘]}，
  ['blocking']
);
