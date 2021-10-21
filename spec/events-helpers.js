/* **@file概观一组帮助函数，可以更轻松地*以异步/等待的方式处理事件。*/

/* **@param{！EventTarget}target*@param{string}eventName*@return{！Promise&lt;！Event&gt;}。*/
const waitForEvent = (target, eventName) => {
  return new Promise(resolve => {
    target.addEventListener(eventName, resolve, { once: true });
  });
};

/* **@param{！EventEmitter}发射器*@param{String}eventName*@return{！Promise&lt;！array&gt;}，第一项为Event。*/
const emittedOnce = (emitter, eventName) => {
  return emittedNTimes(emitter, eventName, 1).then(([result]) => result);
};

const emittedNTimes = (emitter, eventName, times) => {
  const events = [];
  return new Promise(resolve => {
    const handler = (...args) => {
      events.push(args);
      if (events.length === times) {
        emitter.removeListener(eventName, handler);
        resolve(events);
      }
    };
    emitter.on(eventName, handler);
  });
};

exports.emittedOnce = emittedOnce;
exports.emittedNTimes = emittedNTimes;
exports.waitForEvent = waitForEvent;
