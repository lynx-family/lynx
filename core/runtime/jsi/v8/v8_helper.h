// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JSI_V8_V8_HELPER_H_
#define CORE_RUNTIME_JSI_V8_V8_HELPER_H_

#include <string>

#include "core/runtime/jsi/jsi.h"
#include "v8.h"

#define ENTER_SCOPE(ctx)                     \
  v8::Isolate* isolate = ctx->GetIsolate();  \
  v8::Isolate::Scope isolate_scope(isolate); \
  v8::HandleScope handle_scope(isolate);     \
  v8::Context::Scope context_scope(ctx);

#define ENTER_ISO_SCOPE(isolate, ctx)                \
  v8::Isolate::Scope isolate_scope(isolate);         \
  v8::HandleScope handle_scope(isolate);             \
  v8::Local<v8::Context> context = ctx.Get(isolate); \
  v8::Context::Scope context_scope(context);

namespace lynx {
namespace piper {
class V8Runtime;
namespace detail {

#define ESCAPED_SCOPE(ctx)

// TODO: is the symbol protected?
class V8SymbolValue final : public Runtime::PointerValue {
 public:
  V8SymbolValue(v8::Isolate* iso, v8::Local<v8::Symbol> sym);
  void invalidate() override;
  virtual std::string Name() override { return "V8SymbolValue"; }
  v8::Local<v8::Symbol> Get() const;

  v8::Isolate* iso_;
  v8::Persistent<v8::Symbol> sym_;

 protected:
  friend class V8Runtime;
  friend class V8Helper;
};

class V8StringValue final : public Runtime::PointerValue {
 public:
  V8StringValue(v8::Isolate* iso, v8::Local<v8::String> str);
  void invalidate() override;
  virtual std::string Name() override { return "V8StringValue"; }
  v8::Local<v8::String> Get() const;
  v8::Isolate* iso_;
  v8::Persistent<v8::String> str_;

 protected:
  friend class V8Runtime;
  friend class V8Helper;
};

// C++ object wrapper for v8::Object. The underlying v8::Object can be
// optionally protected. You must protect the object if it is ever
// heap-allocated, since otherwise you may end up with an invalid reference.
class V8ObjectValue final : public Runtime::PointerValue {
 public:
  V8ObjectValue(v8::Isolate* iso, v8::Local<v8::Object> obj);

  void invalidate() override;
  virtual std::string Name() override { return "V8ObjectValue"; }
  v8::Local<v8::Object> Get() const;

  v8::Isolate* iso_;  // TODO: use context or isolate?
  v8::Persistent<v8::Object> obj_;
  bool m_isProtected = false;

 protected:
  friend class V8Runtime;
  friend class V8Helper;
};

class V8Helper {
 public:
  static piper::Value createValue(v8::Local<v8::Value> value,
                                  v8::Local<v8::Context>);
  static v8::Local<v8::Value> symbolRef(const piper::Symbol& sym);
  static v8::Local<v8::String> stringRef(const piper::String& str);
  static v8::Local<v8::String> stringRef(const piper::PropNameID& sym);
  static v8::Local<v8::Object> objectRef(const piper::Object& obj);

  static std::string JSStringToSTLString(v8::Local<v8::String> s,
                                         v8::Local<v8::Context> ctx);
  static std::string JSStringToSTLString(v8::Local<v8::String> s,
                                         v8::Isolate* iso);
  // Factory methods for creating String/Object
  static piper::Symbol createSymbol(v8::Local<v8::Symbol> symbolRef,
                                    v8::Isolate* iso);
  static piper::String createString(v8::Local<v8::String> stringRef,
                                    v8::Isolate*);
  static piper::PropNameID createPropNameID(v8::Local<v8::String> stringRef,
                                            v8::Isolate*);
  static piper::PropNameID createPropNameID(v8::Local<v8::Symbol> symbol,
                                            v8::Isolate*);
#if OS_ANDROID
  static piper::PropNameID createPropNameID(v8::Local<v8::Name> symbol,
                                            v8::Isolate*);
#else
  static piper::PropNameID createPropNameID(v8::Local<v8::Name> symbol,
                                            v8::Local<v8::Context> ctx);
#endif
  static piper::Object createObject(v8::Isolate* iso);
  static piper::Object createObject(v8::Local<v8::Object> objectRef,
                                    v8::Isolate* iso);

  // Used by factory methods and clone methods
  static piper::Runtime::PointerValue* makeSymbolValue(
      v8::Local<v8::Symbol> sym, v8::Isolate* iso);
  static piper::Runtime::PointerValue* makeStringValue(
      v8::Local<v8::String> str, v8::Isolate*);
  static piper::Runtime::PointerValue* makeObjectValue(
      v8::Local<v8::Object> obj, v8::Isolate* iso);

  static std::optional<piper::Value> call(piper::V8Runtime* rt,
                                          const piper::Function& f,
                                          const piper::Object& jsThis,
                                          v8::Local<v8::Value>* args,
                                          size_t nArgs);

  static std::optional<piper::Value> callAsConstructor(
      piper::V8Runtime* rt, v8::Local<v8::Object> obj,
      v8::Local<v8::Value>* args, int nArgs);

  static v8::Local<v8::String> ConvertToV8String(v8::Isolate* isolate,
                                                 const std::string& s);

  static void ThrowJsException(v8::Isolate* isolate,
                               const JSINativeException& exception);
};
/*
class ArgsConverter {
 public:
  ArgsConverter(v8::Local<v8::Context> ctx,
                const piper::Value* args,
                size_t count) {
    v8::Local<v8::Value>* destination = inline_;
    if (count > maxStackArgs) {
      outOfLine_ = std::make_unique<v8::Local<v8::Value>[]>(count);
      destination = outOfLine_.get();
    }

    for (size_t i = 0; i < count; ++i) {
      destination[i] = stringRef(args[i], ctx);
    }
  }

  operator v8::Local<v8::Value>*() {
    return outOfLine_ ? outOfLine_.get() : inline_;
  }

 private:
  constexpr static unsigned maxStackArgs = 8;
  v8::Local<v8::Value> inline_[maxStackArgs];
  std::unique_ptr<v8::Local<v8::Value>[]> outOfLine_;
};*/

}  // namespace detail
}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_JSI_V8_V8_HELPER_H_
