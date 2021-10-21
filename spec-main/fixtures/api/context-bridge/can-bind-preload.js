const { contextBridge, ipcRenderer } = require('electron');

console.info(contextBridge);

let bound = false;
try {
  contextBridge.exposeInMainWorld('test', {});
  bound = true;
} catch {
  // 忽略
}

ipcRenderer.send('context-bridge-bound', bound);
