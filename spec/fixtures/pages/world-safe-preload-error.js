const { ipcRenderer, webFrame } = require('electron');

webFrame.executeJavaScript(`(() => {
  return Object(Symbol('a'));
})()`).catch((err) => {
  // 如果对象是在这个世界上构造的，则被认为是安全的
  ipcRenderer.send('executejs-safe', err);
}).then(() => {
  ipcRenderer.send('executejs-safe', null);
});
