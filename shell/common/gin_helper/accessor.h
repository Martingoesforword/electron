// 版权所有(C)2021年Samuel Maddock&lt;Sam@samuelmaddock.com&gt;。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_GIN_HELPER_ACCESSOR_H_
#define SHELL_COMMON_GIN_HELPER_ACCESSOR_H_

namespace gin_helper {

// 中用作访问器的泛型值的包装。
// GIN_HELPER：：DICTIONARY。
template <typename T>
struct AccessorValue {
  T Value;
};
template <typename T>
struct AccessorValue<const T&> {
  T Value;
};
template <typename T>
struct AccessorValue<const T*> {
  T* Value;
};

}  // 命名空间gin_helper。

#endif  // SHELL_COMMON_GIN_HELPER_ACCELER_H_
