// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_FRAME_CHILD_DATA_ANDROID_H_
#define CLAY_UI_COMPONENT_FRAME_CHILD_DATA_ANDROID_H_

#include <cstdint>
#include <functional>
#include <memory>

#include "clay/public/value.h"

namespace clay {

// Keeps frame metadata independent from liblynx C++ symbols. A long value is
// an owned lepus::Value transferred by FrameElement; other values are retained
// as Clay data until the Android runtime converts them to TemplateData.
class FrameChildDataAndroid {
 public:
  using NativeDataReleaser = std::function<void(int64_t)>;

  explicit FrameChildDataAndroid(const Value& value);
  ~FrameChildDataAndroid();

  FrameChildDataAndroid(const FrameChildDataAndroid&) = delete;
  FrameChildDataAndroid& operator=(const FrameChildDataAndroid&) = delete;

  int64_t TakeNativeDataPointer();
  bool has_native_data_pointer() const { return native_data_pointer_ != 0; }
  const Value& value() const { return value_; }

  static void SetNativeDataReleaser(NativeDataReleaser releaser);

 private:
  int64_t native_data_pointer_ = 0;
  Value value_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_FRAME_CHILD_DATA_ANDROID_H_
