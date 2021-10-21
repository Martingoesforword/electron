// 检索有关屏幕大小、显示、光标位置等信息。
// 
// 有关详细信息，请参阅：
// Https://electronjs.org/docs/api/screen。

const { app, BrowserWindow } = require('electron')

let mainWindow = null

app.whenReady().then(() => {
  // 在应用程序准备好之前，我们不能需要屏幕模块。
  const { screen } = require('electron')

  // 创建一个填充屏幕可用工作区的窗口。
  const primaryDisplay = screen.getPrimaryDisplay()
  const { width, height } = primaryDisplay.workAreaSize

  mainWindow = new BrowserWindow({ width, height })
  mainWindow.loadURL('https:// (Electronjs.org‘)
})
