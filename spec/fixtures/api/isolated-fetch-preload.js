const { ipcRenderer } = require('electron');

// 确保从与世隔绝的世界获取工作。
fetch('https:// Localhost：1234‘).catch(err=&gt;{
  ipcRenderer.send('isolated-fetch-error', err.message);
});
