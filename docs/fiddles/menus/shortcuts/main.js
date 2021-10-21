// 用于控制应用程序生命周期和创建本机浏览器窗口的模块。
const { app, BrowserWindow, globalShortcut, dialog } = require('electron')

// 保留窗口对象的全局引用，如果不这样做，窗口将。
// 在对JavaScript对象进行垃圾回收时自动关闭。
let mainWindow

function createWindow () {
  // 创建浏览器窗口。
  mainWindow = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      nodeIntegration: true
    }
  })

  globalShortcut.register('CommandOrControl+Alt+K', () => {
    dialog.showMessageBox({
      type: 'info',
      message: 'Success!',
      detail: 'You pressed the registered global shortcut keybinding.',
      buttons: ['OK']
    })
  })

  // 并加载应用程序的index.html。
  mainWindow.loadFile('index.html')

  // 打开DevTools。
  // MainWindow.webContents.openDevTools()。

  // 在窗口关闭时发出。
  mainWindow.on('closed', function () {
    // 取消对窗口对象的引用，通常会存储窗口。
    // 如果您的应用程序支持多窗口，则此时。
    // 应在何时删除相应的元素。
    mainWindow = null
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

app.on('will-quit', function () {
  globalShortcut.unregisterAll()
})

// 在此文件中，您可以包括应用程序特定主进程的其余部分。
// 密码。您也可以将它们放在单独的文件中，并在此处需要它们。
