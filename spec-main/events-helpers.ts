/* **@file概观一组帮助函数，可以更轻松地*以异步/等待的方式处理事件。*/

/* **@param{！EventTarget}target*@param{string}eventName*@return{！Promise&lt;！Event&gt;}。*/
export const waitForEvent = (target: EventTarget, eventName: string) => {
  return new Promise(resolve => {
    target.addEventListener(eventName, resolve, { once: true });
  });
};

/* **@param{！EventEmitter}发射器*@param{String}eventName*@return{！Promise&lt;！array&gt;}，第一项为Event。*/
export const emittedOnce = (emitter: NodeJS.EventEmitter, eventName: string, trigger?: () => void) => {
  return emittedNTimes(emitter, eventName, 1, trigger).then(([result]) => result);
};

export const emittedNTimes = async (emitter: NodeJS.EventEmitter, eventName: string, times: number, trigger?: () => void) => {
  const events: any[][] = [];
  const p = new Promise<any[][]>(resolve => {
    const handler = (...args: any[]) => {
      events.push(args);
      if (events.length === times) {
        emitter.removeListener(eventName, handler);
        resolve(events);
      }
    };
    emitter.on(eventName, handler);
  });
  if (trigger) {
    await Promise.resolve(trigger());
  }
  return p;
};

export const emittedUntil = async (emitter: NodeJS.EventEmitter, eventName: string, untilFn: Function) => {
  const p = new Promise<any[]>(resolve => {
    const handler = (...args: any[]) => {
      if (untilFn(...args)) {
        emitter.removeListener(eventName, handler);
        resolve(args);
      }
    };
    emitter.on(eventName, handler);
  });
  return p;
};
