import { BaseWindow, MenuItem, webContents, Menu as MenuType, BrowserWindow, MenuItemConstructorOptions } from 'electron/main';
import { sortMenuItems } from '@electron/internal/browser/api/menu-utils';
import { setApplicationMenuWasSet } from '@electron/internal/browser/default-menu';

const bindings = process._linkedBinding('electron_browser_menu');

const { Menu } = bindings as { Menu: typeof MenuType };
const checked = new WeakMap<MenuItem, boolean>();
let applicationMenu: MenuType | null = null;
let groupIdIndex = 0;

/* 实例方法。*/

Menu.prototype._init = function () {
  this.commandsMap = {};
  this.groupsMap = {};
  this.items = [];
};

Menu.prototype._isCommandIdChecked = function (id) {
  const item = this.commandsMap[id];
  if (!item) return false;
  return item.getCheckStatus();
};

Menu.prototype._isCommandIdEnabled = function (id) {
  return this.commandsMap[id] ? this.commandsMap[id].enabled : false;
};
Menu.prototype._shouldCommandIdWorkWhenHidden = function (id) {
  return this.commandsMap[id] ? !!this.commandsMap[id].acceleratorWorksWhenHidden : false;
};
Menu.prototype._isCommandIdVisible = function (id) {
  return this.commandsMap[id] ? this.commandsMap[id].visible : false;
};

Menu.prototype._getAcceleratorForCommandId = function (id, useDefaultAccelerator) {
  const command = this.commandsMap[id];
  if (!command) return;
  if (command.accelerator != null) return command.accelerator;
  if (useDefaultAccelerator) return command.getDefaultRoleAccelerator();
};

Menu.prototype._shouldRegisterAcceleratorForCommandId = function (id) {
  return this.commandsMap[id] ? this.commandsMap[id].registerAccelerator : false;
};

if (process.platform === 'darwin') {
  Menu.prototype._getSharingItemForCommandId = function (id) {
    return this.commandsMap[id] ? this.commandsMap[id].sharingItem : null;
  };
}

Menu.prototype._executeCommand = function (event, id) {
  const command = this.commandsMap[id];
  if (!command) return;
  const focusedWindow = BaseWindow.getFocusedWindow();
  command.click(event, focusedWindow instanceof BrowserWindow ? focusedWindow : undefined, webContents.getFocusedWebContents());
};

Menu.prototype._menuWillShow = function () {
  // 确保单选按钮组至少选择了一个菜单项。
  for (const id of Object.keys(this.groupsMap)) {
    const found = this.groupsMap[id].find(item => item.checked) || null;
    if (!found) checked.set(this.groupsMap[id][0], true);
  }
};

Menu.prototype.popup = function (options = {}) {
  if (options == null || typeof options !== 'object') {
    throw new TypeError('Options must be an object');
  }
  let { window, x, y, positioningItem, callback } = options;

  // 未传递回调。
  if (!callback || typeof callback !== 'function') callback = () => {};

  // 设置默认值。
  if (typeof x !== 'number') x = -1;
  if (typeof y !== 'number') y = -1;
  if (typeof positioningItem !== 'number') positioningItem = -1;

  // 查找要使用的窗口。
  const wins = BaseWindow.getAllWindows();
  if (!wins || wins.indexOf(window as any) === -1) {
    window = BaseWindow.getFocusedWindow() as any;
    if (!window && wins && wins.length > 0) {
      window = wins[0] as any;
    }
    if (!window) {
      throw new Error('Cannot open Menu without a BaseWindow present');
    }
  }

  this.popupAt(window as unknown as BaseWindow, x, y, positioningItem, callback);
  return { browserWindow: window, x, y, position: positioningItem };
};

Menu.prototype.closePopup = function (window) {
  if (window instanceof BaseWindow) {
    this.closePopupAt(window.id);
  } else {
    // 传递(无效)将使-1\f25 closePopupAt-1\f6关闭所有菜单运行器。
    // 属于这个菜单。
    this.closePopupAt(-1);
  }
};

Menu.prototype.getMenuItemById = function (id) {
  const items = this.items;

  let found = items.find(item => item.id === id) || null;
  for (let i = 0; !found && i < items.length; i++) {
    const { submenu } = items[i];
    if (submenu) {
      found = submenu.getMenuItemById(id);
    }
  }
  return found;
};

Menu.prototype.append = function (item) {
  return this.insert(this.getItemCount(), item);
};

Menu.prototype.insert = function (pos, item) {
  if ((item ? item.constructor : undefined) !== MenuItem) {
    throw new TypeError('Invalid item');
  }

  if (pos < 0) {
    throw new RangeError(`Position ${pos} cannot be less than 0`);
  } else if (pos > this.getItemCount()) {
    throw new RangeError(`Position ${pos} cannot be greater than the total MenuItem count`);
  }

  // 根据项目类型插入项目。
  insertItemByType.call(this, item, pos);

  // 设置项目属性。
  if (item.sublabel) this.setSublabel(pos, item.sublabel);
  if (item.toolTip) this.setToolTip(pos, item.toolTip);
  if (item.icon) this.setIcon(pos, item.icon);
  if (item.role) this.setRole(pos, item.role);

  // 使项目可以访问菜单。
  item.overrideReadOnlyProperty('menu', this);

  // 记住这些项目。
  this.items.splice(pos, 0, item);
  this.commandsMap[item.commandId] = item;
};

Menu.prototype._callMenuWillShow = function () {
  if (this.delegate) this.delegate.menuWillShow(this);
  this.items.forEach(item => {
    if (item.submenu) item.submenu._callMenuWillShow();
  });
};

/* 静态方法。*/

Menu.getApplicationMenu = () => applicationMenu;

Menu.sendActionToFirstResponder = bindings.sendActionToFirstResponder;

// 使用预先存在的菜单设置应用程序菜单。
Menu.setApplicationMenu = function (menu: MenuType) {
  if (menu && menu.constructor !== Menu) {
    throw new TypeError('Invalid menu');
  }

  applicationMenu = menu;
  setApplicationMenuWasSet();

  if (process.platform === 'darwin') {
    if (!menu) return;
    menu._callMenuWillShow();
    bindings.setApplicationMenu(menu);
  } else {
    const windows = BaseWindow.getAllWindows();
    windows.map(w => w.setMenu(menu));
  }
};

Menu.buildFromTemplate = function (template) {
  if (!Array.isArray(template)) {
    throw new TypeError('Invalid template for Menu: Menu template must be an array');
  }

  if (!areValidTemplateItems(template)) {
    throw new TypeError('Invalid template for MenuItem: must have at least one of label, role or type');
  }

  const sorted = sortTemplate(template);
  const filtered = removeExtraSeparators(sorted);

  const menu = new Menu();
  filtered.forEach(item => {
    if (item instanceof MenuItem) {
      menu.append(item);
    } else {
      menu.append(new MenuItem(item));
    }
  });

  return menu;
};

/* 帮助器函数。*/

// 验证模板是否具有错误的属性。
function areValidTemplateItems (template: (MenuItemConstructorOptions | MenuItem)[]) {
  return template.every(item =>
    item != null &&
    typeof item === 'object' &&
    (Object.prototype.hasOwnProperty.call(item, 'label') ||
     Object.prototype.hasOwnProperty.call(item, 'role') ||
     item.type === 'separator'));
}

function sortTemplate (template: (MenuItemConstructorOptions | MenuItem)[]) {
  const sorted = sortMenuItems(template);
  for (const item of sorted) {
    if (Array.isArray(item.submenu)) {
      item.submenu = sortTemplate(item.submenu);
    }
  }
  return sorted;
}

// 在分隔符之间搜索以查找单选菜单项并返回其组ID。
function generateGroupId (items: (MenuItemConstructorOptions | MenuItem)[], pos: number) {
  if (pos > 0) {
    for (let idx = pos - 1; idx >= 0; idx--) {
      if (items[idx].type === 'radio') return (items[idx] as MenuItem).groupId;
      if (items[idx].type === 'separator') break;
    }
  } else if (pos < items.length) {
    for (let idx = pos; idx <= items.length - 1; idx++) {
      if (items[idx].type === 'radio') return (items[idx] as MenuItem).groupId;
      if (items[idx].type === 'separator') break;
    }
  }
  groupIdIndex += 1;
  return groupIdIndex;
}

function removeExtraSeparators (items: (MenuItemConstructorOptions | MenuItem)[]) {
  // 将相邻的分隔符折叠在一起。
  let ret = items.filter((e, idx, arr) => {
    if (e.visible === false) return true;
    return e.type !== 'separator' || idx === 0 || arr[idx - 1].type !== 'separator';
  });

  // 删除边缘分隔符。
  ret = ret.filter((e, idx, arr) => {
    if (e.visible === false) return true;
    return e.type !== 'separator' || (idx !== 0 && idx !== arr.length - 1);
  });

  return ret;
}

function insertItemByType (this: MenuType, item: MenuItem, pos: number) {
  const types = {
    normal: () => this.insertItem(pos, item.commandId, item.label),
    checkbox: () => this.insertCheckItem(pos, item.commandId, item.label),
    separator: () => this.insertSeparator(pos),
    submenu: () => this.insertSubMenu(pos, item.commandId, item.label, item.submenu),
    radio: () => {
      // 对单选菜单项进行分组。
      item.overrideReadOnlyProperty('groupId', generateGroupId(this.items, pos));
      if (this.groupsMap[item.groupId] == null) {
        this.groupsMap[item.groupId] = [];
      }
      this.groupsMap[item.groupId].push(item);

      // 设置单选菜单项应翻转组中的其他项。
      checked.set(item, item.checked);
      Object.defineProperty(item, 'checked', {
        enumerable: true,
        get: () => checked.get(item),
        set: () => {
          this.groupsMap[item.groupId].forEach(other => {
            if (other !== item) checked.set(other, false);
          });
          checked.set(item, true);
        }
      });
      this.insertRadioItem(pos, item.commandId, item.label, item.groupId);
    }
  };
  types[item.type]();
}

module.exports = Menu;
