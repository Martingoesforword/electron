// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_NODE_UTIL_H_
#define SHELL_COMMON_NODE_UTIL_H_

#include <vector>

#include "v8/include/v8.h"

namespace node {
class Environment;
}

namespace electron {

namespace util {

// 运行一个将JS源代码捆绑在二进制文件中的脚本，就好像它被包装了一样。
// 在使用空接收器和C++中指定的参数调用的函数中。
// 如果遇到异常，则返回值为空。
// 使用此方法运行的JS代码可以假定其顶层。
// 声明不会影响全局范围。
v8::MaybeLocal<v8::Value> CompileAndCall(
    v8::Local<v8::Context> context,
    const char* id,
    std::vector<v8::Local<v8::String>>* parameters,
    std::vector<v8::Local<v8::Value>>* arguments,
    node::Environment* optional_env);

}  // 命名空间实用程序。

}  // 命名空间电子。

#endif  // Shell_COMMON_NODE_UTIL_H_
