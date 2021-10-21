// 版权所有2013年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在LICENSE.Cr文件中找到。

#ifndef SHELL_COMMON_GIN_HELPER_WRAPPABLE_BASE_H_
#define SHELL_COMMON_GIN_HELPER_WRAPPABLE_BASE_H_

#include "v8/include/v8.h"

namespace gin {
class Arguments;
}

namespace gin_helper {

// Wrappable是具有相应V8包装器的C++对象的基类。
// 物体。若要将Wrappable对象保留在堆栈上，请使用gin：：Handle。
// 
// 用途：
// //my_class.h。
// 类MyClass：可包装的&lt;MyClass&gt;{。
// 公众：
// ..。
// }；
// 
// 子类通常还应该具有私有构造函数，并公开。
// 返回gin：：Handle的静态创建函数。迫使创作者通过。
// 此静态创建函数将强制客户端实际创建。
// 对象的包装。如果客户端无法为可包装的。
// 对象，则该对象将泄漏，因为我们使用。
// 包装器作为删除包装对象的信号。
class WrappableBase {
 public:
  WrappableBase();
  WrappableBase(const WrappableBase&) = delete;
  WrappableBase& operator=(const WrappableBase&) = delete;
  virtual ~WrappableBase();

  // 检索与该对象对应的V8包装器对象。
  v8::Local<v8::Object> GetWrapper() const;
  v8::MaybeLocal<v8::Object> GetWrapper(v8::Isolate* isolate) const;

  // 返回在其中创建此对象的隔离。
  v8::Isolate* isolate() const { return isolate_; }

 protected:
  // 在JavaScript中调用“_init”方法之后调用。
  virtual void AfterInit(v8::Isolate* isolate) {}

  // 将C++类绑定到JS包装器。
  // 此方法只能由使用构造函数的类调用。
  virtual void InitWith(v8::Isolate* isolate, v8::Local<v8::Object> wrapper);

  // 用参数初始化的帮助器。
  void InitWithArgs(gin::Arguments* args);

  v8::Global<v8::Object> wrapper_;  // 瘦弱。

 private:
  static void FirstWeakCallback(
      const v8::WeakCallbackInfo<WrappableBase>& data);
  static void SecondWeakCallback(
      const v8::WeakCallbackInfo<WrappableBase>& data);

  v8::Isolate* isolate_ = nullptr;
};

}  // 命名空间gin_helper。

#endif  // SHELL_COMMON_GIN_HELPER_WRAPPABLE_BASE_H_
