// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_LYNX_RECORDER_TEST_BENCH_UTILS_H_
#define PLATFORM_EMBEDDER_LYNX_RECORDER_TEST_BENCH_UTILS_H_

#include <string>
#include <vector>

#include "third_party/rapidjson/document.h"
#include "third_party/rapidjson/error/en.h"
#include "third_party/rapidjson/reader.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"

namespace lynx {
namespace embedder {

const std::string TEST_BENCH_URL_PREFIX = "sslocal://arkview?";

std::string TestBenchDecode(const std::string& encoded);

std::vector<uint8_t> TestBenchDecompress(const std::vector<uint8_t>& data);

bool StringToInt(const std::string& input, int* output, uint8_t base = 10);

std::string ToJson(const rapidjson::Value& json);

}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_LYNX_RECORDER_TEST_BENCH_UTILS_H_
