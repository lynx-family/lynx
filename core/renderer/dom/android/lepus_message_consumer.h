// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_ANDROID_LEPUS_MESSAGE_CONSUMER_H_
#define CORE_RENDERER_DOM_ANDROID_LEPUS_MESSAGE_CONSUMER_H_

#include <string>
#include <vector>

#include "core/runtime/jsi/jsi.h"
#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace tasm {

class LepusDecoder {
 public:
  LepusDecoder();
  lepus_value DecodeMessage(char* buffer, uint32_t len);
  std::optional<lynx::piper::Value> DecodeJSMessage(piper::Runtime& rt,
                                                    char* buffer, uint32_t len);

 private:
  uint32_t index_;
  uint32_t len_;
  char* buffer_;
  bool in_exception_ = false;
  uint8_t forwardType();
  uint16_t forwardUInt16();
  int32_t forwardInteger();
  int64_t forwardLong();
  double_t forwardDouble();
  int32_t forwardSize();

  base::String forwardString();
  fml::RefPtr<lepus::ByteArray> forwardByteArray();
  fml::RefPtr<lepus::CArray> forwardArray();
  fml::RefPtr<lepus::Dictionary> forwardDictionary();
  lepus_value forwardValue();
  lepus_value forwardTemplateData();

  piper::String forwardJSString(piper::Runtime& rt);
  piper::ArrayBuffer forwardJSByteArray(piper::Runtime& rt);
  std::optional<piper::Array> forwardJSArray(piper::Runtime& rt);
  std::optional<piper::Object> forwardJSDictionary(piper::Runtime& rt);
  std::optional<piper::Value> forwardJSValue(piper::Runtime& rt);
};

class LepusEncoder {
 public:
  LepusEncoder() = default;
  std::vector<int8_t> EncodeMessage(const lepus::Value& value);

 private:
  void WriteValue(std::vector<int8_t>& vec, const lepus::Value& value);
  void WriteString(std::vector<int8_t>& vec, const base::String& string);
  void WriteSize(std::vector<int8_t>& vec, const size_t size);
  void WriteJSValue(std::vector<int8_t>& vec, const lepus::Value& value);
  void WriteJSArray(std::vector<int8_t>& vec, const lepus::Value& value);
  void WriteJSTable(std::vector<int8_t>& vec, const lepus::Value& value);
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_ANDROID_LEPUS_MESSAGE_CONSUMER_H_
