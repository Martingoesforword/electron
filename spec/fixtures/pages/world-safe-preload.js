const { ipcRenderer, webFrame } = require('electron');

webFrame.executeJavaScript(`(() => {
  return {};
})()`).then((obj) => {
  // 如果对象是在这个世界上构造的，则被认为是安全的
  ipcRenderer.send('executejs-safe', obj.constructor === Object);
});
