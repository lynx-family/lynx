// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef THIRD_PARTY_BINDINGS_GEN_TEST_TEST_CONTEXT_H_
#define THIRD_PARTY_BINDINGS_GEN_TEST_TEST_CONTEXT_H_

#include <string>
#include "binding/gen_test/test_async_object.h"
#include "binding/napi/array_buffer_view.h"
#include "base/include/log/logging.h"
#include "third_party/binding/napi/napi_bridge.h"
#include "base/include/shared_vector.h"

namespace lynx {
namespace gen_test {

using binding::ArrayBufferView;
using base::SharedVector;

class TestContext : public binding::ImplBase {
 public:
  TestContext() = default;
  void SetClientOnFrameCallback(std::function<void()> on_frame) {}

  void VoidFromVoid() { Log(__FUNCTION__); }

  void VoidFromString(std::string s) { Log(__FUNCTION__, s); }
  std::string StringFromVoid() {
    Log(__FUNCTION__);
    return return_string_;
  }

  // Binding.
  void VoidFromStringArray(std::vector<Napi::String> sa) {
    Log(__FUNCTION__, sa);
  }
  // Command buffer.
  void VoidFromStringArray(std::vector<std::string> sa) {
    Log(__FUNCTION__, sa);
  }

  // Binding.
  void VoidFromTypedArray(Napi::Float32Array nfa) {
    SharedVector<float> fa(nfa.Data(), nfa.ElementLength());
    Log("VoidFromTypedArray_Binding", fa);
  }
  // Command buffer.
  void VoidFromTypedArray(SharedVector<float> fa) {
    Log("VoidFromTypedArray_CommandBuffer", fa);
  }

  // Binding.
  void VoidFromArrayBuffer(Napi::ArrayBuffer nab) {
    SharedVector<uint8_t> ca((uint8_t*)nab.Data(), nab.ByteLength());
    Log("VoidFromArrayBuffer_Binding", nab.ByteLength(), ca);
  }
  // Command buffer.
  void VoidFromArrayBuffer(void* buffer, uint32_t byte_length) {
    SharedVector<uint8_t> ca((uint8_t*)buffer, byte_length);
    Log("VoidFromArrayBuffer_CommandBuffer", byte_length, ca);
  }

  void VoidFromArrayBufferView(ArrayBufferView abv) {
    Log(__FUNCTION__, abv);
  }

  void VoidFromNullableArrayBufferView(ArrayBufferView abv) {
    Log(__FUNCTION__, abv);
  }

  static TestAsyncObject* async_object;

  TestAsyncObject* CreateAsyncObject() {
    auto* obj = new TestAsyncObject();
    async_object = obj;
    Log(__FUNCTION__, obj);
    return obj;
  }

  void AsyncForAsyncObject(TestAsyncObject* tao) {
    Log(__FUNCTION__, tao);
  }

  void AsyncForNullableAsyncObject(TestAsyncObject* tao) {
    Log(__FUNCTION__, tao);
  }

  std::string SyncForAsyncObject(TestAsyncObject* tao) {
    Log(__FUNCTION__, tao);
    return return_string_;
  }

  void Finish() { Log(__FUNCTION__); }

  static std::vector<std::string> RetrieveLog() {
    std::vector<std::string> temp;
    temp.swap(log_);
    return temp;
  }
  static void SetReturnString(std::string s) {
    return_string_ = std::move(s);
  }
 private:
  template <typename... Args>
  static void Log(const std::string& call, Args&&... args) {
    std::stringstream ss;
    ss << call << "(";
    LogInternal(ss, args...);
    ss << ")";
    log_.push_back(ss.str());
  }

  static void LogInternal(std::stringstream& ss) {}

  template <typename T>
  static void LogInternal(std::stringstream& ss, T&& first) {
    LogArgument(ss, first);
  }

  template <typename T, typename... Args>
  static void LogInternal(std::stringstream& ss, T&& first, Args&&... args) {
    LogArgument(ss, first);
    ss << ", ";
    LogInternal(ss, args...);
  }

  template <typename T>
  static void LogArgument(std::stringstream& ss, T arg) {
    ss << arg;
  }

  template <typename T>
  static void LogArgument(std::stringstream& ss, T* arg) {
    // nullptr is printed as 0x0 on linux but nil on darwin, which is annoying.
    ss << (uintptr_t) arg;
  }

  static void LogArgument(std::stringstream& ss, uint8_t arg);
  static void LogArgument(std::stringstream& ss, const std::string& arg);
  static void LogArgument(std::stringstream& ss, ArrayBufferView abv);

  template <typename T>
  static void LogArgument(std::stringstream& ss, const std::vector<T>& arg) {
    ss << "[";
    for (size_t i = 0; i < arg.size(); ++i) {
      LogArgument(ss, arg[i]);
      if (i != arg.size() - 1) {
        ss << ", ";
      }
    }
    ss << "]";
  }

  template <typename T>
  static void LogArgument(std::stringstream& ss, const SharedVector<T>& arg) {
    ss << "[";
    for (size_t i = 0; i < arg.Size(); ++i) {
      LogArgument(ss, arg[i]);
      if (i != arg.Size() - 1) {
        ss << ", ";
      }
    }
    ss << "]";
  }

  static std::vector<std::string> log_;
  static std::string return_string_;
};

}  // namespace gen_test
}  // namespace lynx

#endif  // THIRD_PARTY_BINDINGS_GEN_TEST_TEST_CONTEXT_H_
