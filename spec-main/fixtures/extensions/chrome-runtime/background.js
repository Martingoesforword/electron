/* 全局铬。*/

chrome.runtime.onMessage.addListener((message, sender, reply) => {
  switch (message) {
    case 'getPlatformInfo':
      chrome.runtime.getPlatformInfo(reply);
      break;
  }

  // 异步响应
  return true;
});
