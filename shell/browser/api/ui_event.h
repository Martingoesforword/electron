// 版权所有(C)2020 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_API_UI_EVENT_H_
#define SHELL_BROWSER_API_UI_EVENT_H_

namespace v8 {
class Object;
template <typename T>
class Local;
}  // 命名空间V8。

namespace electron {
namespace api {

v8::Local<v8::Object> CreateEventFromFlags(int flags);

}  // 命名空间API。
}  // 命名空间电子。

#endif  // Shell_Browser_API_UI_Event_H_
