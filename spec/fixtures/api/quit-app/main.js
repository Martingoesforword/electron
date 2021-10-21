const { app } = require('electron');

app.whenReady().then(function () {
  // 此setImmediate调用获取Linux上传递的规范
  setImmediate(function () {
    app.exit(123);
  });
});

process.on('exit', function (code) {
  console.log('Exit event with code: ' + code);
});
