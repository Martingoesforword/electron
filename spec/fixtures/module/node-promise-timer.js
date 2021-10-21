const waitMs = (msec) => new Promise((resolve) => setTimeout(resolve, msec));

const intervalMsec = 100;
const numIterations = 2;
let curIteration = 0;
let promise;

for (let i = 0; i < numIterations; i++) {
  promise = (promise || waitMs(intervalMsec)).then(() => {
    ++curIteration;
    return waitMs(intervalMsec);
  });
}

// Https://github.com/electron/electron/issues/21515是关于电子的。
// 承诺还没完成就退出了。此测试设置挂起的退出代码。
// 失败，然后只有当所有的承诺都完成时，才会将它重置为成功。
process.exitCode = 1;
promise.then(() => {
  if (curIteration === numIterations) {
    process.exitCode = 0;
  }
});
