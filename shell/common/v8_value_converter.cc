// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/common/v8_value_converter.h"

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/logging.h"
#include "base/values.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_bindings.h"
#include "shell/common/node_includes.h"

namespace electron {

namespace {

const int kMaxRecursionDepth = 100;

}  // 命名空间。

// 调用FromV8Value的状态。
class V8ValueConverter::FromV8ValueState {
 public:
  // Level作用域，更新某些FromV8ValueState的当前深度。
  class Level {
   public:
    explicit Level(FromV8ValueState* state) : state_(state) {
      state_->max_recursion_depth_--;
    }
    ~Level() { state_->max_recursion_depth_++; }

   private:
    FromV8ValueState* state_;
  };

  FromV8ValueState() : max_recursion_depth_(kMaxRecursionDepth) {}

  // 如果|HANDLE|不在|UNIQUE_MAP_|中，则将其添加到|UNIQUE_MAP_|并。
  // 返回true。
  // 
  // 否则，什么都不做并返回false。这里的“A是独一无二的”意味着没有。
  // 地图中的另一个句柄B指向与A相同的对象。请注意，A可以。
  // 保持唯一，即使已经存在具有相同标识的另一个句柄。
  // 映射中的散列(键)，因为两个对象可以具有相同的散列。
  bool AddToUniquenessCheck(v8::Local<v8::Object> handle) {
    int hash;
    auto iter = GetIteratorInMap(handle, &hash);
    if (iter != unique_map_.end())
      return false;

    unique_map_.insert(std::make_pair(hash, handle));
    return true;
  }

  bool RemoveFromUniquenessCheck(v8::Local<v8::Object> handle) {
    int unused_hash;
    auto iter = GetIteratorInMap(handle, &unused_hash);
    if (iter == unique_map_.end())
      return false;
    unique_map_.erase(iter);
    return true;
  }

  bool HasReachedMaxRecursionDepth() { return max_recursion_depth_ < 0; }

 private:
  using HashToHandleMap = std::multimap<int, v8::Local<v8::Object>>;
  using Iterator = HashToHandleMap::const_iterator;

  Iterator GetIteratorInMap(v8::Local<v8::Object> handle, int* hash) {
    *hash = handle->GetIdentityHash();
    // 我们只对具有相同标识的对象使用==WITH句柄进行比较。
    // 哈希。不同的散列显然意味着不同的对象，但有两个对象。
    // 几千人中可能有相同的身份散列。
    std::pair<Iterator, Iterator> range = unique_map_.equal_range(*hash);
    for (auto it = range.first; it != range.second; ++it) {
      // 操作符==for句柄实际上比较了底层对象。
      if (it->second == handle)
        return it;
    }
    // 找不到。
    return unique_map_.end();
  }

  HashToHandleMap unique_map_;

  int max_recursion_depth_;
};

// 一个类，用于确保正在转换的对象/数组。
// 此V8ValueConverterImpl没有周期。
// 
// 循环示例：var v={}；v={key：v}；
// 不是循环的例子：var v={}；a=[v，v]；或w={a：v，b：v}；
class V8ValueConverter::ScopedUniquenessGuard {
 public:
  ScopedUniquenessGuard(V8ValueConverter::FromV8ValueState* state,
                        v8::Local<v8::Object> value)
      : state_(state),
        value_(value),
        is_valid_(state_->AddToUniquenessCheck(value_)) {}
  ~ScopedUniquenessGuard() {
    if (is_valid_) {
      bool removed = state_->RemoveFromUniquenessCheck(value_);
      DCHECK(removed);
    }
  }

  bool is_valid() const { return is_valid_; }

 private:
  typedef std::multimap<int, v8::Local<v8::Object>> HashToHandleMap;
  V8ValueConverter::FromV8ValueState* state_;
  v8::Local<v8::Object> value_;
  bool is_valid_;

  DISALLOW_COPY_AND_ASSIGN(ScopedUniquenessGuard);
};

V8ValueConverter::V8ValueConverter() = default;

void V8ValueConverter::SetRegExpAllowed(bool val) {
  reg_exp_allowed_ = val;
}

void V8ValueConverter::SetFunctionAllowed(bool val) {
  function_allowed_ = val;
}

void V8ValueConverter::SetStripNullFromObjects(bool val) {
  strip_null_from_objects_ = val;
}

v8::Local<v8::Value> V8ValueConverter::ToV8Value(
    const base::Value* value,
    v8::Local<v8::Context> context) const {
  v8::Context::Scope context_scope(context);
  v8::EscapableHandleScope handle_scope(context->GetIsolate());
  return handle_scope.Escape(ToV8ValueImpl(context->GetIsolate(), value));
}

std::unique_ptr<base::Value> V8ValueConverter::FromV8Value(
    v8::Local<v8::Value> val,
    v8::Local<v8::Context> context) const {
  v8::Context::Scope context_scope(context);
  v8::HandleScope handle_scope(context->GetIsolate());
  FromV8ValueState state;
  return FromV8ValueImpl(&state, val, context->GetIsolate());
}

v8::Local<v8::Value> V8ValueConverter::ToV8ValueImpl(
    v8::Isolate* isolate,
    const base::Value* value) const {
  switch (value->type()) {
    case base::Value::Type::NONE:
      return v8::Null(isolate);

    case base::Value::Type::BOOLEAN: {
      bool val = value->GetBool();
      return v8::Boolean::New(isolate, val);
    }

    case base::Value::Type::INTEGER: {
      int val = value->GetInt();
      return v8::Integer::New(isolate, val);
    }

    case base::Value::Type::DOUBLE: {
      double val = value->GetDouble();
      return v8::Number::New(isolate, val);
    }

    case base::Value::Type::STRING: {
      std::string val = value->GetString();
      return v8::String::NewFromUtf8(isolate, val.c_str(),
                                     v8::NewStringType::kNormal, val.length())
          .ToLocalChecked();
    }

    case base::Value::Type::LIST:
      return ToV8Array(isolate, static_cast<const base::ListValue*>(value));

    case base::Value::Type::DICTIONARY:
      return ToV8Object(isolate,
                        static_cast<const base::DictionaryValue*>(value));

    case base::Value::Type::BINARY:
      return ToArrayBuffer(isolate, static_cast<const base::Value*>(value));

    default:
      LOG(ERROR) << "Unexpected value type: " << value->type();
      return v8::Null(isolate);
  }
}

v8::Local<v8::Value> V8ValueConverter::ToV8Array(
    v8::Isolate* isolate,
    const base::ListValue* val) const {
  v8::Local<v8::Array> result(v8::Array::New(isolate, val->GetList().size()));
  auto context = isolate->GetCurrentContext();

  for (size_t i = 0; i < val->GetList().size(); ++i) {
    const base::Value* child = nullptr;
    val->Get(i, &child);

    v8::Local<v8::Value> child_v8 = ToV8ValueImpl(isolate, child);

    v8::TryCatch try_catch(isolate);
    result->Set(context, static_cast<uint32_t>(i), child_v8).Check();
    if (try_catch.HasCaught())
      LOG(ERROR) << "Setter for index " << i << " threw an exception.";
  }

  return result;
}

v8::Local<v8::Value> V8ValueConverter::ToV8Object(
    v8::Isolate* isolate,
    const base::DictionaryValue* val) const {
  gin_helper::Dictionary result = gin::Dictionary::CreateEmpty(isolate);
  result.SetHidden("simple", true);

  for (base::DictionaryValue::Iterator iter(*val); !iter.IsAtEnd();
       iter.Advance()) {
    const std::string& key = iter.key();
    v8::Local<v8::Value> child_v8 = ToV8ValueImpl(isolate, &iter.value());

    v8::TryCatch try_catch(isolate);
    result.Set(key, child_v8);
    if (try_catch.HasCaught()) {
      LOG(ERROR) << "Setter for property " << key.c_str() << " threw an "
                 << "exception.";
    }
  }

  return result.GetHandle();
}

v8::Local<v8::Value> V8ValueConverter::ToArrayBuffer(
    v8::Isolate* isolate,
    const base::Value* value) const {
  const auto* data = reinterpret_cast<const char*>(value->GetBlob().data());
  size_t length = value->GetBlob().size();

  if (NodeBindings::IsInitialized()) {
    return node::Buffer::Copy(isolate, data, length).ToLocalChecked();
  }

  if (length > node::Buffer::kMaxLength) {
    return v8::Local<v8::Object>();
  }
  auto context = isolate->GetCurrentContext();
  auto array_buffer = v8::ArrayBuffer::New(isolate, length);
  std::shared_ptr<v8::BackingStore> backing_store =
      array_buffer->GetBackingStore();
  memcpy(backing_store->Data(), data, length);
  // 从这一点来看，如果出现问题(找不到的缓冲区类。
  // 示例)我们将根据创建的ArrayBuffer返回一个Uint8Array。
  // 如果未为渲染器指定预加载脚本，则可能会发生这种情况。
  gin_helper::Dictionary global(isolate, context->Global());
  v8::Local<v8::Value> buffer_value;

  // 获取在全局对象中存储为隐藏值的缓冲区类。我们会。
  // 使用它返回一个浏览化的缓冲区。
  if (!global.GetHidden("Buffer", &buffer_value) ||
      !buffer_value->IsFunction()) {
    return v8::Uint8Array::New(array_buffer, 0, length);
  }

  gin::Dictionary buffer_class(
      isolate,
      buffer_value->ToObject(isolate->GetCurrentContext()).ToLocalChecked());
  v8::Local<v8::Value> from_value;
  if (!buffer_class.Get("from", &from_value) || !from_value->IsFunction()) {
    return v8::Uint8Array::New(array_buffer, 0, length);
  }

  v8::Local<v8::Value> args[] = {array_buffer};
  auto func = from_value.As<v8::Function>();
  auto result = func->Call(context, v8::Null(isolate), 1, args);
  if (!result.IsEmpty()) {
    return result.ToLocalChecked();
  }

  return v8::Uint8Array::New(array_buffer, 0, length);
}

std::unique_ptr<base::Value> V8ValueConverter::FromV8ValueImpl(
    FromV8ValueState* state,
    v8::Local<v8::Value> val,
    v8::Isolate* isolate) const {
  FromV8ValueState::Level state_level(state);
  if (state->HasReachedMaxRecursionDepth())
    return nullptr;

  if (val->IsExternal())
    return std::make_unique<base::Value>();

  if (val->IsNull())
    return std::make_unique<base::Value>();

  auto context = isolate->GetCurrentContext();

  if (val->IsBoolean())
    return std::make_unique<base::Value>(val->ToBoolean(isolate)->Value());

  if (val->IsInt32())
    return std::make_unique<base::Value>(val.As<v8::Int32>()->Value());

  if (val->IsNumber()) {
    double val_as_double = val.As<v8::Number>()->Value();
    if (!std::isfinite(val_as_double))
      return nullptr;
    return std::make_unique<base::Value>(val_as_double);
  }

  if (val->IsString()) {
    v8::String::Utf8Value utf8(isolate, val);
    return std::make_unique<base::Value>(std::string(*utf8, utf8.length()));
  }

  if (val->IsUndefined())
    // JSON.stringify忽略未定义的。
    return nullptr;

  if (val->IsDate()) {
    v8::Date* date = v8::Date::Cast(*val);
    v8::Local<v8::Value> toISOString =
        date->Get(context, v8::String::NewFromUtf8(isolate, "toISOString",
                                                   v8::NewStringType::kNormal)
                               .ToLocalChecked())
            .ToLocalChecked();
    if (toISOString->IsFunction()) {
      v8::MaybeLocal<v8::Value> result =
          toISOString.As<v8::Function>()->Call(context, val, 0, nullptr);
      if (!result.IsEmpty()) {
        v8::String::Utf8Value utf8(isolate, result.ToLocalChecked());
        return std::make_unique<base::Value>(std::string(*utf8, utf8.length()));
      }
    }
  }

  if (val->IsRegExp()) {
    if (!reg_exp_allowed_)
      // JSON.stringify转换为对象。
      return FromV8Object(val.As<v8::Object>(), state, isolate);
    return std::make_unique<base::Value>(*v8::String::Utf8Value(isolate, val));
  }

  // 由于某些原因，V8：：Value没有ToArray()方法。
  if (val->IsArray())
    return FromV8Array(val.As<v8::Array>(), state, isolate);

  if (val->IsFunction()) {
    if (!function_allowed_)
      // JSON.stringify拒绝转换function(){}。
      return nullptr;
    return FromV8Object(val.As<v8::Object>(), state, isolate);
  }

  if (node::Buffer::HasInstance(val)) {
    return FromNodeBuffer(val, state, isolate);
  }

  if (val->IsObject()) {
    return FromV8Object(val.As<v8::Object>(), state, isolate);
  }

  LOG(ERROR) << "Unexpected v8 value type encountered.";
  return nullptr;
}

std::unique_ptr<base::Value> V8ValueConverter::FromV8Array(
    v8::Local<v8::Array> val,
    FromV8ValueState* state,
    v8::Isolate* isolate) const {
  ScopedUniquenessGuard uniqueness_guard(state, val);
  if (!uniqueness_guard.is_valid())
    return std::make_unique<base::Value>();

  std::unique_ptr<v8::Context::Scope> scope;
  // 如果val是在与当前上下文不同的上下文中创建的，请更改为。
  // 该上下文，但在Val转换后更改回来。
  if (!val->CreationContext().IsEmpty() &&
      val->CreationContext() != isolate->GetCurrentContext())
    scope = std::make_unique<v8::Context::Scope>(val->CreationContext());

  auto result = std::make_unique<base::ListValue>();

  // 只有具有整数键的字段才会传递到ListValue。
  for (uint32_t i = 0; i < val->Length(); ++i) {
    v8::TryCatch try_catch(isolate);
    v8::Local<v8::Value> child_v8;
    v8::MaybeLocal<v8::Value> maybe_child =
        val->Get(isolate->GetCurrentContext(), i);
    if (try_catch.HasCaught() || !maybe_child.ToLocal(&child_v8)) {
      LOG(ERROR) << "Getter for index " << i << " threw an exception.";
      child_v8 = v8::Null(isolate);
    }

    if (!val->HasRealIndexedProperty(isolate->GetCurrentContext(), i)
             .FromMaybe(false)) {
      result->Append(std::make_unique<base::Value>());
      continue;
    }

    std::unique_ptr<base::Value> child =
        FromV8ValueImpl(state, child_v8, isolate);
    if (child)
      result->Append(std::move(child));
    else
      // Stringify将NULL放在值不序列化的位置，FOR。
      // 示例未定义的AND函数。效仿这一行为。
      result->Append(std::make_unique<base::Value>());
  }
  return std::move(result);
}

std::unique_ptr<base::Value> V8ValueConverter::FromNodeBuffer(
    v8::Local<v8::Value> value,
    FromV8ValueState* state,
    v8::Isolate* isolate) const {
  std::vector<char> buffer(
      node::Buffer::Data(value),
      node::Buffer::Data(value) + node::Buffer::Length(value));
  return std::make_unique<base::Value>(std::move(buffer));
}

std::unique_ptr<base::Value> V8ValueConverter::FromV8Object(
    v8::Local<v8::Object> val,
    FromV8ValueState* state,
    v8::Isolate* isolate) const {
  ScopedUniquenessGuard uniqueness_guard(state, val);
  if (!uniqueness_guard.is_valid())
    return std::make_unique<base::Value>();

  std::unique_ptr<v8::Context::Scope> scope;
  // 如果val是在与当前上下文不同的上下文中创建的，请更改为。
  // 该上下文，但在Val转换后更改回来。
  if (!val->CreationContext().IsEmpty() &&
      val->CreationContext() != isolate->GetCurrentContext())
    scope = std::make_unique<v8::Context::Scope>(val->CreationContext());

  auto result = std::make_unique<base::DictionaryValue>();
  v8::Local<v8::Array> property_names;
  if (!val->GetOwnPropertyNames(isolate->GetCurrentContext())
           .ToLocal(&property_names)) {
    return std::move(result);
  }

  for (uint32_t i = 0; i < property_names->Length(); ++i) {
    v8::Local<v8::Value> key =
        property_names->Get(isolate->GetCurrentContext(), i).ToLocalChecked();

    // 根据需要和明智，扩展此测试以涵盖更多类型。
    if (!key->IsString() && !key->IsNumber()) {
      NOTREACHED() << "Key \"" << *v8::String::Utf8Value(isolate, key)
                   << "\" "
                      "is neither a string nor a number";
      continue;
    }

    v8::String::Utf8Value name_utf8(isolate, key);

    v8::TryCatch try_catch(isolate);
    v8::Local<v8::Value> child_v8;
    v8::MaybeLocal<v8::Value> maybe_child =
        val->Get(isolate->GetCurrentContext(), key);
    if (try_catch.HasCaught() || !maybe_child.ToLocal(&child_v8)) {
      LOG(ERROR) << "Getter for property " << *name_utf8
                 << " threw an exception.";
      child_v8 = v8::Null(isolate);
    }

    std::unique_ptr<base::Value> child =
        FromV8ValueImpl(state, child_v8, isolate);
    if (!child)
      // JSON.stringify跳过值未序列化的属性，对于。
      // 示例未定义的AND函数。效仿这一行为。
      continue;

    // 如果询问，则删除NULL(由于UNDEFINED将变为NULL，因此未定义。
    // 也是如此)。支持这一点的用例是JSON模式支持，
    // 特别适用于扩展，其中“可选的”JSON属性可能是。
    // 表示为空，但由于其他地方的遗留错误代码不是。
    // 被这样对待(可能会导致撞车)。例如，
    // “tab.create”函数将一个对象作为其第一个参数，并使用。
    // 可选的“windowId”属性。
    // 
    // 刚刚给的。
    // 
    // Tab.create({})。
    // 
    // 这将在只检查是否存在的代码上按预期工作。
    // “windowId”属性(例如该遗留代码)。无论如何，给定的。
    // 
    // Tab.create({windowId：null})。
    // 
    // 有*个“windowId”属性，但因为它应该是int，所以代码。
    // 在没有额外检查NULL的浏览器上，它将失败。
    // 我们可以通过去掉NULL来避免与此相关的所有错误。
    if (strip_null_from_objects_ && child->is_none())
      continue;

    result->SetWithoutPathExpansion(std::string(*name_utf8, name_utf8.length()),
                                    std::move(child));
  }

  return std::move(result);
}

}  // 命名空间电子
