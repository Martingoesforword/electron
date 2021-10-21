let deprecationHandler: ElectronInternal.DeprecationHandler | null = null;

function warnOnce (oldName: string, newName?: string) {
  let warned = false;
  const msg = newName
    ? `'${oldName}' is deprecated and will be removed. Please use '${newName}' instead.`
    : `'${oldName}' is deprecated and will be removed.`;
  return () => {
    if (!warned && !process.noDeprecation) {
      warned = true;
      deprecate.log(msg);
    }
  };
}

const deprecate: ElectronInternal.DeprecationUtil = {
  warnOnce,
  setHandler: (handler) => { deprecationHandler = handler; },
  getHandler: () => deprecationHandler,
  warn: (oldName, newName) => {
    if (!process.noDeprecation) {
      deprecate.log(`'${oldName}' is deprecated. Use '${newName}' instead.`);
    }
  },
  log: (message) => {
    if (typeof deprecationHandler === 'function') {
      deprecationHandler(message);
    } else if (process.throwDeprecation) {
      throw new Error(message);
    } else if (process.traceDeprecation) {
      return console.trace(message);
    } else {
      return console.warn(`(electron) ${message}`);
    }
  },

  // 在不替换的情况下删除函数。
  removeFunction: (fn, removedName) => {
    if (!fn) { throw Error(`'${removedName} function' is invalid or does not exist.`); }

    // 包装不推荐使用的函数以警告用户。
    const warn = warnOnce(`${fn.name} function`);
    return function (this: any) {
      warn();
      fn.apply(this, arguments);
    } as unknown as typeof fn;
  },

  // 更改函数的名称。
  renameFunction: (fn, newName) => {
    const warn = warnOnce(`${fn.name} function`, `${newName} function`);
    return function (this: any) {
      warn();
      return fn.apply(this, arguments);
    } as unknown as typeof fn;
  },

  moveAPI<T extends Function> (fn: T, oldUsage: string, newUsage: string): T {
    const warn = warnOnce(oldUsage, newUsage);
    return function (this: any) {
      warn();
      return fn.apply(this, arguments);
    } as unknown as typeof fn;
  },

  // 更改事件的名称。
  event: (emitter, oldName, newName) => {
    const warn = newName.startsWith('-') /* 内部事件。*/
      ? warnOnce(`${oldName} event`)
      : warnOnce(`${oldName} event`, `${newName} event`);
    return emitter.on(newName, function (this: NodeJS.EventEmitter, ...args) {
      if (this.listenerCount(oldName) !== 0) {
        warn();
        this.emit(oldName, ...args);
      }
    });
  },

  // 删除不替换的属性。
  removeProperty: (o, removedName, onlyForValues) => {
    // 如果该属性已被移除，请就此发出警告。
    const info = Object.getOwnPropertyDescriptor((o as any).__proto__, removedName) // ESRINT-DISABLE-LINE。
    if (!info) {
      deprecate.log(`Unable to remove property '${removedName}' from an object that lacks it.`);
      return o;
    }
    if (!info.get || !info.set) {
      deprecate.log(`Unable to remove property '${removedName}' from an object does not have a getter / setter`);
      return o;
    }

    // 将不推荐使用的属性包装在访问器中以发出警告。
    const warn = warnOnce(removedName);
    return Object.defineProperty(o, removedName, {
      configurable: true,
      get: () => {
        warn();
        return info.get!.call(o);
      },
      set: newVal => {
        if (!onlyForValues || onlyForValues.includes(newVal)) {
          warn();
        }
        return info.set!.call(o, newVal);
      }
    });
  },

  // 更改属性的名称。
  renameProperty: (o, oldName, newName) => {
    const warn = warnOnce(oldName, newName);

    // 如果新房子还没建好，
    // 注射它并警告它。
    if ((oldName in o) && !(newName in o)) {
      warn();
      o[newName] = (o as any)[oldName];
    }

    // 将不推荐使用的属性包装在访问器中以发出警告。
    // 并重定向到新属性
    return Object.defineProperty(o, oldName, {
      get: () => {
        warn();
        return o[newName];
      },
      set: value => {
        warn();
        o[newName] = value;
      }
    });
  }
};

export default deprecate;
