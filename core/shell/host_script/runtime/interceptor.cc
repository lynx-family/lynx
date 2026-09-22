// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/host_script/runtime/interceptor.h"

#include <array>
#include <atomic>
#include <mutex>
#include <unordered_set>

#include "base/include/log/logging.h"
#include "base/include/no_destructor.h"

namespace lynx::shell {
namespace {
thread_local std::array<std::weak_ptr<Interceptor>, 3> current;
constexpr size_t kKinds = static_cast<size_t>(InterceptKind::kCount);
struct State {
  std::array<std::array<std::atomic<size_t>, kKinds>, 3> handlers{};
  std::atomic<size_t> providers{0};
  std::atomic<uint64_t> failures{0};
  std::mutex mutex;
  std::unordered_set<base::LynxEntityId> views;
};
State& GetState() {
  static base::NoDestructor<State> state;
  return *state;
}
}  // namespace

const char* Interceptor::Name(InterceptKind kind) {
  static constexpr const char* names[] = {
      "view.create", "view.created", "view.loadTemplate", "view.updateMetaData",
      "jsb.call",    "jsb.result",   "jsb.callback"};
  return names[static_cast<size_t>(kind)];
}

bool Interceptor::Attach(const std::shared_ptr<Interceptor>& provider,
                         Domain domain) {
  auto& slot = current[static_cast<size_t>(domain)];
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

std::shared_ptr<Interceptor> Interceptor::Current(InterceptKind kind,
                                                  Domain domain) {
  if (auto provider = current[static_cast<size_t>(domain)].lock()) {
    return provider->IsAttached() && provider->HasHandlers(kind) ? provider
                                                                 : nullptr;
  }
  if (GetState()
          .handlers[static_cast<size_t>(domain)][static_cast<size_t>(kind)]
          .load()) {
    ReportCoverageGap(
        kind, "No interceptor environment on the calling thread/domain");
  }
  return nullptr;
}

void Interceptor::HandlerAdded(InterceptKind kind, Domain domain) {
  ++GetState().handlers[static_cast<size_t>(domain)][static_cast<size_t>(kind)];
}
void Interceptor::HandlerRemoved(InterceptKind kind, Domain domain) {
  --GetState().handlers[static_cast<size_t>(domain)][static_cast<size_t>(kind)];
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
  state.views.insert(view);
}
void Interceptor::DestroyView(base::LynxEntityId view) {
  auto& state = GetState();
  std::lock_guard<std::mutex> lock(state.mutex);
  state.views.erase(view);
}
bool Interceptor::IsViewAlive(base::LynxEntityId view) {
  auto& state = GetState();
  std::lock_guard<std::mutex> lock(state.mutex);
  return state.views.count(view) != 0;
}
}  // namespace lynx::shell
