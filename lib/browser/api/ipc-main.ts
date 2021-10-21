import { IpcMainImpl } from '@electron/internal/browser/ipc-main-impl';

const ipcMain = new IpcMainImpl();

// 当通道名称为“Error”时，不要抛出异常。
ipcMain.on('error', () => {});

export default ipcMain;
