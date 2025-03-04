// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "test_context.h"

namespace lynx {
namespace gen_test {

// static
std::vector<std::string> TestContext::log_;
std::string TestContext::return_string_;
TestAsyncObject* TestContext::async_object = nullptr;

// static
void TestContext::LogArgument(std::stringstream& ss, uint8_t arg) {
  ss << (uint32_t)arg;
}

// static
void TestContext::LogArgument(std::stringstream& ss, const std::string& arg) {
  ss << "'" << arg << "'";
}

// static
void TestContext::LogArgument(std::stringstream& ss, ArrayBufferView abv) {
  auto type = abv.GetType();
  uint32_t len = 0;
  if (abv.GetTypeSize()) {
    len = abv.ByteLength() / abv.GetTypeSize();
  } else if (type == binding::ArrayBufferView::kTypeDataView) {
    len = abv.ByteLength();
  }
  ss << type << "|" << len << ", [";
  void* data = abv.Data();
  for (size_t i = 0; i < len; ++i) {
    if (i > 0 && type != binding::ArrayBufferView::kTypeEmpty) {
      ss << ", ";
    }
    switch (type) {
      case binding::ArrayBufferView::kTypeInt8: {
        int8_t* d = (int8_t*)data;
        ss << (long long)d[i];
        break;
      }
      case binding::ArrayBufferView::kTypeUint8:
      case binding::ArrayBufferView::kTypeUint8Clamped: {
      case binding::ArrayBufferView::kTypeDataView:
        uint8_t* d = (uint8_t*)data;
        ss << (long long)d[i];
        break;
      }
      case binding::ArrayBufferView::kTypeInt16: {
        int16_t* d = (int16_t*)data;
        ss << (long long)d[i];
        break;
      }
      case binding::ArrayBufferView::kTypeUint16: {
        uint16_t* d = (uint16_t*)data;
        ss << (long long)d[i];
        break;
      }
      case binding::ArrayBufferView::kTypeInt32: {
        int32_t* d = (int32_t*)data;
        ss << (long long)d[i];
        break;
      }
      case binding::ArrayBufferView::kTypeUint32: {
        uint32_t* d = (uint32_t*)data;
        ss << (long long)d[i];
        break;
      }
      case binding::ArrayBufferView::kTypeFloat32: {
        float* d = (float*)data;
        ss << (double)d[i];
        break;
      }
      case binding::ArrayBufferView::kTypeFloat64: {
        double* d = (double*)data;
        ss << (double)d[i];
        break;
      }
      default:
        break;
    }
  }
  ss << "]";
}

}  // namespace gen_test
}  // namespace lynx
