// 版权所有(C)2014 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include <set>
#include <string>
#include <utility>

#include "base/files/file_util.h"
#include "base/task/thread_pool.h"
#include "base/threading/thread_restrictions.h"
#include "base/trace_event/trace_config.h"
#include "content/public/browser/tracing_controller.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/node_includes.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

using content::TracingController;

namespace gin {

template <>
struct Converter<base::trace_event::TraceConfig> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::trace_event::TraceConfig* out) {
    // (Alexeykuzmin)：组合了“recastyFilter”和“traceOptions”
    // 必须首先选中，因为没有任何字段。
    // 在`MEMORY_DUMP_CONFIG`中，下面的判据是必填的。
    // 我们不能检查配置格式。
    gin_helper::Dictionary options;
    if (ConvertFromV8(isolate, val, &options)) {
      std::string category_filter, trace_options;
      if (options.Get("categoryFilter", &category_filter) &&
          options.Get("traceOptions", &trace_options)) {
        *out = base::trace_event::TraceConfig(category_filter, trace_options);
        return true;
      }
    }

    base::DictionaryValue memory_dump_config;
    if (ConvertFromV8(isolate, val, &memory_dump_config)) {
      *out = base::trace_event::TraceConfig(memory_dump_config);
      return true;
    }

    return false;
  }
};

}  // 命名空间杜松子酒。

namespace {

using CompletionCallback = base::OnceCallback<void(const base::FilePath&)>;

absl::optional<base::FilePath> CreateTemporaryFileOnIO() {
  base::FilePath temp_file_path;
  if (!base::CreateTemporaryFile(&temp_file_path))
    return absl::nullopt;
  return absl::make_optional(std::move(temp_file_path));
}

void StopTracing(gin_helper::Promise<base::FilePath> promise,
                 absl::optional<base::FilePath> file_path) {
  auto resolve_or_reject = base::BindOnce(
      [](gin_helper::Promise<base::FilePath> promise,
         const base::FilePath& path, absl::optional<std::string> error) {
        if (error) {
          promise.RejectWithErrorMessage(error.value());
        } else {
          promise.Resolve(path);
        }
      },
      std::move(promise), *file_path);
  if (file_path) {
    auto split_callback = base::SplitOnceCallback(std::move(resolve_or_reject));
    auto endpoint = TracingController::CreateFileEndpoint(
        *file_path,
        base::BindOnce(std::move(split_callback.first), absl::nullopt));
    if (!TracingController::GetInstance()->StopTracing(endpoint)) {
      std::move(split_callback.second)
          .Run(absl::make_optional(
              "Failed to stop tracing (was a trace in progress?)"));
    }
  } else {
    std::move(resolve_or_reject)
        .Run(absl::make_optional(
            "Failed to create temporary file for trace data"));
  }
}

v8::Local<v8::Promise> StopRecording(gin_helper::Arguments* args) {
  gin_helper::Promise<base::FilePath> promise(args->isolate());
  v8::Local<v8::Promise> handle = promise.GetHandle();

  base::FilePath path;
  if (args->GetNext(&path) && !path.empty()) {
    StopTracing(std::move(promise), absl::make_optional(path));
  } else {
    // 使用临时文件。
    base::ThreadPool::PostTaskAndReplyWithResult(
        FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
        base::BindOnce(CreateTemporaryFileOnIO),
        base::BindOnce(StopTracing, std::move(promise)));
  }

  return handle;
}

v8::Local<v8::Promise> GetCategories(v8::Isolate* isolate) {
  gin_helper::Promise<const std::set<std::string>&> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  // 注意：此方法总是成功的。
  TracingController::GetInstance()->GetCategories(base::BindOnce(
      gin_helper::Promise<const std::set<std::string>&>::ResolvePromise,
      std::move(promise)));

  return handle;
}

v8::Local<v8::Promise> StartTracing(
    v8::Isolate* isolate,
    const base::trace_event::TraceConfig& trace_config) {
  gin_helper::Promise<void> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  if (!TracingController::GetInstance()->StartTracing(
          trace_config,
          base::BindOnce(gin_helper::Promise<void>::ResolvePromise,
                         std::move(promise)))) {
    // 如果StartTracing返回False，这意味着它没有调用其回调。
    // 退回已解决的承诺并放弃先前的承诺(它。
    // 将std：：Move()d放入StartTracing回调，并已被删除。
    // 这一点)。
    return gin_helper::Promise<void>::ResolvedPromise(isolate);
  }
  return handle;
}

void OnTraceBufferUsageAvailable(
    gin_helper::Promise<gin_helper::Dictionary> promise,
    float percent_full,
    size_t approximate_count) {
  gin_helper::Dictionary dict = gin::Dictionary::CreateEmpty(promise.isolate());
  dict.Set("percentage", percent_full);
  dict.Set("value", approximate_count);

  promise.Resolve(dict);
}

v8::Local<v8::Promise> GetTraceBufferUsage(v8::Isolate* isolate) {
  gin_helper::Promise<gin_helper::Dictionary> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  // 注意：此方法总是成功的。
  TracingController::GetInstance()->GetTraceBufferUsage(
      base::BindOnce(&OnTraceBufferUsageAvailable, std::move(promise)));
  return handle;
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  gin_helper::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("getCategories", &GetCategories);
  dict.SetMethod("startRecording", &StartTracing);
  dict.SetMethod("stopRecording", &StopRecording);
  dict.SetMethod("getTraceBufferUsage", &GetTraceBufferUsage);
}

}  // 命名空间

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_content_tracing, Initialize)
