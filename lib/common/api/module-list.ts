// 常用模块，请按字母顺序排序。
export const commonModuleList: ElectronInternal.ModuleEntry[] = [
  { name: 'clipboard', loader: () => require('./clipboard') },
  { name: 'nativeImage', loader: () => require('./native-image') },
  { name: 'shell', loader: () => require('./shell') },
  // 内部模块是不可见的，除非你知道它们的名字。
  { name: 'deprecate', loader: () => require('./deprecate'), private: true }
];
