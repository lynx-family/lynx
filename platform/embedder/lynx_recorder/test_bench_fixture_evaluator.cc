// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/public/capi/lynx_replay_capi.h"

#if ENABLE_TESTBENCH_REPLAY
#include <limits>

#include "core/services/replay/fixture_evaluator.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"
#endif

LYNX_EXTERN_C bool lynx_replay_evaluate_fixture(
    const char* directory, void* context,
    void (*callback)(void*, const char*, size_t)) {
  if (!callback) return false;
#if ENABLE_TESTBENCH_REPLAY
  if (directory && directory[0]) {
    auto result = lynx::tasm::replay::EvaluateFixture(directory);
    if (!result.has_value()) {
      callback(context, result.error().data(), result.error().size());
      return false;
    }
    // Desktop schedulers convert milliseconds to OS timer units. Keep user
    // scripts within a signed 32-bit millisecond interval on both platforms.
    for (const auto& action : result->actions) {
      if (action.delay_ms > std::numeric_limits<int32_t>::max()) {
        constexpr char error[] = "Fixture action delay exceeds desktop limit";
        callback(context, error, sizeof(error) - 1);
        return false;
      }
    }
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    writer.StartObject();
    writer.Key("Action List");
    writer.StartArray();
    for (const auto& action : result->actions) {
      writer.StartObject();
      writer.Key("Function Name");
      writer.String(action.function_name.data(), action.function_name.size());
      writer.Key("RecordMillisecond");
      writer.Int64(action.delay_ms);
      writer.Key("Params");
      writer.RawValue(action.params_json.data(), action.params_json.size(),
                      rapidjson::kObjectType);
      writer.EndObject();
    }
    writer.EndArray();
    writer.EndObject();
    callback(context, buffer.GetString(), buffer.GetSize());
    return true;
  }
  constexpr char error[] = "Missing Fixture directory";
#else
  constexpr char error[] = "Fixture replay is disabled";
#endif
  callback(context, error, sizeof(error) - 1);
  return false;
}
