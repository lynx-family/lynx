// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_RECORDER_NATIVE_MODULE_RECORDER_H_
#define CORE_SERVICES_RECORDER_NATIVE_MODULE_RECORDER_H_

#include <string>
#include <vector>

#include "core/runtime/jsi/jsi.h"
#include "core/services/recorder/recorder_constants.h"
#include "core/services/recorder/testbench_base_recorder.h"
#include "third_party/rapidjson/document.h"
namespace lynx {

namespace piper {
class Runtime;
class Value;
}  // namespace piper

namespace tasm {
namespace recorder {

class NativeModuleRecorder {
 public:
  static NativeModuleRecorder& GetInstance() {
    static base::NoDestructor<NativeModuleRecorder> instance_;
    return *instance_.get();
  }
  void RecordFunctionCall(const char* module_name, const char* js_method_name,
                          uint32_t argc, const piper::Value* args,
                          const int64_t* callbacks, uint32_t count,
                          piper::Value& res, piper::Runtime* rt,
                          int64_t record_id);

  void RecordCallback(const char* module_name, const char* method_name,
                      const piper::Value& args, piper::Runtime* rt,
                      int64_t callback_id, int64_t record_id);

  void RecordCallback(const char* module_name, const char* method_name,
                      const piper::Value* args, uint32_t count,
                      piper::Runtime* rt, int64_t callback_id,
                      int64_t record_id);

  void RecordGlobalEvent(std::string module_id, std::string method_id,
                         const piper::Value* args, uint64_t count,
                         piper::Runtime* rt, int64_t record_id);

  // event_type: 0 stands for touch event (or MotionEvent), and 1 for key event.
  void RecordEventAndroid(const std::vector<std::string>& args,
                          int32_t event_type, int64_t record_id,
                          TestBenchBaseRecorder* instance);

 private:
  friend base::NoDestructor<NativeModuleRecorder>;
  NativeModuleRecorder() = default;
  ~NativeModuleRecorder() = default;
  NativeModuleRecorder(const NativeModuleRecorder&) = delete;
  NativeModuleRecorder& operator=(const NativeModuleRecorder&) = delete;
  static rapidjson::Value ParsePiperValueToJsonValue(const piper::Value& res,
                                                     piper::Runtime* rt);
};

}  // namespace recorder
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_RECORDER_NATIVE_MODULE_RECORDER_H_
