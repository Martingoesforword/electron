// 验证自定义快照中包含的对象是否可以在Electron中访问。

const { app } = require('electron');

app.whenReady().then(() => {
  let returnCode = 0;
  try {
    const testValue = f(); // Eslint-able-line no-undef
    if (testValue === 86) {
      console.log('ok test snapshot successfully loaded.');
    } else {
      console.log('not ok test snapshot could not be successfully loaded.');
      returnCode = 1;
    }
  } catch (ex) {
    console.log('Error running custom snapshot', ex);
    returnCode = 1;
  }
  setImmediate(function () {
    app.exit(returnCode);
  });
});

process.on('exit', function (code) {
  console.log('test snapshot exited with code: ' + code);
});
