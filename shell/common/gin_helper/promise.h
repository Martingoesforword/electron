// 版权所有(C)2018 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_GIN_HELPER_PROMISE_H_
#define SHELL_COMMON_GIN_HELPER_PROMISE_H_

#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

#include "base/strings/string_piece.h"
#include "base/task/post_task.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "shell/common/gin_converters/std_converter.h"
#include "shell/common/gin_helper/locker.h"
#include "shell/common/gin_helper/microtasks_scope.h"

namespace gin_helper {

// V8：：Promise的包装纸。
// 
// 这是在模板之间共享代码的非模板基类。
// 实例。
// 
// 这是一个仅移动类型，传递给时应始终为`std：：move`d。
// 回调，应该在相同的创建线程上销毁它。
class PromiseBase {
 public:
  explicit PromiseBase(v8::Isolate* isolate);
  PromiseBase(v8::Isolate* isolate, v8::Local<v8::Promise::Resolver> handle);
  ~PromiseBase();

  // 支持搬家。
  PromiseBase(PromiseBase&&);
  PromiseBase& operator=(PromiseBase&&);

  // 用于拒绝带有错误消息的承诺的帮助器。
  // 
  // 注意：参数类型为PromiseBase&&，因此它可以采用。
  // 承诺&lt;T&gt;类型。
  static void RejectPromise(PromiseBase&& promise, base::StringPiece errmsg) {
    if (gin_helper::Locker::IsBrowserProcess() &&
        !content::BrowserThread::CurrentlyOn(content::BrowserThread::UI)) {
      base::PostTask(
          FROM_HERE, {content::BrowserThread::UI},
          base::BindOnce(
              // 请注意，此回调不能接受StringPiess，
              // 因为StringPiess仅在内部引用字符串，并且。
              // 将在经过临时字符串时爆炸。
              [](PromiseBase&& promise, std::string str) {
                promise.RejectWithErrorMessage(str);
              },
              std::move(promise), std::string(errmsg.data(), errmsg.size())));
    } else {
      promise.RejectWithErrorMessage(errmsg);
    }
  }

  v8::Maybe<bool> Reject();
  v8::Maybe<bool> Reject(v8::Local<v8::Value> except);
  v8::Maybe<bool> RejectWithErrorMessage(base::StringPiece message);

  v8::Local<v8::Context> GetContext() const;
  v8::Local<v8::Promise> GetHandle() const;

  v8::Isolate* isolate() const { return isolate_; }

 protected:
  v8::Local<v8::Promise::Resolver> GetInner() const;

 private:
  v8::Isolate* isolate_;
  v8::Global<v8::Context> context_;
  v8::Global<v8::Promise::Resolver> resolver_;

  DISALLOW_COPY_AND_ASSIGN(PromiseBase);
};

// 返回值的模板实现。
template <typename RT>
class Promise : public PromiseBase {
 public:
  using PromiseBase::PromiseBase;

  // 使用|Result|解析承诺的帮助器。
  static void ResolvePromise(Promise<RT> promise, RT result) {
    if (gin_helper::Locker::IsBrowserProcess() &&
        !content::BrowserThread::CurrentlyOn(content::BrowserThread::UI)) {
      base::PostTask(FROM_HERE, {content::BrowserThread::UI},
                     base::BindOnce([](Promise<RT> promise,
                                       RT result) { promise.Resolve(result); },
                                    std::move(promise), std::move(result)));
    } else {
      promise.Resolve(result);
    }
  }

  // 返回已解析的承诺。
  static v8::Local<v8::Promise> ResolvedPromise(v8::Isolate* isolate,
                                                RT result) {
    Promise<RT> resolved(isolate);
    resolved.Resolve(result);
    return resolved.GetHandle();
  }

  // 承诺解决是一项微任务。
  // 我们使用MicrotasksRunner来触发挂起的微任务的运行。
  v8::Maybe<bool> Resolve(const RT& value) {
    gin_helper::Locker locker(isolate());
    v8::HandleScope handle_scope(isolate());
    gin_helper::MicrotasksScope microtasks_scope(isolate());
    v8::Context::Scope context_scope(GetContext());

    return GetInner()->Resolve(GetContext(),
                               gin::ConvertToV8(isolate(), value));
  }

  template <typename... ResolveType>
  v8::MaybeLocal<v8::Promise> Then(
      base::OnceCallback<void(ResolveType...)> cb) {
    static_assert(sizeof...(ResolveType) <= 1,
                  "A promise's 'Then' callback should only receive at most one "
                  "parameter");
    static_assert(
        std::is_same<RT, std::tuple_element_t<0, std::tuple<ResolveType...>>>(),
        "A promises's 'Then' callback must handle the same type as the "
        "promises resolve type");
    gin_helper::Locker locker(isolate());
    v8::HandleScope handle_scope(isolate());
    v8::Context::Scope context_scope(GetContext());

    v8::Local<v8::Value> value = gin::ConvertToV8(isolate(), std::move(cb));
    v8::Local<v8::Function> handler = value.As<v8::Function>();

    return GetHandle()->Then(GetContext(), handler);
  }
};

// 不返回任何内容的模板实现。
template <>
class Promise<void> : public PromiseBase {
 public:
  using PromiseBase::PromiseBase;

  // 解决空头承诺的帮助者。
  static void ResolvePromise(Promise<void> promise);

  // 返回已解析的承诺。
  static v8::Local<v8::Promise> ResolvedPromise(v8::Isolate* isolate);

  v8::Maybe<bool> Resolve();
};

}  // 命名空间gin_helper。

namespace gin {

template <typename T>
struct Converter<gin_helper::Promise<T>> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const gin_helper::Promise<T>& val) {
    return val.GetHandle();
  }
  // TODO(MarshallOfSound)：实现FromV8以允许承诺链接。
  // 在故乡。
  // 静态布尔来自V8(V8：：Isolate*Isolate，
  // V8：：local&lt;v8：：value&gt;val，
  // 承诺*出局)；
};

}  // 命名空间杜松子酒。

#endif  // Shell_COMMON_GIN_HELPER_PROMANCE_H_
