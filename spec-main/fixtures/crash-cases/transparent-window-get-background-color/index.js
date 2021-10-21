const { app, BrowserWindow } = require('electron');

function createWindow () {
  // 创建浏览器窗口。
  const mainWindow = new BrowserWindow({
    transparent: true
  });
  mainWindow.getBackgroundColor();
}

app.on('ready', () => {
  createWindow();
  setTimeout(() => app.quit());
});
