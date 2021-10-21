// 将原始全局变量捕获到作用域中，以确保用户区域修改。
// 不是撞击电子。请注意，用户正在执行以下操作：
// 
// Global.Promise.Resolve=myFn。
// 
// 也会使这个捕获的基因发生变异，这是可以的。

export const Promise = global.Promise;
