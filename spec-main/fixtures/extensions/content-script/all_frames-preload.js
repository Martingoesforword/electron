const { ipcRenderer, webFrame } = require('electron');

if (process.isMainFrame) {
  // Https
  ipcRenderer.on('executeJavaScriptInFrame', (event, frameRoutingId, code, responseId) => {
    const frame = webFrame.findFrameByRoutingId(frameRoutingId);
    if (!frame) {
      throw new Error(`Can't find frame for routing ID ${frameRoutingId}`);
    }
    frame.executeJavaScript(code, false).then(result => {
      event.sender.send(`executeJavaScriptInFrame_${responseId}`, result);
    });
  });
}
