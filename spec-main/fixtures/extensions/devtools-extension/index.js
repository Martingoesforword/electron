// Eslint-禁用-下一行
chrome.devtools.inspectedWindow.eval(`require("electron").ipcRenderer.send("winning")`, (result, exc) => {
  console.log(result, exc);
});
