// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <string>

#include "base/include/platform/android/jni_convert_helper.h"
#include "core/base/android/jni_helper.h"
#include "core/base/json/json_util.h"
#if ENABLE_TESTBENCH_REPLAY
#include "core/services/replay/fixture_evaluator.h"
#endif
#include "core/services/recorder/recorder_controller.h"
#include "core/services/recorder/testbench_base_recorder.h"
#include "platform/android/lynx_devtool/src/main/jni/gen/RecorderController_jni.h"
#include "platform/android/lynx_devtool/src/main/jni/gen/RecorderController_register_jni.h"

void StartRecord(JNIEnv* env, jclass jcaller) {
  lynx::tasm::recorder::RecorderController::StartRecord();
}

jbyteArray EvaluateFixture(JNIEnv* env, jclass, jstring directory) {
  std::string json;
#if ENABLE_TESTBENCH_REPLAY
  // Reuse the exported JSON serializer for string escaping. Evaluator payloads
  // are already valid JSON; append them unchanged to preserve business data.
  auto quote = [](const std::string& value) {
    return lynx::base::ToJson(rapidjson::Value(
        value.data(), static_cast<rapidjson::SizeType>(value.size())));
  };
  auto result = lynx::tasm::replay::EvaluateFixture(
      lynx::base::android::JNIConvertHelper::ConvertToString(env, directory));
  if (result.has_value()) {
    json = "{\"actions\":[";
    bool first = true;
    for (const auto& action : result->actions) {
      if (!first) json += ',';
      first = false;
      json += "{\"functionName\":";
      json += quote(action.function_name);
      json += ",\"delayMs\":";
      json += std::to_string(action.delay_ms);
      json += ",\"params\":";
      json += action.params_json;
      json += '}';
    }
    json += "],\"sharedData\":{";
    first = true;
    for (const auto& item : result->shared_data) {
      if (!first) json += ',';
      first = false;
      json += quote(item.key);
      json += ':';
      json += item.value_json;
    }
    json += "}}";
  } else {
    json = "{\"error\":";
    json += quote(result.error());
    json += '}';
  }
#else
  json = "{\"error\":\"Fixture replay is disabled in this build\"}";
#endif
  // Return this local reference to Java; a scoped wrapper would delete it.
  // NOLINTNEXTLINE(lynx_custom/new_java_ref)
  auto bytes = env->NewByteArray(static_cast<jsize>(json.size()));
  if (bytes)
    env->SetByteArrayRegion(bytes, 0, static_cast<jsize>(json.size()),
                            reinterpret_cast<const jbyte*>(json.data()));
  return bytes;
}

namespace lynx {
namespace devtool {
namespace jni {

bool RegisterJNIForRecorderController(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace jni
}  // namespace devtool
}  // namespace lynx
