const { WebContentsView, app } = require('electron');
app.whenReady().then(function () {
  new WebContentsView({})  // ESRINT-DISABLE-LINE

  app.quit();
});
