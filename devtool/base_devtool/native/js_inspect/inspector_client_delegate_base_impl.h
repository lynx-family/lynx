// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_BASE_DEVTOOL_NATIVE_JS_INSPECT_INSPECTOR_CLIENT_DELEGATE_BASE_IMPL_H_
#define DEVTOOL_BASE_DEVTOOL_NATIVE_JS_INSPECT_INSPECTOR_CLIENT_DELEGATE_BASE_IMPL_H_

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>
#include <utility>

#include "base/include/base_export.h"
#include "base/include/thread/timed_task.h"
#include "devtool/base_devtool/native/js_inspect/script_manager_ng.h"
#include "devtool/fundamentals/js_inspect/inspector_client_delegate.h"
#include "devtool/js_inspect/inspector_const.h"

namespace lynx {
namespace devtool {

// Public implementations of DevTool.
class BASE_EXPORT InspectorClientDelegateBaseImpl
    : public InspectorClientDelegate,
      public std::enable_shared_from_this<InspectorClientDelegateBaseImpl> {
 public:
  InspectorClientDelegateBaseImpl(const std::string& vm_type);
  ~InspectorClientDelegateBaseImpl() override = default;

  // Dispatch messages to the JS engine. It can be called from any thread except
  // the JS thread.
  void DispatchMessageAsync(const std::string& message, int instance_id);

  /*
   * You need to override this function, we only provide a template here.
   *
   * auto mes = PrepareResponseMessage(message, instance_id);
   * // Send mes to the frontend...
   *
   */
  void SendResponse(const std::string& message, int instance_id) override = 0;

  void RunMessageLoopOnPause(const std::string& group_id) override;
  void QuitMessageLoopOnPause() override;

  double CurrentTimeMS() override;
  // The following two functions are implemented when using
  // lynx::fml::MessageLoop, if you use other message loop implementation, you
  // can override them again.
  void StartRepeatingTimer(double interval, std::function<void(void*)> callback,
                           void* data) override;
  void CancelTimer(void* data) override;

  // Stop debugging breakpoints. Must be called on non-JS thread.
  void StopDebug(int instance_id);

 protected:
  // Post task to the JS thread.
  virtual void PostTask(int instance_id, std::function<void()>&& closure) = 0;

  void DispatchMessageAsyncWithLockHeld(const std::string& message,
                                        int instance_id);
  void FlushMessageQueue();              // Must be called on JS thread.
  void FlushMessageQueueWithLockHeld();  // Must be called on JS thread.

  // Dispatch enable and cached breakpoints messages.
  // You can call this function if you want to initialize before receiving
  // frontend messages or after reloading.
  // Must be called on the JS thread and before loading JS files.
  // The last parameter can be set to true only after receiving
  // Page.getResourceTree from the frontend, since the frontend can process
  // Runtime.consoleAPICalled messages only after receiving the response of
  // Page.getResourceTree. If we send Runtime.enable to the JS engine too early,
  // Runtime.consoleAPICalled messages will be ignored by the frontend.
  void DispatchInitMessage(
      int instance_id, const std::unique_ptr<ScriptManagerNG>& script_manager,
      bool runtime_enable = false);
  void SetBreakpointCached(
      int instance_id, const std::unique_ptr<ScriptManagerNG>& script_manager);

  // The following two functions are used to pre-process CDP messages. If you
  // need to do something before sending the message, you can override them.
  // Otherwise do not override and they will return the original message.
  virtual std::string PrepareDispatchMessage(rapidjson::Document& message,
                                             int instance_id);
  virtual std::string PrepareResponseMessage(const std::string& message,
                                             int instance_id);

  // The following two functions can help cache breakpoint information. The
  // complete information needs to be obtained from both the request and
  // response message, so if you need this capability, call them in
  // PrepareDispatchMessage and PrepareResponseMessage.
  void CacheBreakpointsByRequestMessage(
      const rapidjson::Document& message,
      const std::unique_ptr<ScriptManagerNG>& script_manager);
  void CacheBreakpointsByResponseMessage(
      const rapidjson::Document& message,
      const std::unique_ptr<ScriptManagerNG>& script_manager);

  void RecordDebuggingInstanceID(const rapidjson::Document& message,
                                 int instance_id);

  // Add "engineType" parameter to the response of "Debugger.enable" message.
  void AddEngineTypeParam(rapidjson::Document& message);

  // Generate a simple CDP message which only has "method" and "id".
  std::string GenSimpleMessage(const std::string& method, int message_id = 0);
  std::string GenMessageSetBreakpointByUrl(const std::string& url,
                                           const std::string& condition,
                                           int line, int column,
                                           int message_id = 0);
  std::string GenMessageSetBreakpointsActive(bool active, int message_id = 0);
  rapidjson::Document GenTargetInfo(const std::string& target_id,
                                    const std::string& title);
  std::string GenMessageTargetCreated(const std::string& target_id,
                                      const std::string& title);
  std::string GenMessageAttachedToTarget(const std::string& target_id,
                                         const std::string& session_id,
                                         const std::string& title);
  std::string GenMessageTargetDestroyed(const std::string& target_id);
  std::string GenMessageDetachedFromTarget(const std::string& session_id);

  bool ParseStrToJson(rapidjson::Document& json_mes, const std::string& mes);
  void RemoveInvalidMembers(rapidjson::Value& message);

  bool paused_{false};
  std::queue<std::pair<int, std::string>> message_queue_;

  std::mutex mutex_;
  std::condition_variable cv_;

  std::unique_ptr<base::TimedTaskManager> timer_;
  std::unordered_map<void*, uint32_t> timed_task_ids_;

  std::string vm_type_;
  std::atomic_int debugging_instance_id_{kErrorViewID};
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_BASE_DEVTOOL_NATIVE_JS_INSPECT_INSPECTOR_CLIENT_DELEGATE_BASE_IMPL_H_
