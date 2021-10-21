import { internalContextBridge } from '@electron/internal/renderer/api/context-bridge';
import { ipcRendererInternal } from '@electron/internal/renderer/ipc-renderer-internal';
import * as ipcRendererUtils from '@electron/internal/renderer/ipc-renderer-internal-utils';
import { webFrame } from 'electron/renderer';
import { IPC_MESSAGES } from '../common/ipc-messages';

const { contextIsolationEnabled } = internalContextBridge;

/* 更正了在电子战中需要调整的一些检验者。*1)使用Menu接口显示上下文菜单。*2)更正文件系统Chromium返回UNDEFINED。*3)使用DIALOG API覆盖文件选择器对话框。*/
window.onload = function () {
  if (contextIsolationEnabled) {
    internalContextBridge.overrideGlobalValueFromIsolatedWorld([
      'InspectorFrontendHost', 'showContextMenuAtPoint'
    ], createMenu);
    internalContextBridge.overrideGlobalValueFromIsolatedWorld([
      'Persistence', 'FileSystemWorkspaceBinding', 'completeURL'
    ], completeURL);
    internalContextBridge.overrideGlobalValueFromIsolatedWorld([
      'UI', 'createFileSelectorElement'
    ], createFileSelectorElement);
  } else {
    window.InspectorFrontendHost!.showContextMenuAtPoint = createMenu;
    window.Persistence!.FileSystemWorkspaceBinding.completeURL = completeURL;
    window.UI!.createFileSelectorElement = createFileSelectorElement;
  }
};

// 由于MacOS需要绝对路径，因此需要额外的/。
function completeURL (project: string, path: string) {
  project = 'file:// /‘；
  return `${project}${path}`;
}

// DOM实现需要(message？：string)=&gt;布尔值
window.confirm = function (message?: string, title?: string) {
  return ipcRendererUtils.invokeSync(IPC_MESSAGES.INSPECTOR_CONFIRM, message, title) as boolean;
};

const useEditMenuItems = function (x: number, y: number, items: ContextMenuItem[]) {
  return items.length === 0 && document.elementsFromPoint(x, y).some(function (element) {
    return element.nodeName === 'INPUT' ||
      element.nodeName === 'TEXTAREA' ||
      (element as HTMLElement).isContentEditable;
  });
};

const createMenu = function (x: number, y: number, items: ContextMenuItem[]) {
  const isEditMenu = useEditMenuItems(x, y, items);
  ipcRendererInternal.invoke<number>(IPC_MESSAGES.INSPECTOR_CONTEXT_MENU, items, isEditMenu).then(id => {
    if (typeof id === 'number') {
      webFrame.executeJavaScript(`window.DevToolsAPI.contextMenuItemSelected(${JSON.stringify(id)})`);
    }

    webFrame.executeJavaScript('window.DevToolsAPI.contextMenuCleared()');
  });
};

const showFileChooserDialog = function (callback: (blob: File) => void) {
  ipcRendererInternal.invoke<[ string, any ]>(IPC_MESSAGES.INSPECTOR_SELECT_FILE).then(([path, data]) => {
    if (path && data) {
      callback(dataToHtml5FileObject(path, data));
    }
  });
};

const dataToHtml5FileObject = function (path: string, data: any) {
  return new File([data], path);
};

const createFileSelectorElement = function (this: any, callback: () => void) {
  const fileSelectorElement = document.createElement('span');
  fileSelectorElement.style.display = 'none';
  fileSelectorElement.click = showFileChooserDialog.bind(this, callback);
  return fileSelectorElement;
};
