// 所有Node.js API都在预加载过程中可用。
// 它拥有与Chrome扩展相同的沙箱。
const { contextBridge, ipcRenderer } = require('electron')

// 在呈现器进程和主进程之间建立上下文桥
contextBridge.exposeInMainWorld(
  'shell',
  {
    open: () => ipcRenderer.send('shell:open'),
  }
)