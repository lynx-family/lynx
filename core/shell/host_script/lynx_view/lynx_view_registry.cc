// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/host_script/lynx_view/lynx_view_registry.h"

#include <algorithm>
#include <utility>

namespace lynx {
namespace shell {

LynxViewRegistry& LynxViewRegistry::GetInstance() {
  // Intentionally leaked: platform threads may report Views during exit.
  static auto* registry = new LynxViewRegistry();
  return *registry;
}

void LynxViewRegistry::SetUIRunner(fml::RefPtr<fml::TaskRunner> runner) {
  std::lock_guard<std::mutex> lock(mutex_);
  ui_runner_ = std::move(runner);
}

bool LynxViewRegistry::Register(base::LynxEntityId view,
                                std::shared_ptr<LynxViewRefProxy> proxy,
                                std::string url) {
  if (view < 0 || !proxy) return false;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!views_.emplace(view, Record{std::move(proxy), std::move(url)}).second)
      return false;
  }
  // Inserted before broadcasting, so a concurrent List() either includes the
  // View or precedes this notification; subscribers deduplicate the overlap.
  Broadcast({LynxViewEvent::Type::kCreated, view});
  return true;
}

void LynxViewRegistry::Unregister(base::LynxEntityId view) {
  std::shared_ptr<LynxViewRefProxy> proxy;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    auto found = views_.find(view);
    if (found == views_.end()) return;
    proxy = std::move(found->second.proxy);
    views_.erase(found);
  }
  proxy->Invalidate();
  Broadcast({LynxViewEvent::Type::kDestroyed, view});
}

void LynxViewRegistry::UpdateUrl(base::LynxEntityId view, std::string url) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (auto found = views_.find(view); found != views_.end())
    found->second.url = std::move(url);
}

void LynxViewRegistry::NotifyLifecycle(base::LynxEntityId view,
                                       std::string name, int32_t code,
                                       std::string message) {
  if (!IsRegistered(view)) return;
  Broadcast({LynxViewEvent::Type::kLifecycle, view, std::move(name), code,
             std::move(message)});
}

bool LynxViewRegistry::IsRegistered(base::LynxEntityId view) const {
  std::lock_guard<std::mutex> lock(mutex_);
  return views_.count(view) != 0;
}

std::string LynxViewRegistry::Url(base::LynxEntityId view) const {
  std::lock_guard<std::mutex> lock(mutex_);
  auto found = views_.find(view);
  return found == views_.end() ? std::string() : found->second.url;
}

std::vector<LynxViewRegistry::Entry> LynxViewRegistry::List() const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<Entry> entries;
  entries.reserve(views_.size());
  for (const auto& [view, record] : views_)
    entries.push_back({view, record.url});
  // Replay in creation order; engine identities increase monotonically.
  std::sort(entries.begin(), entries.end(),
            [](const Entry& a, const Entry& b) { return a.view < b.view; });
  return entries;
}

bool LynxViewRegistry::Post(base::LynxEntityId view, Command command,
                            Guard guard) {
  fml::RefPtr<fml::TaskRunner> runner;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ui_runner_ || !views_.count(view)) return false;
    runner = ui_runner_;
  }
  runner->PostTask(
      [this, view, command = std::move(command), guard = std::move(guard)] {
        if (guard && !guard()) return;
        std::shared_ptr<LynxViewRefProxy> proxy;
        {
          std::lock_guard<std::mutex> lock(mutex_);
          auto found = views_.find(view);
          if (found == views_.end()) return;
          proxy = found->second.proxy;
        }
        command(*proxy);
      });
  return true;
}

void LynxViewRegistry::AddObserver(const std::shared_ptr<Observer>& observer) {
  std::lock_guard<std::mutex> lock(mutex_);
  observers_.push_back(observer);
}

void LynxViewRegistry::RemoveObserver(const Observer* observer) {
  std::lock_guard<std::mutex> lock(mutex_);
  observers_.erase(std::remove_if(observers_.begin(), observers_.end(),
                                  [observer](const auto& weak) {
                                    auto locked = weak.lock();
                                    return !locked || locked.get() == observer;
                                  }),
                   observers_.end());
}

void LynxViewRegistry::Broadcast(const LynxViewEvent& event) {
  std::vector<std::shared_ptr<Observer>> observers;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& weak : observers_)
      if (auto observer = weak.lock()) observers.push_back(std::move(observer));
  }
  for (const auto& observer : observers) observer->OnViewEvent(event);
}

}  // namespace shell
}  // namespace lynx
