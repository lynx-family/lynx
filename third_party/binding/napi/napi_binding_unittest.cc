// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <string>

#include "core/runtime/bindings/napi/napi_environment.h"
#include "core/runtime/bindings/napi/napi_runtime_proxy.h"
#include "core/runtime/bindings/napi/napi_runtime_proxy_quickjs.h"
#include "core/runtime/bindings/napi/shim/shim_napi_env_quickjs.h"
#include "base/include/string/string_utils.h"
#include "third_party/binding/napi/napi_bridge.h"
#include "third_party/binding/napi/napi_object.h"
#include "third_party/binding/napi/napi_object_ref.h"
#include "third_party/binding/napi/napi_value.h"
#include "third_party/binding/napi/shim/shim_napi.h"
#include "third_party/binding/napi/shim/shim_napi_env.h"
#include "third_party/binding/napi/shim/shim_napi_runtime.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus
#include "third_party/quickjs/include/quickjs.h"
#ifdef __cplusplus
}
#endif  // __cplusplus

namespace lynx {
namespace binding {

#define AssertExp(exp) EXPECT_EQ(env_.RunScript(exp).ToBoolean().Value(), true);

class NapiBindingTest : public ::testing::Test {
 public:
  NapiBindingTest()
      : rt_(LEPUS_NewRuntime()),
        ctx_(LEPUS_NewContext(rt_)),
        runtime_proxy_(static_cast<piper::NapiRuntimeProxy*>(
            piper::NapiRuntimeProxyQuickjs::Create(ctx_).release())),
        e(std::make_unique<TestDelegate>()),
        env_(runtime_proxy_->Env()) {
    e.SetRuntimeProxy(std::move(runtime_proxy_));
    e.Attach();
  }

  ~NapiBindingTest() override { Dispose(); }

  void SetUp() override {}

  void Dispose() {
    if (disposed_) {
      return;
    }
    e.Detach();
    LEPUS_FreeContext(ctx_);
    LEPUS_FreeRuntime(rt_);
    disposed_ = true;
  }

 protected:
  LEPUSRuntime* rt_;

 private:
  class TestDelegate : public piper::NapiEnvironment::Delegate {
    void OnAttach(Napi::Env env) override {}
  };

  LEPUSContext* ctx_;
  std::unique_ptr<piper::NapiRuntimeProxy> runtime_proxy_;
  piper::NapiEnvironment e;
  bool disposed_ = false;

 protected:
  Napi::Env env_;
};

TEST_F(NapiBindingTest, EnvTest) {
  // Env created by Napi::Env should be Napi.
  Env env = FromNAPI(env_);
  EXPECT_TRUE(env.IsNapi());
  EXPECT_FALSE(env.IsRemote());

  // Env should be able to converted back to Napi::Env.
  Napi::Env nenv = ToNAPI(env);
  EXPECT_EQ(nenv, env_);

  // Test setting instance data from Env.
  EXPECT_TRUE(env.GetInstanceData<int>(1) == nullptr);
  EXPECT_TRUE(env_.GetInstanceData<int>(1) == nullptr);
  auto* data1 = new int;
  env.SetInstanceData(1, data1);
  EXPECT_EQ(env.GetInstanceData<int>(1), data1);
  EXPECT_EQ(env_.GetInstanceData<int>(1), data1);

  // Test setting instance data from Napi::Env.
  EXPECT_TRUE(env.GetInstanceData<int>(2) == nullptr);
  EXPECT_TRUE(env_.GetInstanceData<int>(2) == nullptr);
  auto* data2 = new int;
  env_.SetInstanceData(2, data2);
  EXPECT_EQ(env.GetInstanceData<int>(2), data2);
  EXPECT_EQ(env_.GetInstanceData<int>(2), data2);

  // Test setting instance data with finalizer.
  static bool s_set_by_finalizer = false;
  EXPECT_TRUE(env.GetInstanceData<int>(3) == nullptr);
  auto* data3 = new int;
  env.SetInstanceData(
      3, data3,
      [](Env env, void* data, void* hint) { s_set_by_finalizer = true; },
      nullptr);
  EXPECT_EQ(env.GetInstanceData<int>(3), data3);
  EXPECT_EQ(env_.GetInstanceData<int>(3), data3);
  EXPECT_FALSE(s_set_by_finalizer);
  Dispose();
  EXPECT_TRUE(s_set_by_finalizer);
}

TEST_F(NapiBindingTest, ObjectTest) {
  // Empty object should be empty.
  Object empty = Object::CreateEmpty();
  EXPECT_TRUE(empty.IsEmpty());
  EXPECT_FALSE(empty.IsNapi());
  EXPECT_FALSE(empty.IsRemote());

  // Object created from empty Napi::Object should also be empty.
  Object empty1 = FromNAPI(Napi::Object());
  EXPECT_TRUE(empty1.IsEmpty());
  EXPECT_FALSE(empty1.IsNapi());
  EXPECT_FALSE(empty1.IsRemote());

  // Napi::Object should be Napi.
  Napi::Object obj = Napi::Object::New(env_);
  Object o = FromNAPI(obj);
  EXPECT_FALSE(o.IsEmpty());
  EXPECT_TRUE(o.IsNapi());
  EXPECT_FALSE(o.IsRemote());

  // Object should be able to converted back to the same Napi::Object.
  Napi::Object obj1 = ToNAPI(o);
  EXPECT_EQ(obj, obj1);
  obj["foo"] = Napi::String::New(env_, "bar");
  EXPECT_EQ(obj.Get("foo").As<Napi::String>().Utf8Value(), "bar");
  EXPECT_EQ(obj1.Get("foo").As<Napi::String>().Utf8Value(), "bar");

  // Test object copy.
  Object o1 = o;
  EXPECT_FALSE(o1.IsEmpty());
  EXPECT_TRUE(o1.IsNapi());
  EXPECT_FALSE(o1.IsRemote());
  EXPECT_EQ(ToNAPI(o1), obj1);
  EXPECT_FALSE(o.IsEmpty());
  Object empty2 = empty1;
  EXPECT_TRUE(empty2.IsEmpty());
  EXPECT_FALSE(empty2.IsNapi());
  EXPECT_FALSE(empty2.IsRemote());

  // Test object move.
  Object o2 = std::move(o);
  EXPECT_FALSE(o2.IsEmpty());
  EXPECT_TRUE(o2.IsNapi());
  EXPECT_FALSE(o2.IsRemote());
  EXPECT_EQ(ToNAPI(o2), ToNAPI(o1));
  EXPECT_TRUE(o.IsEmpty());
  Object empty3 = std::move(empty1);
  EXPECT_TRUE(empty3.IsEmpty());
  EXPECT_FALSE(empty3.IsNapi());
  EXPECT_FALSE(empty3.IsRemote());
}

TEST_F(NapiBindingTest, ObjectRefAPITest) {
  // Empty object should produce empty ref.
  Object empty = Object::CreateEmpty();
  ObjectRef empty_ref = empty.AdoptRef();
  EXPECT_TRUE(empty_ref.IsEmpty());
  EXPECT_TRUE(empty_ref.Get().IsEmpty());

  // Test creating ref from object, and getting object from ref.
  Napi::Object obj = Napi::Object::New(env_);
  Object o = FromNAPI(obj);
  ObjectRef ref = o.AdoptRef();
  EXPECT_FALSE(ref.IsEmpty());
  EXPECT_EQ(ToNAPI(ref.Get()), obj);

  // Test copying ref.
  ObjectRef ref1 = ref.Clone();
  EXPECT_FALSE(ref1.IsEmpty());
  EXPECT_EQ(ToNAPI(ref1.Get()), obj);
  ref.Unref();
  EXPECT_TRUE(ref.IsEmpty());
  EXPECT_FALSE(ref1.IsEmpty());
  EXPECT_EQ(ToNAPI(ref1.Get()), obj);

  // Test moving ref.
  ObjectRef ref2 = std::move(ref1);
  EXPECT_TRUE(ref1.IsEmpty());
  EXPECT_FALSE(ref2.IsEmpty());
  EXPECT_EQ(ToNAPI(ref2.Get()), obj);
  ObjectRef empty1 = std::move(empty_ref);
  EXPECT_TRUE(empty_ref.IsEmpty());
  EXPECT_TRUE(empty1.IsEmpty());
}

TEST_F(NapiBindingTest, ObjectRefBehaviorTest) {
  // TODO(yuyifei): actually test the ref behavior.
}

TEST_F(NapiBindingTest, ValueToNapiValueTest) {
  Value empty = Value();
  EXPECT_TRUE(empty.IsEmpty());
  auto napi_empty = ToNAPI(std::move(empty), env_);
  EXPECT_TRUE(napi_empty.IsEmpty());

  Value null = Value::Null();
  EXPECT_TRUE(null.IsNull());
  auto napi_null = ToNAPI(std::move(null), env_);
  EXPECT_TRUE(napi_null.IsNull());

  Value undefined = Value::Undefined();
  EXPECT_TRUE(undefined.IsUndefined());
  auto napi_undefined = ToNAPI(std::move(undefined), env_);
  EXPECT_TRUE(napi_undefined.IsUndefined());

  bool b = true;
  Value boolean = Value::Boolean(b);
  EXPECT_TRUE(boolean.GetType() == ValueType::kBoolean);
  auto napi_boolean = ToNAPI(std::move(boolean), env_);
  EXPECT_TRUE(napi_boolean.IsBoolean());
  EXPECT_TRUE(napi_boolean.As<Napi::Boolean>().Value() == b);

  float f = 1.2;
  Value number = Value::Number(f);
  EXPECT_TRUE(number.GetType() == ValueType::kNumber);
  auto napi_number = ToNAPI(std::move(number), env_);
  EXPECT_TRUE(napi_number.IsNumber());
  EXPECT_TRUE(napi_number.As<Napi::Number>().FloatValue() == f);

  std::string s = "foo";
  Value string = Value::String(s);
  EXPECT_TRUE(string.GetType() == ValueType::kString);
  auto napi_string = ToNAPI(std::move(string), env_);
  EXPECT_TRUE(napi_string.IsString());
  EXPECT_TRUE(napi_string.As<Napi::String>().Utf8Value() == s);

  // Arrays.
  std::vector<int> array = {1, -2, 3};
  Value array_value = Value::Array(array, ArrayType::kTypeInt8);
  EXPECT_TRUE(array_value.GetType() == ValueType::kArray);
  EXPECT_TRUE(array_value.GetArrayType() == ArrayType::kTypeInt8);
  auto napi_array = ToNAPI(std::move(array_value), env_);
  EXPECT_TRUE(napi_array.IsArray());
  EXPECT_TRUE(napi_array.As<Napi::Array>().Length() == array.size());
  for (size_t i = 0; i < array.size(); i++) {
    EXPECT_TRUE(
        napi_array.As<Napi::Array>().Get(i).As<Napi::Number>().Int32Value() ==
        array[i]);
  }

  // No overflow or clamping.
  Value uarray_value = Value::Array(array, ArrayType::kTypeUint8);
  EXPECT_TRUE(uarray_value.GetType() == ValueType::kArray);
  EXPECT_TRUE(uarray_value.GetArrayType() == ArrayType::kTypeUint8);
  napi_array = ToNAPI(std::move(array_value), env_);
  EXPECT_TRUE(napi_array.IsArray());
  EXPECT_TRUE(napi_array.As<Napi::Array>().Length() == array.size());
  for (size_t i = 0; i < array.size(); i++) {
    EXPECT_TRUE(
        napi_array.As<Napi::Array>().Get(i).As<Napi::Number>().Uint32Value() ==
        array[i]);
  }

  array_value = Value::Array(array, ArrayType::kTypeInt32);
  EXPECT_TRUE(array_value.GetType() == ValueType::kArray);
  EXPECT_TRUE(array_value.GetArrayType() == ArrayType::kTypeInt32);
  napi_array = ToNAPI(std::move(array_value), env_);
  EXPECT_TRUE(napi_array.IsArray());
  EXPECT_TRUE(napi_array.As<Napi::Array>().Length() == array.size());
  for (size_t i = 0; i < array.size(); i++) {
    EXPECT_TRUE(
        napi_array.As<Napi::Array>().Get(i).As<Napi::Number>().Int32Value() ==
        array[i]);
  }

  std::vector<uint> uarray = {1, 2, 2147483648};
  uarray_value = Value::Array(uarray, ArrayType::kTypeUint32);
  EXPECT_TRUE(uarray_value.GetType() == ValueType::kArray);
  EXPECT_TRUE(uarray_value.GetArrayType() == ArrayType::kTypeUint32);
  napi_array = ToNAPI(std::move(uarray_value), env_);
  EXPECT_TRUE(napi_array.IsArray());
  EXPECT_TRUE(napi_array.As<Napi::Array>().Length() == uarray.size());
  for (size_t i = 0; i < uarray.size(); i++) {
    EXPECT_TRUE(
        napi_array.As<Napi::Array>().Get(i).As<Napi::Number>().Uint32Value() ==
        uarray[i]);
  }

  std::vector<float> farray = {1.2, 3.4, 5.6};
  Value farray_value = Value::Array(farray, ArrayType::kTypeFloat32);
  EXPECT_TRUE(farray_value.GetType() == ValueType::kArray);
  EXPECT_TRUE(farray_value.GetArrayType() == ArrayType::kTypeFloat32);
  napi_array = ToNAPI(std::move(farray_value), env_);
  EXPECT_TRUE(napi_array.IsArray());
  EXPECT_TRUE(napi_array.As<Napi::Array>().Length() == farray.size());
  for (size_t i = 0; i < farray.size(); i++) {
    EXPECT_TRUE(
        napi_array.As<Napi::Array>().Get(i).As<Napi::Number>().FloatValue() ==
        farray[i]);
  }

  std::vector<double> darray = {1.2, 3.4, 5.6};
  Value darray_value = Value::Array(darray, ArrayType::kTypeFloat64);
  EXPECT_TRUE(darray_value.GetType() == ValueType::kArray);
  EXPECT_TRUE(darray_value.GetArrayType() == ArrayType::kTypeFloat64);
  napi_array = ToNAPI(std::move(darray_value), env_);
  EXPECT_TRUE(napi_array.IsArray());
  EXPECT_TRUE(napi_array.As<Napi::Array>().Length() == darray.size());
  for (size_t i = 0; i < darray.size(); i++) {
    EXPECT_TRUE(
        napi_array.As<Napi::Array>().Get(i).As<Napi::Number>().DoubleValue() ==
        darray[i]);
  }

  std::vector<std::string> sarray = {"foo", "bar", ""};
  Value sarray_value = Value::Array(sarray);
  EXPECT_TRUE(sarray_value.GetType() == ValueType::kArray);
  EXPECT_TRUE(sarray_value.GetArrayType() == ArrayType::kTypeString);
  napi_array = ToNAPI(std::move(sarray_value), env_);
  EXPECT_TRUE(napi_array.IsArray());
  EXPECT_TRUE(napi_array.As<Napi::Array>().Length() == sarray.size());
  for (size_t i = 0; i < sarray.size(); i++) {
    EXPECT_TRUE(
        napi_array.As<Napi::Array>().Get(i).As<Napi::String>().Utf8Value() ==
        sarray[i]);
  }

  // TypedArrays.
  array_value = Value::Int32Array(array);
  EXPECT_TRUE(array_value.GetType() == ValueType::kTypedArray);
  EXPECT_TRUE(array_value.GetArrayType() == ArrayType::kTypeInt32);
  napi_array = ToNAPI(std::move(array_value), env_);
  EXPECT_TRUE(napi_array.IsTypedArray());
  EXPECT_TRUE(napi_array.IsInt32Array());
  EXPECT_TRUE(napi_array.As<Napi::TypedArray>().ElementLength() ==
              array.size());
  for (size_t i = 0; i < array.size(); i++) {
    EXPECT_TRUE(napi_array.As<Napi::TypedArray>()
                    .Get(i)
                    .As<Napi::Number>()
                    .Int32Value() == array[i]);
  }

  uarray_value = Value::Uint32Array(uarray);
  EXPECT_TRUE(uarray_value.GetType() == ValueType::kTypedArray);
  EXPECT_TRUE(uarray_value.GetArrayType() == ArrayType::kTypeUint32);
  napi_array = ToNAPI(std::move(uarray_value), env_);
  EXPECT_TRUE(napi_array.IsTypedArray());
  EXPECT_TRUE(napi_array.IsUint32Array());
  EXPECT_TRUE(napi_array.As<Napi::TypedArray>().ElementLength() ==
              uarray.size());
  for (size_t i = 0; i < uarray.size(); i++) {
    EXPECT_TRUE(napi_array.As<Napi::TypedArray>()
                    .Get(i)
                    .As<Napi::Number>()
                    .Uint32Value() == uarray[i]);
  }

  farray_value = Value::Float32Array(farray);
  EXPECT_TRUE(farray_value.GetType() == ValueType::kTypedArray);
  EXPECT_TRUE(farray_value.GetArrayType() == ArrayType::kTypeFloat32);
  napi_array = ToNAPI(std::move(farray_value), env_);
  EXPECT_TRUE(napi_array.IsTypedArray());
  EXPECT_TRUE(napi_array.IsFloat32Array());
  EXPECT_TRUE(napi_array.As<Napi::TypedArray>().ElementLength() ==
              farray.size());
  for (size_t i = 0; i < farray.size(); i++) {
    EXPECT_TRUE(napi_array.As<Napi::TypedArray>()
                    .Get(i)
                    .As<Napi::Number>()
                    .FloatValue() == farray[i]);
  }

  // ArrayBufferViews.
  std::vector<char> buffer;
  std::vector<int> i32_buffer = {1, -2, 3, -4, 5, -6, 7, -8, 9, -10};
  buffer.resize(i32_buffer.size() * sizeof(int32_t));
  std::memcpy(buffer.data(), i32_buffer.data(), buffer.size());
  auto i32_value = Value::ArrayBufferView(buffer, ArrayType::kTypeInt32);
  EXPECT_TRUE(i32_value.GetType() == ValueType::kArrayBufferView);
  EXPECT_TRUE(i32_value.GetArrayType() == ArrayType::kTypeInt32);
  napi_array = ToNAPI(std::move(i32_value), env_);
  EXPECT_TRUE(napi_array.IsTypedArray());
  EXPECT_TRUE(napi_array.IsInt32Array());
  EXPECT_TRUE(napi_array.As<Napi::Int32Array>().ByteLength() == buffer.size());
  EXPECT_TRUE(napi_array.As<Napi::Int32Array>().ElementLength() ==
              i32_buffer.size());
  for (size_t i = 0; i < i32_buffer.size(); i++) {
    EXPECT_TRUE(napi_array.As<Napi::TypedArray>()
                    .Get(i)
                    .As<Napi::Number>()
                    .Int32Value() == i32_buffer[i]);
  }

  std::vector<uint> u32_buffer = {1, 2, 2147483648};
  buffer.resize(u32_buffer.size() * sizeof(uint32_t));
  std::memcpy(buffer.data(), u32_buffer.data(), buffer.size());
  auto u32_value = Value::ArrayBufferView(buffer, ArrayType::kTypeUint32);
  EXPECT_TRUE(u32_value.GetType() == ValueType::kArrayBufferView);
  EXPECT_TRUE(u32_value.GetArrayType() == ArrayType::kTypeUint32);
  napi_array = ToNAPI(std::move(u32_value), env_);
  EXPECT_TRUE(napi_array.IsTypedArray());
  EXPECT_TRUE(napi_array.IsUint32Array());
  EXPECT_TRUE(napi_array.As<Napi::Uint32Array>().ByteLength() == buffer.size());
  EXPECT_TRUE(napi_array.As<Napi::Uint32Array>().ElementLength() ==
              u32_buffer.size());
  for (size_t i = 0; i < u32_buffer.size(); i++) {
    EXPECT_TRUE(napi_array.As<Napi::TypedArray>()
                    .Get(i)
                    .As<Napi::Number>()
                    .Uint32Value() == u32_buffer[i]);
  }

  std::vector<float> f32_buffer = {1.2, 3.4, 5.6, -7.89};
  buffer.resize(f32_buffer.size() * sizeof(float));
  std::memcpy(buffer.data(), f32_buffer.data(), buffer.size());
  auto f32_value = Value::ArrayBufferView(buffer, ArrayType::kTypeFloat32);
  EXPECT_TRUE(f32_value.GetType() == ValueType::kArrayBufferView);
  EXPECT_TRUE(f32_value.GetArrayType() == ArrayType::kTypeFloat32);
  napi_array = ToNAPI(std::move(f32_value), env_);
  EXPECT_TRUE(napi_array.IsTypedArray());
  EXPECT_TRUE(napi_array.IsFloat32Array());
  EXPECT_TRUE(napi_array.As<Napi::Float32Array>().ByteLength() ==
              buffer.size());
  EXPECT_TRUE(napi_array.As<Napi::Float32Array>().ElementLength() ==
              f32_buffer.size());
  for (size_t i = 0; i < f32_buffer.size(); i++) {
    EXPECT_TRUE(napi_array.As<Napi::TypedArray>()
                    .Get(i)
                    .As<Napi::Number>()
                    .FloatValue() == f32_buffer[i]);
  }

  // Objects.
  Napi::Object obj = Napi::Object::New(env_);
  obj["foo"] = Napi::Number::New(env_, 123);
  obj["bar"] = Napi::String::New(env_, "hello");
  obj["baz"] = Napi::Boolean::New(env_, true);
  auto obj_value = Value::Object(FromNAPI(obj));
  EXPECT_TRUE(obj_value.GetType() == ValueType::kObject);
  auto napi_obj = ToNAPI(std::move(obj_value), env_);
  EXPECT_TRUE(napi_obj.IsObject());
  EXPECT_TRUE(napi_obj == obj);
  EXPECT_EQ(
      napi_obj.As<Napi::Object>().Get("foo").As<Napi::Number>().Int32Value(),
      123);
  EXPECT_EQ(
      napi_obj.As<Napi::Object>().Get("bar").As<Napi::String>().Utf8Value(),
      "hello");
  EXPECT_TRUE(
      napi_obj.As<Napi::Object>().Get("baz").As<Napi::Boolean>().Value());
}

TEST_F(NapiBindingTest, ArrayBufferToNapiValueTest) {
  size_t buffer_size = 5;
  auto ab_value = Value::ArrayBuffer(buffer_size);
  EXPECT_TRUE(ab_value.GetType() == ValueType::kArrayBuffer);
  auto napi_array = ToNAPI(std::move(ab_value), env_);
  EXPECT_TRUE(napi_array.IsArrayBuffer());
  EXPECT_TRUE(napi_array.As<Napi::ArrayBuffer>().ByteLength() == buffer_size);
  int8_t* data =
      static_cast<int8_t*>(napi_array.As<Napi::ArrayBuffer>().Data());
  for (size_t i = 0; i < buffer_size; i++) {
    EXPECT_TRUE(data[i] == 0);
  }

  int8_t data2[] = {1, -2, 3, -4, 5};
  buffer_size = sizeof(data2);
  ab_value = Value::ArrayBuffer(buffer_size, data2);
  EXPECT_TRUE(ab_value.GetType() == ValueType::kArrayBuffer);
  napi_array = ToNAPI(std::move(ab_value), env_);
  EXPECT_TRUE(napi_array.IsArrayBuffer());
  EXPECT_TRUE(napi_array.As<Napi::ArrayBuffer>().ByteLength() == buffer_size);
  data = static_cast<int8_t*>(napi_array.As<Napi::ArrayBuffer>().Data());
  for (size_t i = 0; i < buffer_size; i++) {
    EXPECT_TRUE(data[i] == data2[i]);
  }

  static bool finalizer_works = false;
  static void* ab_data = static_cast<void*>(&ab_data);
  Value::Finalizer finalizer = [](void* data) {
    if (data == ab_data) {
      finalizer_works = true;
    }
  };
  ab_value = Value::ArrayBuffer(buffer_size, ab_data, finalizer);
  EXPECT_TRUE(ab_value.GetType() == ValueType::kArrayBuffer);
  napi_array = ToNAPI(std::move(ab_value), env_);
  EXPECT_TRUE(napi_array.IsArrayBuffer());
  EXPECT_TRUE(napi_array.As<Napi::ArrayBuffer>().ByteLength() == buffer_size);
  void* napi_data = napi_array.As<Napi::ArrayBuffer>().Data();
  EXPECT_TRUE(napi_data == ab_data);

  Dispose();
  EXPECT_TRUE(finalizer_works);
}

TEST_F(NapiBindingTest, ValueCopyMoveTest) {
  std::string s = "foo";
  Value value = Value::String(s);
  Value copied = value;
  EXPECT_EQ(copied.GetType(), value.GetType());
  EXPECT_EQ(*copied.Data<std::string>(), s);
  EXPECT_EQ(*copied.Data<std::string>(), *value.Data<std::string>());
  Value moved = std::move(value);
  EXPECT_EQ(moved.GetType(), value.GetType());
  EXPECT_EQ(*moved.Data<std::string>(), s);
  EXPECT_NE(*value.Data<std::string>(), s);

  std::vector<int> iarray = {1, 2, 3};
  value = Value::Array(iarray, ArrayType::kTypeInt32);
  copied = value;
  EXPECT_EQ(copied.GetType(), value.GetType());
  EXPECT_EQ(*copied.Data<std::vector<int32_t>>(), iarray);
  EXPECT_EQ(*copied.Data<std::vector<int32_t>>(),
            *value.Data<std::vector<int32_t>>());
  moved = std::move(value);
  EXPECT_EQ(moved.GetType(), value.GetType());
  EXPECT_EQ(*moved.Data<std::vector<int32_t>>(), iarray);
  EXPECT_NE(*moved.Data<std::vector<int32_t>>(),
            *value.Data<std::vector<int32_t>>());

  value = Value::Int32Array(iarray);
  copied = value;
  EXPECT_EQ(copied.GetType(), value.GetType());
  EXPECT_EQ(*copied.Data<std::vector<int32_t>>(), iarray);
  EXPECT_EQ(*copied.Data<std::vector<int32_t>>(), *value.Data<std::vector<int32_t>>());
  moved = std::move(value);
  EXPECT_EQ(moved.GetType(), value.GetType());
  EXPECT_EQ(*moved.Data<std::vector<int32_t>>(), iarray);
  EXPECT_NE(*moved.Data<std::vector<int32_t>>(), *value.Data<std::vector<int32_t>>());

  value = Value::ArrayBuffer(iarray.size() * sizeof(int32_t), iarray.data(),
                             nullptr);
  copied = value;
  EXPECT_EQ(copied.GetType(), value.GetType());
  EXPECT_EQ(copied.Data<Value::ArrayBufferData>()->size_, value.Data<Value::ArrayBufferData>()->size_);
  EXPECT_EQ(copied.Data<Value::ArrayBufferData>()->data_, value.Data<Value::ArrayBufferData>()->data_);
  moved = std::move(value);
  EXPECT_EQ(moved.GetType(), value.GetType());
  EXPECT_EQ(moved.Data<Value::ArrayBufferData>()->size_, copied.Data<Value::ArrayBufferData>()->size_);
  EXPECT_EQ(moved.Data<Value::ArrayBufferData>()->data_, copied.Data<Value::ArrayBufferData>()->data_);
  EXPECT_NE(moved.Data<Value::ArrayBufferData>()->size_, value.Data<Value::ArrayBufferData>()->size_);
  EXPECT_NE(moved.Data<Value::ArrayBufferData>()->data_, value.Data<Value::ArrayBufferData>()->data_);

  std::vector<char> buffer{'a', 'b', 'c', 'd', 'e'};
  value = Value::ArrayBufferView(buffer, ArrayType::kTypeInt32);
  copied = value;
  EXPECT_EQ(copied.GetType(), value.GetType());
  EXPECT_EQ(*copied.Data<std::vector<char>>(), *value.Data<std::vector<char>>());
  moved = std::move(value);
  EXPECT_EQ(moved.GetType(), value.GetType());
  EXPECT_EQ(*moved.Data<std::vector<char>>(), *copied.Data<std::vector<char>>());
  EXPECT_NE(*moved.Data<std::vector<char>>(), *value.Data<std::vector<char>>());

  Napi::Object obj = Napi::Object::New(env_);
  Object o = FromNAPI(obj);
  value = Value::Object(std::move(o));
  copied = value;
  EXPECT_EQ(copied.GetType(), value.GetType());
  EXPECT_EQ(ToNAPI(*copied.Data<Object>()), obj);
  moved = std::move(value);
  EXPECT_EQ(moved.GetType(), value.GetType());
  EXPECT_EQ(ToNAPI(*moved.Data<Object>()), obj);
  EXPECT_TRUE(ToNAPI(*value.Data<Object>()).IsEmpty());
}

}  // namespace binding
}  // namespace lynx
