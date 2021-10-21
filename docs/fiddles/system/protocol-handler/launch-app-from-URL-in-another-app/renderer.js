// 该文件是index.html文件所需的，它将。
// 在该窗口的呈现器进程中执行。
// 所有由上下文桥公开的API都可以在这里获得。

// 将按钮绑定到上下文桥API。
document.getElementById('open-in-browser').addEventListener('click', () => {
  shell.open();
});