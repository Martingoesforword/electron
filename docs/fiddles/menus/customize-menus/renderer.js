const { ipcRenderer } = require('electron')

// 告诉主进程在单击演示按钮时显示菜单
const contextMenuBtn = document.getElementById('context-menu')

contextMenuBtn.addEventListener('click', () => {
  ipcRenderer.send('show-context-menu')
})
