// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/javascript_environment.h"

#include <memory>
#include <string>
#include <unordered_set>
#include <utility>

#include "base/allocator/partition_allocator/partition_alloc.h"
#include "base/command_line.h"
#include "base/task/current_thread.h"
#include "base/task/thread_pool/initialization_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/trace_event.h"
#include "content/public/common/content_switches.h"
#include "gin/array_buffer.h"
#include "gin/v8_initializer.h"
#include "shell/browser/microtasks_runner.h"
#include "shell/common/gin_helper/cleaned_up_at_exit.h"
#include "shell/common/node_includes.h"

namespace {
v8::Isolate* g_isolate;
}

namespace gin {

class ConvertableToTraceFormatWrapper final
    : public base::trace_event::ConvertableToTraceFormat {
 public:
  explicit ConvertableToTraceFormatWrapper(
      std::unique_ptr<v8::ConvertableToTraceFormat> inner)
      : inner_(std::move(inner)) {}
  ~ConvertableToTraceFormatWrapper() override = default;
  void AppendAsTraceFormat(std::string* out) const final {
    inner_->AppendAsTraceFormat(out);
  }

 private:
  std::unique_ptr<v8::ConvertableToTraceFormat> inner_;

  DISALLOW_COPY_AND_ASSIGN(ConvertableToTraceFormatWrapper);
};

}  // 命名空间杜松子酒。

// 允许std：：Unique_PTR&lt;V8：：ConvertableToTraceFormat&gt;为有效的。
// 跟踪宏的初始化值。
template <>
struct base::trace_event::TraceValue::Helper<
    std::unique_ptr<v8::ConvertableToTraceFormat>> {
  static constexpr unsigned char kType = TRACE_VALUE_TYPE_CONVERTABLE;
  static inline void SetValue(
      TraceValue* v,
      std::unique_ptr<v8::ConvertableToTraceFormat> value) {
    // 注意：|as_Convertable|是拥有的指针，所以在这里使用new。
    // 是可以接受的。
    v->as_convertable =
        new gin::ConvertableToTraceFormatWrapper(std::move(value));
  }
};

namespace electron {

class ArrayBufferAllocator : public v8::ArrayBuffer::Allocator {
 public:
  enum InitializationPolicy { kZeroInitialize, kDontInitialize };

  ArrayBufferAllocator() {
    // 裁判。
    // Https://source.chromium.org/chromium/chromium/src/+/master:third_party/blink/renderer/platform/wtf/allocator/partitions.cc；l=94；drc=062c315a858a87f834e16a144c2c8e9591af2beb。
    allocator_->init({base::PartitionOptions::AlignedAlloc::kDisallowed,
                      base::PartitionOptions::ThreadCache::kDisabled,
                      base::PartitionOptions::Quarantine::kAllowed,
                      base::PartitionOptions::Cookie::kAllowed,
                      base::PartitionOptions::BackupRefPtr::kDisabled,
                      base::PartitionOptions::UseConfigurablePool::kNo});
  }

  // ALLOCATE()方法返回NULL，向V8发出分配失败的信号，
  // 应该通过抛出RangeError、Per。
  // Http://www.ecma-international.org/ecma-262/6.0/#sec-createbytedatablock.。
  void* Allocate(size_t size) override {
    void* result = AllocateMemoryOrNull(size, kZeroInitialize);
    return result;
  }

  void* AllocateUninitialized(size_t size) override {
    void* result = AllocateMemoryOrNull(size, kDontInitialize);
    return result;
  }

  void Free(void* data, size_t size) override {
    allocator_->root()->Free(data);
  }

 private:
  static void* AllocateMemoryOrNull(size_t size, InitializationPolicy policy) {
    return AllocateMemoryWithFlags(size, policy,
                                   base::PartitionAllocReturnNull);
  }

  static void* AllocateMemoryWithFlags(size_t size,
                                       InitializationPolicy policy,
                                       int flags) {
    // 数组缓冲区内容有时预期为16字节对齐。
    // 以获得SSE的最佳优化，特别是在音频情况下。
    // 和视频缓冲器。因此，将给定大小与16字节边界对齐。
    // 从技术上讲，16字节对齐大小并不意味着16字节对齐。
    // 地址，但此启发式方法适用于。
    // Partitionalloc(Partitionalloc目前不支持更好的方式)。
    if (base::kAlignment <
        16) {  // Base：：kAlign是编译时常量。
      size_t aligned_size = base::bits::AlignUp(size, 16);
      if (size == 0) {
        aligned_size = 16;
      }
      if (aligned_size >= size) {  // 仅当没有溢出时。
        size = aligned_size;
      }
    }

    if (policy == kZeroInitialize) {
      flags |= base::PartitionAllocZeroFill;
    }
    void* data = allocator_->root()->AllocFlags(flags, size, "Electron");
    if (base::kAlignment < 16) {
      char* ptr = reinterpret_cast<char*>(data);
      DCHECK_EQ(base::bits::AlignUp(ptr, 16), ptr)
          << "Pointer " << ptr << " not 16B aligned for size " << size;
    }
    return data;
  }

  static base::NoDestructor<base::PartitionAllocator> allocator_;
};

base::NoDestructor<base::PartitionAllocator> ArrayBufferAllocator::allocator_{};

JavascriptEnvironment::JavascriptEnvironment(uv_loop_t* event_loop)
    : isolate_(Initialize(event_loop)),
      isolate_holder_(base::ThreadTaskRunnerHandle::Get(),
                      gin::IsolateHolder::kSingleThread,
                      gin::IsolateHolder::kAllowAtomicsWait,
                      gin::IsolateHolder::IsolateType::kUtility,
                      gin::IsolateHolder::IsolateCreationMode::kNormal,
                      isolate_),
      locker_(isolate_) {
  isolate_->Enter();
  v8::HandleScope scope(isolate_);
  auto context = node::NewContext(isolate_);
  context_ = v8::Global<v8::Context>(isolate_, context);
  context->Enter();
}

JavascriptEnvironment::~JavascriptEnvironment() {
  DCHECK_NE(platform_, nullptr);
  platform_->DrainTasks(isolate_);

  {
    v8::Locker locker(isolate_);
    v8::HandleScope scope(isolate_);
    context_.Get(isolate_)->Exit();
  }
  isolate_->Exit();
  g_isolate = nullptr;

  platform_->UnregisterIsolate(isolate_);
}

class EnabledStateObserverImpl final
    : public base::trace_event::TraceLog::EnabledStateObserver {
 public:
  EnabledStateObserverImpl() {
    base::trace_event::TraceLog::GetInstance()->AddEnabledStateObserver(this);
  }

  ~EnabledStateObserverImpl() override {
    base::trace_event::TraceLog::GetInstance()->RemoveEnabledStateObserver(
        this);
  }

  void OnTraceLogEnabled() final {
    base::AutoLock lock(mutex_);
    for (auto* o : observers_) {
      o->OnTraceEnabled();
    }
  }

  void OnTraceLogDisabled() final {
    base::AutoLock lock(mutex_);
    for (auto* o : observers_) {
      o->OnTraceDisabled();
    }
  }

  void AddObserver(v8::TracingController::TraceStateObserver* observer) {
    {
      base::AutoLock lock(mutex_);
      DCHECK(!observers_.count(observer));
      observers_.insert(observer);
    }

    // 如果录制已在进行中，则解雇观察者。
    if (base::trace_event::TraceLog::GetInstance()->IsEnabled())
      observer->OnTraceEnabled();
  }

  void RemoveObserver(v8::TracingController::TraceStateObserver* observer) {
    base::AutoLock lock(mutex_);
    DCHECK_EQ(observers_.count(observer), 1lu);
    observers_.erase(observer);
  }

 private:
  base::Lock mutex_;
  std::unordered_set<v8::TracingController::TraceStateObserver*> observers_;

  DISALLOW_COPY_AND_ASSIGN(EnabledStateObserverImpl);
};

base::LazyInstance<EnabledStateObserverImpl>::Leaky g_trace_state_dispatcher =
    LAZY_INSTANCE_INITIALIZER;

class TracingControllerImpl : public node::tracing::TracingController {
 public:
  TracingControllerImpl() = default;
  ~TracingControllerImpl() override = default;

  // TracingController实现。
  const uint8_t* GetCategoryGroupEnabled(const char* name) override {
    return TRACE_EVENT_API_GET_CATEGORY_GROUP_ENABLED(name);
  }
  uint64_t AddTraceEvent(
      char phase,
      const uint8_t* category_enabled_flag,
      const char* name,
      const char* scope,
      uint64_t id,
      uint64_t bind_id,
      int32_t num_args,
      const char** arg_names,
      const uint8_t* arg_types,
      const uint64_t* arg_values,
      std::unique_ptr<v8::ConvertableToTraceFormat>* arg_convertables,
      unsigned int flags) override {
    base::trace_event::TraceArguments args(
        num_args, arg_names, arg_types,
        reinterpret_cast<const unsigned long long*>(  // NOLINT(运行时/INT)。
            arg_values),
        arg_convertables);
    DCHECK_LE(num_args, 2);
    base::trace_event::TraceEventHandle handle =
        TRACE_EVENT_API_ADD_TRACE_EVENT_WITH_BIND_ID(
            phase, category_enabled_flag, name, scope, id, bind_id, &args,
            flags);
    uint64_t result;
    memcpy(&result, &handle, sizeof(result));
    return result;
  }
  uint64_t AddTraceEventWithTimestamp(
      char phase,
      const uint8_t* category_enabled_flag,
      const char* name,
      const char* scope,
      uint64_t id,
      uint64_t bind_id,
      int32_t num_args,
      const char** arg_names,
      const uint8_t* arg_types,
      const uint64_t* arg_values,
      std::unique_ptr<v8::ConvertableToTraceFormat>* arg_convertables,
      unsigned int flags,
      int64_t timestampMicroseconds) override {
    base::trace_event::TraceArguments args(
        num_args, arg_names, arg_types,
        reinterpret_cast<const unsigned long long*>(  // NOLINT(运行时/INT)。
            arg_values),
        arg_convertables);
    DCHECK_LE(num_args, 2);
    base::TimeTicks timestamp =
        base::TimeTicks() +
        base::TimeDelta::FromMicroseconds(timestampMicroseconds);
    base::trace_event::TraceEventHandle handle =
        TRACE_EVENT_API_ADD_TRACE_EVENT_WITH_THREAD_ID_AND_TIMESTAMP(
            phase, category_enabled_flag, name, scope, id, bind_id,
            TRACE_EVENT_API_CURRENT_THREAD_ID, timestamp, &args, flags);
    uint64_t result;
    memcpy(&result, &handle, sizeof(result));
    return result;
  }
  void UpdateTraceEventDuration(const uint8_t* category_enabled_flag,
                                const char* name,
                                uint64_t handle) override {
    base::trace_event::TraceEventHandle traceEventHandle;
    memcpy(&traceEventHandle, &handle, sizeof(handle));
    TRACE_EVENT_API_UPDATE_TRACE_EVENT_DURATION(category_enabled_flag, name,
                                                traceEventHandle);
  }
  void AddTraceStateObserver(TraceStateObserver* observer) override {
    g_trace_state_dispatcher.Get().AddObserver(observer);
  }
  void RemoveTraceStateObserver(TraceStateObserver* observer) override {
    g_trace_state_dispatcher.Get().RemoveObserver(observer);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TracingControllerImpl);
};

v8::Isolate* JavascriptEnvironment::Initialize(uv_loop_t* event_loop) {
  auto* cmd = base::CommandLine::ForCurrentProcess();

  // --JS-FLAGS。
  std::string js_flags = cmd->GetSwitchValueASCII(switches::kJavaScriptFlags);
  if (!js_flags.empty())
    v8::V8::SetFlagsFromString(js_flags.c_str(), js_flags.size());

  // GIN的V8平台依赖于Chromium的任务调度，而Chromium没有。
  // 在这一点上已经开始了，所以我们必须依赖Node的V8Platform。
  auto* tracing_agent = node::CreateAgent();
  auto* tracing_controller = new TracingControllerImpl();
  node::tracing::TraceEventHelper::SetAgent(tracing_agent);
  platform_ = node::CreatePlatform(
      base::RecommendedMaxNumberOfThreadsInThreadGroup(3, 8, 0.1, 0),
      tracing_controller, gin::V8Platform::PageAllocator());

  v8::V8::InitializePlatform(platform_);
  gin::IsolateHolder::Initialize(
      gin::IsolateHolder::kNonStrictMode, new ArrayBufferAllocator(),
      nullptr /* 外部参考表格。*/, false /* 创建_v8_平台。*/);

  v8::Isolate* isolate = v8::Isolate::Allocate();
  platform_->RegisterIsolate(isolate, event_loop);
  g_isolate = isolate;

  return isolate;
}

// 静电。
v8::Isolate* JavascriptEnvironment::GetIsolate() {
  CHECK(g_isolate);
  return g_isolate;
}

void JavascriptEnvironment::OnMessageLoopCreated() {
  DCHECK(!microtasks_runner_);
  microtasks_runner_ = std::make_unique<MicrotasksRunner>(isolate());
  base::CurrentThread::Get()->AddTaskObserver(microtasks_runner_.get());
}

void JavascriptEnvironment::OnMessageLoopDestroying() {
  DCHECK(microtasks_runner_);
  {
    v8::Locker locker(isolate_);
    v8::HandleScope scope(isolate_);
    gin_helper::CleanedUpAtExit::DoCleanup();
  }
  base::CurrentThread::Get()->RemoveTaskObserver(microtasks_runner_.get());
}

NodeEnvironment::NodeEnvironment(node::Environment* env) : env_(env) {}

NodeEnvironment::~NodeEnvironment() {
  auto* isolate_data = env_->isolate_data();
  node::FreeEnvironment(env_);
  node::FreeIsolateData(isolate_data);
}

}  // 命名空间电子
