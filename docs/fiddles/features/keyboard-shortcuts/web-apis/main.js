// 用于控制应用程序生命周期和创建本机浏览器窗口的模块。
const {app, BrowserWindow} = require('electron')
const path = require('path')

function createWindow () {
  // 创建浏览器窗口。
  const mainWindow = new BrowserWindow({
    width: 800,
    height: 600,
  })

  // 并加载应用程序的index.html。
  mainWindow.loadFile('index.html')

}

// 当Electron完成时，将调用此方法。
// 初始化，并准备好创建浏览器窗口。
// 有些接口只有在该事件发生后才能使用。
app.whenReady().then(() => {
  createWindow()
  
  app.on('activate', function () {
    // 在MacOS上，在以下情况下在应用程序中重新创建窗口是很常见的。
    // 单击停靠图标，没有其他窗口打开。
    if (BrowserWindow.getAllWindows().length === 0) createWindow()
  })
})

// 关闭所有窗口(MacOS除外)后退出。在那里，这是很常见的。
// 使应用程序及其菜单栏保持活动状态，直到用户退出。
// 显式使用Cmd+Q。
app.on('window-all-closed', function () {
  if (process.platform !== 'darwin') app.quit()
})
