// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_WORKLET_LEPUS_LYNX_H_
#define CORE_RENDERER_WORKLET_LEPUS_LYNX_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/include/thread/timed_task.h"
#include "core/renderer/worklet/lepus_element.h"
#include "core/renderer/worklet/lepus_raf_handler.h"
#include "third_party/binding/napi/napi_bridge.h"

namespace lynx {

namespace tasm {
class TemplateAssembler;
}  // namespace tasm

namespace worklet {

using binding::BridgeBase;
using binding::ImplBase;

class NapiFrameCallback;
class NapiFuncCallback;

class LepusLynx : public ImplBase {
 public:
  static LepusLynx* Create(Napi::Env env, const std::string& entry_name,
                           tasm::TemplateAssembler* assembler) {
    return new LepusLynx(env, entry_name, assembler);
  }
  LepusLynx(const LepusLynx&) = delete;
  virtual ~LepusLynx() = default;

  uint32_t SetTimeout(std::unique_ptr<NapiFuncCallback> callback,
                      int64_t delay);
  uint32_t SetInterval(std::unique_ptr<NapiFuncCallback> callback,
                       int64_t delay);
  void ClearTimeout(uint32_t task_id);
  void ClearInterval(uint32_t task_id);

  void TriggerLepusBridge(const std::string& method_name,
                          Napi::Object method_detail,
                          std::unique_ptr<NapiFuncCallback> callback);
  Napi::Value TriggerLepusBridgeSync(const std::string& method_name,
                                     Napi::Object method_detail);
  void InvokeLepusBridge(const int32_t callback_id, const lepus::Value& data);

 private:
  LepusLynx(Napi::Env env, const std::string& entry_name,
            tasm::TemplateAssembler* assembler);
  void EnsureTimeTaskInvoker();
  void RemoveTimedTask(uint32_t task_id);

  Napi::Env env_;
  std::string entry_name_;
  tasm::TemplateAssembler* tasm_;
  std::unique_ptr<LepusApiHandler> task_handler_;
  std::unique_ptr<base::TimedTaskManager> timer_;

  std::unordered_map<uint32_t, int64_t> task_to_callback_map_{};
};
}  // namespace worklet
}  // namespace lynx

#endif  // CORE_RENDERER_WORKLET_LEPUS_LYNX_H_
