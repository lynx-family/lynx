// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/platform/android/jni_convert_helper.h"
#include "core/base/android/jni_helper.h"
#if ENABLE_TESTBENCH_REPLAY
#include "core/services/replay/fixture_evaluator.h"
#endif
#include "core/services/recorder/recorder_controller.h"
#include "core/services/recorder/testbench_base_recorder.h"
#include "platform/android/lynx_devtool/src/main/jni/gen/RecorderController_jni.h"
#include "platform/android/lynx_devtool/src/main/jni/gen/RecorderController_register_jni.h"
#include "third_party/rapidjson/document.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"

void StartRecord(JNIEnv* env, jclass jcaller) {
  lynx::tasm::recorder::RecorderController::StartRecord();
}

jbyteArray EvaluateFixture(JNIEnv* env, jclass, jstring directory) {
  rapidjson::Document output(rapidjson::kObjectType);
  auto& allocator = output.GetAllocator();
#if ENABLE_TESTBENCH_REPLAY
  auto result = lynx::tasm::replay::EvaluateFixture(
      lynx::base::android::JNIConvertHelper::ConvertToString(env, directory));
  if (result.has_value()) {
    rapidjson::Value actions(rapidjson::kArrayType);
    for (const auto& action : result->actions) {
      rapidjson::Value entry(rapidjson::kObjectType);
      entry.AddMember("functionName",
                      rapidjson::Value(action.function_name.c_str(), allocator),
                      allocator);
      entry.AddMember("delayMs", action.delay_ms, allocator);
      rapidjson::Document params;
      params.Parse(action.params_json.c_str());
      entry.AddMember("params", rapidjson::Value(params, allocator), allocator);
      actions.PushBack(entry, allocator);
    }
    output.AddMember("actions", actions, allocator);
    rapidjson::Value shared(rapidjson::kObjectType);
    for (const auto& item : result->shared_data) {
      rapidjson::Document value;
      value.Parse(item.value_json.c_str());
      shared.AddMember(
          rapidjson::Value(item.key.data(),
                           static_cast<rapidjson::SizeType>(item.key.size()),
                           allocator),
          rapidjson::Value(value, allocator), allocator);
    }
    output.AddMember("sharedData", shared, allocator);
  } else {
    output.AddMember("error",
                     rapidjson::Value(result.error().c_str(), allocator),
                     allocator);
  }
#else
  output.AddMember("error", "Fixture replay is disabled in this build",
                   allocator);
#endif
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  output.Accept(writer);
  // Return this local reference to Java; a scoped wrapper would delete it.
  // NOLINTNEXTLINE(lynx_custom/new_java_ref)
  auto bytes = env->NewByteArray(static_cast<jsize>(buffer.GetSize()));
  if (bytes)
    env->SetByteArrayRegion(bytes, 0, static_cast<jsize>(buffer.GetSize()),
                            reinterpret_cast<const jbyte*>(buffer.GetString()));
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
