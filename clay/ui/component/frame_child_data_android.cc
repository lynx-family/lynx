// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/frame_child_data_android.h"

#include <mutex>
#include <utility>

#include "clay/fml/logging.h"

namespace clay {
namespace {

std::mutex& ReleaserMutex() {
  static std::mutex mutex;
  return mutex;
}

FrameChildDataAndroid::NativeDataReleaser& RegisteredNativeDataReleaser() {
  static FrameChildDataAndroid::NativeDataReleaser releaser;
  return releaser;
}

Value RetainValue(const Value& value) {
  if (value.IsNull()) {
    return Value::Null();
  }
  switch (value.type()) {
    case Value::kBool:
      return Value(value.GetBool());
    case Value::kInt:
      return Value(value.GetInt());
    case Value::kUInt:
      return Value(value.GetUint());
    case Value::kFloat:
      return Value(value.GetFloat());
    case Value::kDouble:
      return Value(value.GetDouble());
    case Value::kString:
      return Value(value.GetString());
    case Value::kArray:
      return Value(value.value<std::shared_ptr<Value::Array>>());
    case Value::kArrayBuffer:
      return Value(value.value<std::shared_ptr<Value::ArrayBuffer>>());
    case Value::kMap:
      return Value(value.value<std::shared_ptr<Value::Map>>());
    default:
      return Value();
  }
}

}  // namespace

FrameChildDataAndroid::FrameChildDataAndroid(const Value& value) {
  if (value.IsLong()) {
    native_data_pointer_ = value.GetLong();
  } else {
    value_ = RetainValue(value);
  }
}

FrameChildDataAndroid::~FrameChildDataAndroid() {
  if (!native_data_pointer_) {
    return;
  }
  NativeDataReleaser releaser;
  {
    std::lock_guard<std::mutex> lock(ReleaserMutex());
    releaser = RegisteredNativeDataReleaser();
  }
  if (releaser) {
    releaser(native_data_pointer_);
  } else {
    FML_LOG(ERROR) << "Frame native data releaser is not installed";
  }
}

int64_t FrameChildDataAndroid::TakeNativeDataPointer() {
  const int64_t pointer = native_data_pointer_;
  native_data_pointer_ = 0;
  return pointer;
}

void FrameChildDataAndroid::SetNativeDataReleaser(
    NativeDataReleaser releaser) {
  std::lock_guard<std::mutex> lock(ReleaserMutex());
  RegisteredNativeDataReleaser() = std::move(releaser);
}

}  // namespace clay
