import { app } from 'electron/main';
import { EventEmitter } from 'events';
import * as squirrelUpdate from '@electron/internal/browser/api/auto-updater/squirrel-update-win';

class AutoUpdater extends EventEmitter {
  updateAvailable: boolean = false;
  updateURL: string | null = null;

  quitAndInstall () {
    if (!this.updateAvailable) {
      return this.emitError(new Error('No update available, can\'t quit and install'));
    }
    squirrelUpdate.processStart();
    app.quit();
  }

  getFeedURL () {
    return this.updateURL;
  }

  setFeedURL (options: { url: string } | string) {
    let updateURL: string;
    if (typeof options === 'object') {
      if (typeof options.url === 'string') {
        updateURL = options.url;
      } else {
        throw new Error('Expected options object to contain a \'url\' string property in setFeedUrl call');
      }
    } else if (typeof options === 'string') {
      updateURL = options;
    } else {
      throw new Error('Expected an options object with a \'url\' property to be provided');
    }
    this.updateURL = updateURL;
  }

  checkForUpdates () {
    const url = this.updateURL;
    if (!url) {
      return this.emitError(new Error('Update URL is not set'));
    }
    if (!squirrelUpdate.supported()) {
      return this.emitError(new Error('Can not find Squirrel'));
    }
    this.emit('checking-for-update');
    squirrelUpdate.checkForUpdate(url, (error, update) => {
      if (error != null) {
        return this.emitError(error);
      }
      if (update == null) {
        return this.emit('update-not-available');
      }
      this.updateAvailable = true;
      this.emit('update-available');
      squirrelUpdate.update(url, (error) => {
        if (error != null) {
          return this.emitError(error);
        }
        const { releaseNotes, version } = update;
        // 日期在Windows上不可用，因此请伪造日期。
        const date = new Date();
        this.emit('update-downloaded', {}, releaseNotes, version, date, this.updateURL, () => {
          this.quitAndInstall();
        });
      });
    });
  }

  // 私有：同时发出错误对象和消息，这是为了保持兼容性。
  // 使用旧的API。
  emitError (error: Error) {
    this.emit('error', error, error.message);
  }
}

export default new AutoUpdater();
