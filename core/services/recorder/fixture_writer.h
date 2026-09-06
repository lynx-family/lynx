// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_RECORDER_FIXTURE_WRITER_H_
#define CORE_SERVICES_RECORDER_FIXTURE_WRITER_H_

#include <cstdint>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "third_party/rapidjson/document.h"

namespace lynx {
namespace tasm {
namespace recorder {

// Fixture (zip) view: recording-stream data organized incrementally in the
// Record* hooks, assembled and written directly at EndRecord time (no
// intermediate format).

struct FixtureCallback {
  int index = 0;           // index of the callback within the call's args
  std::string value_json;  // "null" if absent
  int64_t delay_ms = 0;    // delay relative to the invocation time
};

struct FixtureCall {
  std::string args_json;          // JSON of the "args" array
  std::string return_value_json;  // empty = no returnValue
  std::vector<FixtureCallback> callbacks;
  int64_t record_ms = 0;  // invocation time (epoch ms)
};

struct FixtureAction {
  std::string function_name;
  std::string params_json;
  int64_t record_ms = 0;  // invocation time (epoch ms)
};

struct FixtureData {
  // key = module + '\0' + method -> calls in chronological order
  std::unordered_map<std::string, std::vector<FixtureCall>> calls;
  std::vector<FixtureAction> actions;
  std::vector<std::pair<std::string, int>> components;  // name, type
  std::vector<std::pair<std::string, std::string>> shared_data;
  std::string config_json;  // replay config (filled in at EndRecord)

  // loadTemplate info (extracted from the action stream while recording)
  bool has_load_template = false;
  std::string load_template_url;
  std::string load_template_source_base64;  // template.bin base64
  std::string load_template_data_json;      // empty = none

  // callback_id -> (calls key, call index, callback index in args)
  std::unordered_map<int64_t, std::tuple<std::string, size_t, int>>
      callback_lookup;
};

// Hard cap on the total uncompressed content of one fixture zip (OOM
// guard). 128 MiB is ~10x headroom over the largest fixtures seen in practice.
constexpr size_t kMaxFixtureTotalBytes = 128ull * 1024 * 1024;

// Writes recorder{shell_id}.zip (fixture.js + config.json +
// component_list.json + assets/...). Returns true on success.
bool WriteFixtureZip(const std::string& zip_path, const FixtureData& data);

// Serializes a rapidjson::Value into a compact JSON string (used when
// recording the fixture view).
std::string JsonToCompactString(const rapidjson::Value& value);

}  // namespace recorder
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_RECORDER_FIXTURE_WRITER_H_
