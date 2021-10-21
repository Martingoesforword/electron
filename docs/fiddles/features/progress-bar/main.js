const { app, BrowserWindow } = require('electron')

let progressInterval

function createWindow () {
  const win = new BrowserWindow({
    width: 800,
    height: 600
  })

  win.loadFile('index.html')

  const INCREMENT = 0.03
  const INTERVAL_DELAY = 100 // 女士。

  let c = 0
  progressInterval = setInterval(() => {
    // 将进度条更新为下一个值。
    // 介于0和1之间的值将显示进度，&gt;1将显示不确定或保持在100%。
    win.setProgressBar(c)

    // 增加或重置进度条。
    if (c < 2) {
      c += INCREMENT
    } else {
      c = (-INCREMENT * 5) // 重置为小于0的位以显示重置状态。
    }
  }, INTERVAL_DELAY)
}

app.whenReady().then(createWindow)

// 在应用程序终止之前，清除两个计时器
app.on('before-quit', () => {
  clearInterval(progressInterval)
})

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') {
    app.quit()
  }
})

app.on('activate', () => {
  if (BrowserWindow.getAllWindows().length === 0) {
    createWindow()
  }
})
