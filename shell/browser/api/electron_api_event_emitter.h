// 版权所有(C)2019 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_API_ELECTRON_API_EVENT_EMITTER_H_
#define SHELL_BROWSER_API_ELECTRON_API_EVENT_EMITTER_H_

namespace v8 {
template <typename T>
class Local;
class Object;
class Isolate;
}  // 命名空间V8。

namespace electron {

v8::Local<v8::Object> GetEventEmitterPrototype(v8::Isolate* isolate);

}  // 命名空间电子。

#endif  // SHELL_BROWSER_API_ELECTRON_API_EVENT_EMITTER_H_
