// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_JSI_JS_TASK_ADAPTER_H_
#define CORE_RUNTIME_BINDINGS_JSI_JS_TASK_ADAPTER_H_

#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>

#include "base/include/closure.h"
#include "base/include/thread/timed_task.h"
#include "core/runtime/jsi/jsi.h"

namespace lynx {
namespace piper {

// Ownered by js_app
class JsTaskAdapter : public std::enable_shared_from_this<JsTaskAdapter> {
 public:
  explicit JsTaskAdapter(const std::weak_ptr<Runtime>& rt,
                         const std::string& group_id);
  ~JsTaskAdapter();

  JsTaskAdapter(const JsTaskAdapter&) = delete;
  JsTaskAdapter& operator=(const JsTaskAdapter&) = delete;
  JsTaskAdapter(JsTaskAdapter&&) = default;
  JsTaskAdapter& operator=(JsTaskAdapter&&) = default;

  piper::Value SetTimeout(Function func, int32_t delay);

  piper::Value SetInterval(Function func, int32_t delay);

  void RemoveTask(uint32_t task);

  void QueueMicrotask(Function func);

 private:
  enum class TaskType {
    kSetTimeout,
    kSetInterval,
    kQueueMicrotask,
  };
  base::closure MakeTask(Function func, TaskType task_type);

  std::unique_ptr<base::TimedTaskManager> manager_;

  // bind to thread which JsTaskAdapter created.
  fml::RefPtr<fml::TaskRunner> runner_;

  std::weak_ptr<Runtime> rt_;

  std::string group_id_;
};

}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_JSI_JS_TASK_ADAPTER_H_
