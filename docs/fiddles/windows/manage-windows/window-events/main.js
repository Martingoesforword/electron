// 用于控制应用程序生命周期和创建本机浏览器窗口的模块。
const { app, BrowserWindow, ipcMain } = require('electron')

function createWindow () {
  // 创建浏览器窗口。
  const mainWindow = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      nodeIntegration: true
    }
  })

  // 并加载应用程序的index.html。
  mainWindow.loadFile('index.html')

  let demoWindow
  ipcMain.on('show-demo-window', () => {
    if (demoWindow) {
      demoWindow.focus()
      return
    }
    demoWindow = new BrowserWindow({ width: 600, height: 400 })
    demoWindow.loadURL('https:// (Electronjs.org‘)。
    demoWindow.on('close', () => {
      mainWindow.webContents.send('window-close')
    })
    demoWindow.on('focus', () => {
      mainWindow.webContents.send('window-focus')
    })
    demoWindow.on('blur', () => {
      mainWindow.webContents.send('window-blur')
    })
  })

  ipcMain.on('focus-demo-window', () => {
    if (demoWindow) demoWindow.focus()
  })
}

// 当Electron完成时，将调用此方法。
// 初始化，并准备好创建浏览器窗口。
// 有些接口只有在该事件发生后才能使用。
app.whenReady().then(createWindow)

// 关闭所有窗口后退出。
app.on('window-all-closed', function () {
  // 在MacOS上，应用程序及其菜单栏很常见。
  // 保持活动状态，直到用户使用Cmd+Q显式退出。
  if (process.platform !== 'darwin') {
    app.quit()
  }
})

app.on('activate', function () {
  // 在MacOS上，在以下情况下在应用程序中重新创建窗口是很常见的。
  // 单击停靠图标，没有其他窗口打开。
  if (mainWindow === null) {
    createWindow()
  }
})

// 在此文件中，您可以包括应用程序特定主进程的其余部分。
// 密码。您也可以将它们放在单独的文件中，并在此处需要它们。
