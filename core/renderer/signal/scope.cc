// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/signal/scope.h"

#include <utility>

#include "core/renderer/signal/computation.h"
#include "core/renderer/signal/signal_context.h"
#include "core/runtime/vm/lepus/context.h"

namespace lynx {
namespace tasm {

BaseScope::BaseScope(SignalContext* context, lepus::Context* vm_context)
    : signal_context_(context), vm_context_(vm_context) {}

BaseScope::~BaseScope() {}

void BaseScope::CleanUp() {
  for (auto computation : owned_computation_) {
    computation->CleanUp();
  }
  owned_computation_.clear();

  for (auto clean_up : clean_up_callbacks_) {
    vm_context_->CallClosure(clean_up);
  }
  clean_up_callbacks_.clear();
}

void BaseScope::OnCleanUp(const lepus::Value& block) {
  clean_up_callbacks_.emplace_back(block);
}

void BaseScope::AdoptComputation(fml::RefPtr<Computation>&& computation) {
  computation->owner_ = this;
  owned_computation_.emplace_back(std::move(computation));
}

BaseScope* BaseScope::GetOwner() { return owner_; }

Scope::Scope(SignalContext* signal_context_ptr, lepus::Context* vm_context_ptr,
             const lepus::Value& closure)
    : BaseScope(signal_context_ptr, vm_context_ptr) {
  signal_context()->PushScope(this);
  signal_context()->PushComputation(nullptr);

  signal_context()->RunUpdates([closure = std::move(closure), this]() {
    result_ = vm_context()->CallClosure(closure,
                                        lepus::Value(fml::RefPtr<Scope>(this)));
  });

  signal_context()->PopComputation();
  signal_context()->PopScope();

  signal_context()->RecordScope(this);
}

Scope::~Scope() {
  CleanUp();
  if (!will_destroy_) {
    signal_context()->EraseScope(this);
  }
}

lepus::Value Scope::ObtainResult() {
  lepus::Value result = std::move(result_);
  result_ = lepus::Value();
  return result;
}

void Scope::WillDestroy() {
  will_destroy_ = true;
  CleanUp();
}

}  // namespace tasm
}  // namespace lynx
