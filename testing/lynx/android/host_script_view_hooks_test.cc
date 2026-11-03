// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <jni.h>

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "base/include/platform/android/jni_convert_helper.h"
#include "core/shell/host_script/lynx_view/lynx_view_registry.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"

namespace {
class ViewEventRecorder : public lynx::shell::LynxViewRegistry::Observer {
 public:
  explicit ViewEventRecorder(lynx::base::LynxEntityId view) : view_(view) {}

  void OnViewEvent(const lynx::shell::LynxViewEvent& event) override {
    if (event.view != view_ ||
        event.type != lynx::shell::LynxViewEvent::Type::kLifecycle) {
      return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    events_.push_back(event);
  }

  std::string ToJson() {
    std::lock_guard<std::mutex> lock(mutex_);
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    writer.StartArray();
    for (const auto& event : events_) {
      writer.StartObject();
      writer.Key("viewId");
      writer.Int64(event.view);
      writer.Key("name");
      writer.String(event.name.c_str(),
                    static_cast<rapidjson::SizeType>(event.name.size()));
      writer.Key("code");
      writer.Int(event.code);
      writer.Key("message");
      writer.String(event.message.c_str(),
                    static_cast<rapidjson::SizeType>(event.message.size()));
      writer.EndObject();
    }
    writer.EndArray();
    return {buffer.GetString(), buffer.GetSize()};
  }

 private:
  const lynx::base::LynxEntityId view_;
  std::mutex mutex_;
  std::vector<lynx::shell::LynxViewEvent> events_;
};
}  // namespace

extern "C" JNIEXPORT jboolean JNICALL
Java_com_lynx_tasm_LynxHostScriptViewHooksTest_nativeIsViewRegistered(
    JNIEnv*, jclass, jlong view_id) {
  return lynx::shell::LynxViewRegistry::GetInstance().IsRegistered(view_id);
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_lynx_tasm_LynxHostScriptViewCommandsTest_nativeCaptureEvents(
    JNIEnv* env, jclass, jlong view_id, jobject command) {
  auto recorder = std::make_shared<ViewEventRecorder>(view_id);
  auto& registry = lynx::shell::LynxViewRegistry::GetInstance();
  registry.AddObserver(recorder);
  jclass command_class = env->GetObjectClass(command);
  jmethodID run = env->GetMethodID(command_class, "run", "()V");
  env->CallVoidMethod(command, run);
  registry.RemoveObserver(recorder.get());
  env->DeleteLocalRef(command_class);
  if (env->ExceptionCheck()) {
    return nullptr;
  }
  // Return UTF-8 bytes so error messages containing non-BMP text stay intact.
  auto result = lynx::base::android::JNIConvertHelper::ConvertToJNIByteArray(
      env, recorder->ToJson());
  return static_cast<jbyteArray>(env->NewLocalRef(result.Get()));  // NOLINT
}
