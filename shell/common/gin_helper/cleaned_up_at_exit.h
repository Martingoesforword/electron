// 版权所有(C)2020 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_GIN_HELPER_CLEANED_UP_AT_EXIT_H_
#define SHELL_COMMON_GIN_HELPER_CLEANED_UP_AT_EXIT_H_

namespace gin_helper {

// 此类型的对象将在处置V8之前立即销毁。
// 隔离。这应该仅用于GIN：：Wrappable对象，其生存期。
// 由V8以其他方式管理。
// 
// 注意：这只是因为具有SetWeak的V8：：Global对象。
// 终结回调没有在调用它们的终结回调。
// 隔离拆卸。
class CleanedUpAtExit {
 public:
  CleanedUpAtExit();
  virtual ~CleanedUpAtExit();

  static void DoCleanup();
};

}  // 命名空间gin_helper。

#endif  // Shell_COMMON_GIN_HELPER_CLEARED_UP_AT_EXIT_H_
