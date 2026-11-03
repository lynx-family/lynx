// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_HOST_SCRIPT_LYNX_VIEW_LYNX_VIEW_REGISTRY_H_
#define CORE_SHELL_HOST_SCRIPT_LYNX_VIEW_LYNX_VIEW_REGISTRY_H_

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/include/fml/task_runner.h"
#include "base/include/log/log_context.h"
#include "core/shell/host_script/lynx_view/lynx_view_ref_proxy.h"

namespace lynx {
namespace shell {

struct LynxViewEvent {
  enum class Type { kCreated, kLifecycle, kDestroyed };
  Type type;
  base::LynxEntityId view;
  // Lifecycle name, such as loadSuccess or error.
  std::string name;
  int32_t code = 0;
  std::string message;
};

// Process-wide record of operable platform Views, keyed by the engine View
// identity. It outlives every Host Script context and never owns a View: the
// platform proxy keeps a weak reference and the platform unregisters it on
// destruction. Render-only Views are never registered here.
class LynxViewRegistry {
 public:
  struct Entry {
    base::LynxEntityId view;
    std::string url;
  };

  // Receives events on the reporting thread. Implementations post them to
  // their own owner and never run script inline.
  class Observer {
   public:
    virtual ~Observer() = default;
    virtual void OnViewEvent(const LynxViewEvent& event) = 0;
  };

  // A command runs on the UI runner, against a View that is still registered.
  using Command = std::function<void(LynxViewRefProxy&)>;
  using Guard = std::function<bool()>;

  static LynxViewRegistry& GetInstance();

  // The platform UI runner. Commands are always posted there, never inlined.
  void SetUIRunner(fml::RefPtr<fml::TaskRunner> runner);

  // Platform UI thread. A View registers once; a repeated id is ignored.
  bool Register(base::LynxEntityId view,
                std::shared_ptr<LynxViewRefProxy> proxy, std::string url);
  // Invalidates the proxy immediately, then reports destruction once.
  void Unregister(base::LynxEntityId view);
  // The most recent URL committed by a load.
  void UpdateUrl(base::LynxEntityId view, std::string url);
  void NotifyLifecycle(base::LynxEntityId view, std::string name,
                       int32_t code = 0, std::string message = {});

  bool IsRegistered(base::LynxEntityId view) const;
  std::string Url(base::LynxEntityId view) const;
  std::vector<Entry> List() const;

  // Returns whether the command was accepted. On the UI runner it is skipped
  // when the guard fails or the View was unregistered meanwhile.
  bool Post(base::LynxEntityId view, Command command, Guard guard);

  void AddObserver(const std::shared_ptr<Observer>& observer);
  void RemoveObserver(const Observer* observer);

 private:
  struct Record {
    std::shared_ptr<LynxViewRefProxy> proxy;
    std::string url;
  };
  LynxViewRegistry() = default;
  void Broadcast(const LynxViewEvent& event);

  mutable std::mutex mutex_;
  fml::RefPtr<fml::TaskRunner> ui_runner_;
  std::unordered_map<base::LynxEntityId, Record> views_;
  std::vector<std::weak_ptr<Observer>> observers_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_HOST_SCRIPT_LYNX_VIEW_LYNX_VIEW_REGISTRY_H_
