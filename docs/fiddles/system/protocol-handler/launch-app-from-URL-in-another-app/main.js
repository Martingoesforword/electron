// 用于控制应用程序生命周期和创建本机浏览器窗口的模块。
const { app, BrowserWindow, ipcMain, shell, dialog } = require('electron')
const path = require('path')

let mainWindow;

if (process.defaultApp) {
  if (process.argv.length >= 2) {
    app.setAsDefaultProtocolClient('electron-fiddle', process.execPath, [path.resolve(process.argv[1])])
  }
} else {
    app.setAsDefaultProtocolClient('electron-fiddle')
}

const gotTheLock = app.requestSingleInstanceLock()

if (!gotTheLock) {
  app.quit()
} else {
  app.on('second-instance', (event, commandLine, workingDirectory) => {
    // 有人试图运行第二个实例，我们应该聚焦窗口。
    if (mainWindow) {
      if (mainWindow.isMinimized()) mainWindow.restore()
      mainWindow.focus()
    }
  })

  // 创建mainWindow，加载应用程序的其余部分，等等。
  app.whenReady().then(() => {
    createWindow()
  })
  
  app.on('open-url', (event, url) => {
    dialog.showErrorBox('Welcome Back', `You arrived from: ${url}`)
  })
}

function createWindow () {
  // 创建浏览器窗口。
  mainWindow = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      preload: path.join(__dirname, 'preload.js'),
    }
  })

  mainWindow.loadFile('index.html')
}

// 关闭所有窗口(MacOS除外)后退出。在那里，这是很常见的。
// 使应用程序及其菜单栏保持活动状态，直到用户退出。
// 显式使用Cmd+Q。
app.on('window-all-closed', function () {
  if (process.platform !== 'darwin') app.quit()
})

// 通过IPC处理窗口控件。
ipcMain.on('shell:open', () => {
  const pageDirectory = __dirname.replace('app.asar', 'app.asar.unpacked')
  const pagePath = path.join('file:// ‘，pageDirectory，’index.html‘)
  shell.openExternal(pagePath)
})
