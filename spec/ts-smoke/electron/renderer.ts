
import {
  desktopCapturer,
  ipcRenderer,
  webFrame,
  clipboard,
  crashReporter,
  shell
} from 'electron'

import * as fs from 'fs'

// 在渲染器进程中(网页)。
// Https://github.com/electron/electron/blob/master/docs/api/ipc-renderer.md。
console.log(ipcRenderer.sendSync('synchronous-message', 'ping')) // 印花“乒乓”

ipcRenderer.on('asynchronous-reply', (event, arg: any) => {
  console.log(arg) // 印花“乒乓”
  event.sender.send('another-message', 'Hello World!')
})

ipcRenderer.send('asynchronous-message', 'ping')

// Web框架。
// Https://github.com/electron/electron/blob/master/docs/api/web-frame.md。

webFrame.setZoomFactor(2)
console.log(webFrame.getZoomFactor())

webFrame.setZoomLevel(200)
console.log(webFrame.getZoomLevel())

webFrame.setVisualZoomLevelLimits(50, 200)

webFrame.setSpellCheckProvider('en-US', {
  spellCheck (words, callback) {
    setTimeout(() => {
      const spellchecker = require('spellchecker')
      const misspelled = words.filter(x => spellchecker.isMisspelled(x))
      callback(misspelled)
    }, 0)
  }
})

webFrame.insertText('text')

webFrame.executeJavaScript('return true;').then((v: boolean) => console.log(v))
webFrame.executeJavaScript('return true;', true).then((v: boolean) => console.log(v))
webFrame.executeJavaScript('return true;', true)
webFrame.executeJavaScript('return true;', true).then((result: boolean) => console.log(result))

console.log(webFrame.getResourceUsage())
webFrame.clearCache()

// 剪贴板。
// Https://github.com/electron/electron/blob/master/docs/api/clipboard.md。

clipboard.writeText('Example String')
clipboard.writeText('Example String', 'selection')
console.log(clipboard.readText('selection'))
console.log(clipboard.availableFormats())
clipboard.clear()

clipboard.write({
  html: '<html></html>',
  text: 'Hello World!',
  bookmark: 'Bookmark name',
  image: clipboard.readImage()
})

// 撞车记者。
// Https://github.com/electron/electron/blob/master/docs/api/crash-reporter.md。

crashReporter.start({
  productName: 'YourName',
  companyName: 'YourCompany',
  submitURL: 'https:// Your-domain.com/url-to-submit‘，
  uploadToServer: true
})

// 台式机捕获器。
// Https://github.com/electron/electron/blob/master/docs/api/desktop-capturer.md。

desktopCapturer.getSources({ types: ['window', 'screen'] }).then(sources => {
  for (let i = 0; i < sources.length; ++i) {
    if (sources[i].name == 'Electron') {
      (navigator as any).webkitGetUserMedia({
        audio: false,
        video: {
          mandatory: {
            chromeMediaSource: 'desktop',
            chromeMediaSourceId: sources[i].id,
            minWidth: 1280,
            maxWidth: 1280,
            minHeight: 720,
            maxHeight: 720
          }
        }
      }, gotStream, getUserMediaError)
      return
    }
  }
})

function gotStream (stream: any) {
  (document.querySelector('video') as HTMLVideoElement).src = URL.createObjectURL(stream)
}

function getUserMediaError (error: Error) {
  console.log('getUserMediaError', error)
}

// 文件对象。
// Https://github.com/electron/electron/blob/master/docs/api/file-object.md。

/* <div>将您的文件拖到此处</div>。*/

const holder = document.getElementById('holder')

holder.ondragover = function () {
  return false
}

holder.ondragleave = holder.ondragend = function () {
  return false
}

holder.ondrop = function (e) {
  e.preventDefault()
  const file = e.dataTransfer.files[0]
  console.log('File you dragged here is', file.path)
  return false
}

// 原生图像。
// Https://github.com/electron/electron/blob/master/docs/api/native-image.md。

const image = clipboard.readImage()

// Https://github.com/electron/electron/blob/master/docs/api/process.md。

// Preload.js。
const _setImmediate = setImmediate
const _clearImmediate = clearImmediate
process.once('loaded', function () {
  global.setImmediate = _setImmediate
  global.clearImmediate = _clearImmediate
})

// 壳。
// Https://github.com/electron/electron/blob/master/docs/api/shell.md。

shell.openExternal('https:// Github.com‘).然后(()=&gt;{})。

// &lt;WebView&gt;。
// Https://github.com/electron/electron/blob/master/docs/api/web-view-tag.md。

const webview = document.createElement('webview')
webview.loadURL('https:// Github.com‘)。

webview.addEventListener('console-message', function (e) {
  console.log('Guest page logged a message:', e.message)
})

webview.addEventListener('found-in-page', function (e) {
  if (e.result.finalUpdate) {
    webview.stopFindInPage('keepSelection')
  }
})

const requestId = webview.findInPage('test')

webview.addEventListener('new-window', async e => {
  const { shell } = require('electron')
  await shell.openExternal(e.url)
})

webview.addEventListener('close', function () {
  webview.src = 'about:blank'
})

// 在嵌入器页面中。
webview.addEventListener('ipc-message', function (event) {
  console.log(event.channel) // 印花“乒乓”
})
webview.send('ping')
webview.capturePage().then(image => { console.log(image) })

{
  const opened: boolean = webview.isDevToolsOpened()
  const focused: boolean = webview.isDevToolsFocused()
}

// 在来宾页面。
ipcRenderer.on('ping', function () {
  ipcRenderer.sendToHost('pong')
})
