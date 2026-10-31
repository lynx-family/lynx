// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/host_script/runtime/interceptor.h"

#include <array>
#include <atomic>
#include <mutex>
#include <unordered_map>

#include "base/include/log/logging.h"
#include "base/include/no_destructor.h"

namespace lynx::shell {
namespace {
thread_local std::array<std::weak_ptr<Interceptor>, kHostScriptDomainCount>
    current;
constexpr size_t kKinds = static_cast<size_t>(InterceptKind::kCount);
struct State {
  std::array<std::array<std::atomic<size_t>, kKinds>, kHostScriptDomainCount>
      handlers{};
  std::atomic<size_t> providers{0};
  std::atomic<uint64_t> failures{0};
  std::mutex mutex;
  std::unordered_map<base::LynxEntityId, size_t> views;
};
State& GetState() {
  static base::NoDestructor<State> state;
  return *state;
}
std::atomic<size_t>& HandlerCount(InterceptKind kind,
                                  Interceptor::Domain domain) {
  return GetState()
      .handlers[HostScriptDomainIndex(domain)][static_cast<size_t>(kind)];
}
}  // namespace

const char* Interceptor::Name(InterceptKind kind) {
  static constexpr const char* names[] = {
      "view.created", "view.loadTemplate", "view.updateMetaData", "jsb.call",
      "jsb.result",   "jsb.callback",      "view.error"};
  return names[static_cast<size_t>(kind)];
}

bool Interceptor::Attach(const std::shared_ptr<Interceptor>& provider,
                         Domain domain) {
  auto& slot = current[HostScriptDomainIndex(domain)];
  if (!provider || provider->attached_ || !slot.expired()) return false;
  slot = provider;
  provider->attached_ = true;
  ++GetState().providers;
  return true;
}

void Interceptor::Detach(const Interceptor* provider) {
  for (auto& slot : current) {
    if (auto existing = slot.lock(); existing.get() == provider) slot.reset();
  }
  if (provider && provider->attached_) {
    const_cast<Interceptor*>(provider)->attached_ = false;
    --GetState().providers;
  }
}

std::shared_ptr<Interceptor> Interceptor::Current(
    std::initializer_list<InterceptKind> kinds, Domain domain) {
  if (auto provider = current[HostScriptDomainIndex(domain)].lock()) {
    if (!provider->IsAttached()) return nullptr;
    for (auto kind : kinds)
      if (provider->HasHandlers(kind)) return provider;
    return nullptr;
  }
  auto& handlers = GetState().handlers[HostScriptDomainIndex(domain)];
  for (auto kind : kinds) {
    if (!handlers[static_cast<size_t>(kind)].load()) continue;
    ReportCoverageGap(
        kind, "No interceptor environment on the calling thread/domain");
    break;
  }
  return nullptr;
}

void Interceptor::HandlerAdded(InterceptKind kind, Domain domain) {
  ++HandlerCount(kind, domain);
}
void Interceptor::HandlerRemoved(InterceptKind kind, Domain domain) {
  --HandlerCount(kind, domain);
}
bool Interceptor::IsEnabled() { return GetState().providers.load() != 0; }
uint64_t Interceptor::FailureCount() { return GetState().failures.load(); }
void Interceptor::ReportCoverageGap(InterceptKind kind, const char* reason) {
  ++GetState().failures;
  LOGE("Interceptor " << Name(kind) << ": " << reason);
  // Reporting must not enter a different execution domain.
}

void Interceptor::RegisterView(base::LynxEntityId view) {
  if (view < 0) return;
  auto& state = GetState();
  std::lock_guard<std::mutex> lock(state.mutex);
  ++state.views[view];
}
void Interceptor::DestroyView(base::LynxEntityId view) {
  auto& state = GetState();
  std::lock_guard<std::mutex> lock(state.mutex);
  auto entry = state.views.find(view);
  if (entry != state.views.end() && --entry->second == 0)
    state.views.erase(entry);
}
bool Interceptor::IsViewAlive(base::LynxEntityId view) {
  auto& state = GetState();
  std::lock_guard<std::mutex> lock(state.mutex);
  return state.views.count(view) != 0;
}
}  // namespace lynx::shell
