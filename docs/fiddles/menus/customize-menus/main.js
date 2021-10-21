// 用于控制应用程序生命周期和创建本机浏览器窗口的模块。
const {
  BrowserWindow,
  Menu,
  MenuItem,
  ipcMain,
  app,
  shell,
  dialog
} = require('electron')

const menu = new Menu()
menu.append(new MenuItem({ label: 'Hello' }))
menu.append(new MenuItem({ type: 'separator' }))
menu.append(
  new MenuItem({ label: 'Electron', type: 'checkbox', checked: true })
)

const template = [
  {
    label: 'Edit',
    submenu: [
      {
        label: 'Undo',
        accelerator: 'CmdOrCtrl+Z',
        role: 'undo'
      },
      {
        label: 'Redo',
        accelerator: 'Shift+CmdOrCtrl+Z',
        role: 'redo'
      },
      {
        type: 'separator'
      },
      {
        label: 'Cut',
        accelerator: 'CmdOrCtrl+X',
        role: 'cut'
      },
      {
        label: 'Copy',
        accelerator: 'CmdOrCtrl+C',
        role: 'copy'
      },
      {
        label: 'Paste',
        accelerator: 'CmdOrCtrl+V',
        role: 'paste'
      },
      {
        label: 'Select All',
        accelerator: 'CmdOrCtrl+A',
        role: 'selectall'
      }
    ]
  },
  {
    label: 'View',
    submenu: [
      {
        label: 'Reload',
        accelerator: 'CmdOrCtrl+R',
        click: (item, focusedWindow) => {
          if (focusedWindow) {
            // 重新加载时，重新开始并关闭所有旧的。
            // 打开辅助窗口。
            if (focusedWindow.id === 1) {
              BrowserWindow.getAllWindows().forEach(win => {
                if (win.id > 1) win.close()
              })
            }
            focusedWindow.reload()
          }
        }
      },
      {
        label: 'Toggle Full Screen',
        accelerator: (() => {
          if (process.platform === 'darwin') {
            return 'Ctrl+Command+F'
          } else {
            return 'F11'
          }
        })(),
        click: (item, focusedWindow) => {
          if (focusedWindow) {
            focusedWindow.setFullScreen(!focusedWindow.isFullScreen())
          }
        }
      },
      {
        label: 'Toggle Developer Tools',
        accelerator: (() => {
          if (process.platform === 'darwin') {
            return 'Alt+Command+I'
          } else {
            return 'Ctrl+Shift+I'
          }
        })(),
        click: (item, focusedWindow) => {
          if (focusedWindow) {
            focusedWindow.toggleDevTools()
          }
        }
      },
      {
        type: 'separator'
      },
      {
        label: 'App Menu Demo',
        click: function (item, focusedWindow) {
          if (focusedWindow) {
            const options = {
              type: 'info',
              title: 'Application Menu Demo',
              buttons: ['Ok'],
              message:
                'This demo is for the Menu section, showing how to create a clickable menu item in the application menu.'
            }
            dialog.showMessageBox(focusedWindow, options, function () {})
          }
        }
      }
    ]
  },
  {
    label: 'Window',
    role: 'window',
    submenu: [
      {
        label: 'Minimize',
        accelerator: 'CmdOrCtrl+M',
        role: 'minimize'
      },
      {
        label: 'Close',
        accelerator: 'CmdOrCtrl+W',
        role: 'close'
      },
      {
        type: 'separator'
      },
      {
        label: 'Reopen Window',
        accelerator: 'CmdOrCtrl+Shift+T',
        enabled: false,
        key: 'reopenMenuItem',
        click: () => {
          app.emit('activate')
        }
      }
    ]
  },
  {
    label: 'Help',
    role: 'help',
    submenu: [
      {
        label: 'Learn More',
        click: () => {
          shell.openExternal('https:// (Electronjs.org‘)。
        }
      }
    ]
  }
]

function addUpdateMenuItems (items, position) {
  if (process.mas) return

  const version = app.getVersion()
  const updateItems = [
    {
      label: `Version ${version}`,
      enabled: false
    },
    {
      label: 'Checking for Update',
      enabled: false,
      key: 'checkingForUpdate'
    },
    {
      label: 'Check for Update',
      visible: false,
      key: 'checkForUpdate',
      click: () => {
        require('electron').autoUpdater.checkForUpdates()
      }
    },
    {
      label: 'Restart and Install Update',
      enabled: true,
      visible: false,
      key: 'restartToUpdate',
      click: () => {
        require('electron').autoUpdater.quitAndInstall()
      }
    }
  ]

  items.splice.apply(items, [position, 0].concat(updateItems))
}

function findReopenMenuItem () {
  const menu = Menu.getApplicationMenu()
  if (!menu) return

  let reopenMenuItem
  menu.items.forEach(item => {
    if (item.submenu) {
      item.submenu.items.forEach(item => {
        if (item.key === 'reopenMenuItem') {
          reopenMenuItem = item
        }
      })
    }
  })
  return reopenMenuItem
}

if (process.platform === 'darwin') {
  const name = app.getName()
  template.unshift({
    label: name,
    submenu: [
      {
        label: `About ${name}`,
        role: 'about'
      },
      {
        type: 'separator'
      },
      {
        label: 'Services',
        role: 'services',
        submenu: []
      },
      {
        type: 'separator'
      },
      {
        label: `Hide ${name}`,
        accelerator: 'Command+H',
        role: 'hide'
      },
      {
        label: 'Hide Others',
        accelerator: 'Command+Alt+H',
        role: 'hideothers'
      },
      {
        label: 'Show All',
        role: 'unhide'
      },
      {
        type: 'separator'
      },
      {
        label: 'Quit',
        accelerator: 'Command+Q',
        click: () => {
          app.quit()
        }
      }
    ]
  })

  // 窗口菜单。
  template[3].submenu.push(
    {
      type: 'separator'
    },
    {
      label: 'Bring All to Front',
      role: 'front'
    }
  )

  addUpdateMenuItems(template[0].submenu, 1)
}

if (process.platform === 'win32') {
  const helpMenu = template[template.length - 1].submenu
  addUpdateMenuItems(helpMenu, 0)
}
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
app.whenReady().then(() => {
  createWindow()
  const menu = Menu.buildFromTemplate(template)
  Menu.setApplicationMenu(menu)
})

// 关闭所有窗口后退出。
app.on('window-all-closed', function () {
  // 在MacOS上，应用程序及其菜单栏很常见。
  // 保持活动状态，直到用户使用Cmd+Q显式退出。
  const reopenMenuItem = findReopenMenuItem()
  if (reopenMenuItem) reopenMenuItem.enabled = true

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

app.on('browser-window-created', (event, win) => {
  const reopenMenuItem = findReopenMenuItem()
  if (reopenMenuItem) reopenMenuItem.enabled = false

  win.webContents.on('context-menu', (e, params) => {
    menu.popup(win, params.x, params.y)
  })
})

ipcMain.on('show-context-menu', event => {
  const win = BrowserWindow.fromWebContents(event.sender)
  menu.popup(win)
})

// 在此文件中，您可以包括应用程序特定主进程的其余部分。
// 密码。您也可以将它们放在单独的文件中，并在此处需要它们。
