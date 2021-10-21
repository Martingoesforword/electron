const { app, BrowserWindow } = require('electron');

let win;
// 此测试使用“app.once(‘Ready’)”，而|test-menu-null|test使用。
// App.When Ready()，这两个接口在覆盖的时序上略有不同。
// 更多的案子。
app.once('ready', function () {
  win = new BrowserWindow({});
  win.setMenuBarVisibility(false);

  setTimeout(() => {
    if (win.isMenuBarVisible()) {
      console.log('Window has a menu');
    } else {
      console.log('Window has no menu');
    }
    app.quit();
  });
});
