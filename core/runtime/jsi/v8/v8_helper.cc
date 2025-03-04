// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/jsi/v8/v8_helper.h"

#include "base/include/log/logging.h"
#include "core/runtime/jsi/v8/v8_exception.h"
#include "core/runtime/jsi/v8/v8_runtime.h"

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
#include "base/trace/native/instance_counter_trace.h"
#endif

namespace lynx {
namespace piper {
namespace detail {

V8SymbolValue::V8SymbolValue(v8::Isolate* iso, v8::Local<v8::Symbol> sym) {
  iso_ = iso;
  sym_.Reset(iso, sym);
}

void V8SymbolValue::invalidate() {
  if (!sym_.IsEmpty()) {
    sym_.Reset();
  }
  delete this;
}

v8::Local<v8::Symbol> V8SymbolValue::Get() const { return sym_.Get(iso_); }

V8StringValue::V8StringValue(v8::Isolate* iso, v8::Local<v8::String> str) {
  iso_ = iso;
  str_.Reset(iso, str);
}

void V8StringValue::invalidate() {
  if (!str_.IsEmpty()) {
    str_.Reset();
  }
  delete this;
}

v8::Local<v8::String> V8StringValue::Get() const { return str_.Get(iso_); }

V8ObjectValue::V8ObjectValue(v8::Isolate* iso, v8::Local<v8::Object> obj)
    : iso_(iso) {
  // m_isProtected = isProtected;
  obj_.Reset(iso, obj);
  // if (!m_isProtected) {
  //   MakeWeak();
  // }
}

void V8ObjectValue::invalidate() {
  if (!obj_.IsEmpty()) {
    obj_.ClearWeak();
    obj_.Reset();
  }
  delete this;
}

v8::Local<v8::Object> V8ObjectValue::Get() const { return obj_.Get(iso_); }

std::string V8Helper::JSStringToSTLString(v8::Local<v8::String> s,
                                          v8::Local<v8::Context> ctx) {
  return JSStringToSTLString(s, ctx->GetIsolate());
}

std::string V8Helper::JSStringToSTLString(v8::Local<v8::String> s,
                                          v8::Isolate* iso) {
  if (s.IsEmpty()) {
    return std::string("");
  }
  if (s->IsNull()) {
    return std::string("Null");
  } else if (!(s->IsString() || s->IsStringObject())) {
    return std::string("value from v8 is not a string or string object");
  } else {
    v8::String::Utf8Value str(iso, s);
    return std::string(*str);
  }
}

piper::Value V8Helper::createValue(v8::Local<v8::Value> value,
                                   v8::Local<v8::Context> ctx) {
  if (value->IsNumber()) {
    return piper::Value(value->NumberValue(ctx).ToChecked());
  } else if (value->IsBoolean()) {
    return piper::Value(value->BooleanValue(ctx->GetIsolate()));
  } else if (value->IsNull()) {
    return piper::Value(nullptr);
  } else if (value->IsUndefined()) {
    return piper::Value();
  } else if (value->IsString()) {
    v8::Local<v8::String> str = value->ToString(ctx).ToLocalChecked();
    auto result = piper::Value(createString(str, ctx->GetIsolate()));
    return result;
  } else if (value->IsObject()) {
    v8::Local<v8::Object> objRef = value->ToObject(ctx).ToLocalChecked();
    return piper::Value(createObject(objRef, ctx->GetIsolate()));
  } else if (value->IsSymbol()) {
    return piper::Value(
        createSymbol(value.As<v8::Symbol>(), ctx->GetIsolate()));
  } else {
    // WHAT ARE YOU
    abort();
  }
}

v8::Local<v8::Value> V8Helper::symbolRef(const piper::Symbol& sym) {
  const V8SymbolValue* v8_sym =
      static_cast<const V8SymbolValue*>(Runtime::getPointerValue(sym));
  return v8_sym->Get();
}

v8::Local<v8::String> V8Helper::stringRef(const piper::String& str) {
  auto v8_str =
      static_cast<const V8StringValue*>(Runtime::getPointerValue(str));
  return v8_str->Get();
}

v8::Local<v8::String> V8Helper::stringRef(const piper::PropNameID& sym) {
  auto v8_str =
      static_cast<const V8StringValue*>(Runtime::getPointerValue(sym));
  return v8_str->Get();
}

v8::Local<v8::Object> V8Helper::objectRef(const piper::Object& obj) {
  const V8ObjectValue* v8_obj =
      static_cast<const V8ObjectValue*>(Runtime::getPointerValue(obj));
  return v8_obj->Get();
}

piper::Runtime::PointerValue* V8Helper::makeSymbolValue(
    v8::Local<v8::Symbol> symbolRef, v8::Isolate* iso) {
  return new V8SymbolValue(iso, symbolRef);
}

namespace {
v8::Local<v8::String> getEmptyString(v8::Isolate* iso) {
#if V8_MAJOR_VERSION >= 9
  static v8::Local<v8::String> empty =
      v8::String::NewFromUtf8(iso, "", v8::NewStringType::kNormal)
          .ToLocalChecked();
#else
  static v8::Local<v8::String> empty = v8::String::NewFromUtf8(iso, "");
#endif
  return empty;
}
}  // namespace

piper::Runtime::PointerValue* V8Helper::makeStringValue(
    v8::Local<v8::String> stringRef, v8::Isolate* iso) {
  if (stringRef.IsEmpty()) {
    stringRef = getEmptyString(iso);
  }
  return new V8StringValue(iso, stringRef);
}

piper::Symbol V8Helper::createSymbol(v8::Local<v8::Symbol> sym,
                                     v8::Isolate* iso) {
  return Runtime::make<piper::Symbol>(makeSymbolValue(sym, iso));
}

piper::String V8Helper::createString(v8::Local<v8::String> str,
                                     v8::Isolate* iso) {
  return Runtime::make<piper::String>(makeStringValue(str, iso));
}

piper::PropNameID V8Helper::createPropNameID(v8::Local<v8::Symbol> symbol,
                                             v8::Isolate* iso) {
  return Runtime::make<piper::PropNameID>(makeSymbolValue(symbol, iso));
}

piper::PropNameID V8Helper::createPropNameID(v8::Local<v8::String> str,
                                             v8::Isolate* iso) {
  return Runtime::make<piper::PropNameID>(makeStringValue(str, iso));
}

#if OS_ANDROID
piper::PropNameID V8Helper::createPropNameID(v8::Local<v8::Name> name,
                                             v8::Isolate* iso) {
  if (name->IsString()) {
    v8::Local<v8::String> v8_str =
        name->ToString(iso->GetCurrentContext()).ToLocalChecked();
    return createPropNameID(v8_str, iso);
  }
  if (name->IsSymbol()) {
    v8::Local<v8::Symbol> v8_sym = v8::Local<v8::Symbol>::Cast(name);
    return createPropNameID(v8_sym, iso);
  }
  // never go to here
  abort();
}
#else
piper::PropNameID V8Helper::createPropNameID(v8::Local<v8::Name> name,
                                             v8::Local<v8::Context> ctx) {
  if (name->IsString()) {
    v8::Local<v8::String> v8_str = name->ToString(ctx).ToLocalChecked();
    return createPropNameID(v8_str, ctx->GetIsolate());
  }
  if (name->IsSymbol()) {
    v8::Local<v8::Symbol> v8_sym = v8::Local<v8::Symbol>::Cast(name);
    return createPropNameID(v8_sym, ctx->GetIsolate());
  }
  // never go to here
  abort();
}
#endif

piper::Runtime::PointerValue* V8Helper::makeObjectValue(
    v8::Local<v8::Object> objectRef, v8::Isolate* iso) {
  if (iso == nullptr) {
    LOGE("iso is null!");
  }
  if (objectRef.IsEmpty()) {
    objectRef = v8::Object::New(iso);
  }
  return new V8ObjectValue(iso, objectRef);
}

piper::Object V8Helper::createObject(v8::Isolate* iso) {
  return createObject(v8::Local<v8::Object>(), iso);
}

piper::Object V8Helper::createObject(v8::Local<v8::Object> obj,
                                     v8::Isolate* iso) {
  return Runtime::make<piper::Object>(makeObjectValue(obj, iso));
}

std::optional<piper::Value> V8Helper::call(piper::V8Runtime* rt,
                                           const piper::Function& f,
                                           const piper::Object& jsThis,
                                           v8::Local<v8::Value>* args,
                                           size_t nArgs) {
  auto ctx = rt->getContext();
  v8::Local<v8::Object> thisObj = V8Helper::objectRef(jsThis);
  v8::Local<v8::Object> func = objectRef(f);

  v8::Isolate* isolate = ctx->GetIsolate();
  v8::Isolate::Scope isolate_scope(isolate);
  v8::EscapableHandleScope handleScope(isolate);
  v8::Local<v8::Context> context = ctx;
  v8::Context::Scope context_scope(context);
  v8::TryCatch try_catch(isolate);

  v8::Local<v8::Object> target_object =
      thisObj.IsEmpty() ? context->Global() : thisObj;

  v8::MaybeLocal<v8::Value> result = func->CallAsFunction(
      context, target_object, static_cast<int>(nArgs), args);
#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
  if (isolate) {
    v8::HeapStatistics heapStatistics;
    isolate->GetHeapStatistics(&heapStatistics);
    lynx::trace::InstanceCounterTrace::JsHeapMemoryUsedTrace(
        heapStatistics.used_heap_size());
  }
#endif
  if (try_catch.HasCaught()) {
    // Actually there is no need to check if try_catch.Exception() is empty
    // handle. Since v8 will only return an empty handle when HasCaught() is
    // false. Here we make a IsEmpty() check just to ensure not crashing. See:
    // https://chromium.googlesource.com/v8/v8.git/+/dc7926bebd49d8749074c414dcf08a846bae0007/src/api/api.cc#3090
    // Also see:
    // https://chromium.googlesource.com/v8/v8.git/+/dc7926bebd49d8749074c414dcf08a846bae0007/src/api/api.cc#3118
    if (!try_catch.Exception().IsEmpty()) {
      rt->reportJSIException(V8Exception(*rt, try_catch.Exception()));
    }
    return std::optional<piper::Value>();
  }

  if (result.IsEmpty()) {
    // This result will only be an empty handle when an exception occurred.
    // So this if-block should never be entered.
    // Here we make a IsEmpty() check just to ensure not crashing.
    // See:
    // https://chromium.googlesource.com/v8/v8.git/+/dc7926bebd49d8749074c414dcf08a846bae0007/src/api/api.cc#5337
    rt->reportJSIException(BUILD_JSI_NATIVE_EXCEPTION(
        "Exception calling object as function. MaybeLocal empty."));
    return std::optional<piper::Value>();
  }
  return createValue(handleScope.Escape(result.ToLocalChecked()), context);
}

std::optional<piper::Value> V8Helper::callAsConstructor(
    piper::V8Runtime* rt, v8::Local<v8::Object> v8obj,
    v8::Local<v8::Value>* args, int nArgs) {
  auto ctx = rt->getContext();
  v8::Isolate* isolate = ctx->GetIsolate();
  v8::Isolate::Scope isolate_scope(isolate);
  v8::EscapableHandleScope handleScope(isolate);
  v8::Local<v8::Context> context = ctx;
  v8::Context::Scope context_scope(context);
  v8::TryCatch trycatch(isolate);

  v8::MaybeLocal<v8::Value> result =
      v8obj->CallAsConstructor(context, nArgs, args);

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
  if (isolate) {
    v8::HeapStatistics heapStatistics;
    isolate->GetHeapStatistics(&heapStatistics);
    lynx::trace::InstanceCounterTrace::JsHeapMemoryUsedTrace(
        heapStatistics.used_heap_size());
  }
#endif
  if (!V8Exception::ReportExceptionIfNeeded(*rt, trycatch) ||
      result.IsEmpty()) {
    return std::optional<piper::Value>();
  }
  return createValue(
      handleScope.Escape(
          result.ToLocalChecked()->ToObject(context).ToLocalChecked()),
      context);
}

v8::Local<v8::String> V8Helper::ConvertToV8String(v8::Isolate* isolate,
                                                  const std::string& s) {
  v8::Local<v8::String> str =
      v8::String::NewFromUtf8(isolate, s.c_str(), v8::NewStringType::kNormal)
          .ToLocalChecked();
  return str;
}

void V8Helper::ThrowJsException(v8::Isolate* isolate,
                                const JSINativeException& exception) {
  DCHECK(!exception.message().empty());
  auto message = ConvertToV8String(isolate, exception.message());
  auto exception_value = v8::Exception::Error(message);
  auto maybe_exception_obj =
      exception_value->ToObject(isolate->GetCurrentContext());
  v8::Local<v8::Object> exception_obj;
  if (!maybe_exception_obj.ToLocal(&exception_obj)) {
    LOGE("error create failed.");
    return;
  }
  if (exception.IsJSError()) {
    auto ret = exception_obj->Set(
        isolate->GetCurrentContext(), ConvertToV8String(isolate, "stack"),
        ConvertToV8String(isolate, exception.stack()));
    if (!ret.IsJust() || !ret.FromJust()) {
      LOGE("stack add failed");
    }
    ret = exception_obj->Set(isolate->GetCurrentContext(),
                             ConvertToV8String(isolate, "name"),
                             ConvertToV8String(isolate, exception.name()));
    if (!ret.IsJust() || !ret.FromJust()) {
      LOGE("name add failed");
    }
  } else {
    auto ret = exception_obj->Set(
        isolate->GetCurrentContext(), ConvertToV8String(isolate, "cause"),
        ConvertToV8String(isolate, exception.stack()));
    if (!ret.IsJust() || !ret.FromJust()) {
      LOGE("cause add failed");
    }
  }
  isolate->ThrowException(exception_value);
}

}  // namespace detail
}  // namespace piper
}  // namespace lynx
