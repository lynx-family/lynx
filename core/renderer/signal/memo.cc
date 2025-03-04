// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/signal/memo.h"

#include "core/renderer/signal/computation.h"
#include "core/renderer/signal/signal_context.h"

namespace lynx {
namespace tasm {

Memo::Memo(SignalContext* signal_context, lepus::Context* vm_context,
           const lepus::Value& closure, const lepus::Value& value)
    : Signal(signal_context, value),
      computation_(fml::MakeRefCounted<Computation>(
          signal_context_, vm_context, closure, value, false, this)) {}

Memo::~Memo() {}

void Memo::CleanUp() { computation_->CleanUp(); }

void Memo::OnCleanUp(const lepus::Value& block) {
  computation_->OnCleanUp(block);
}

void Memo::OnInvoked(const lepus::Value& value) { SetValue(value); }

void Memo::MarkDownStream() {
  for (auto computation : computation_list_) {
    if (computation->GetState() != ScopeState::kStateNone) {
      computation->SetState(ScopeState::kStatePending);

      signal_context()->EnqueueComputation(computation);

      computation->MarkDownStream();
    }
  }
}

void Memo::LookUpstream(Computation* ignore) {
  if (computation_->GetState() == ScopeState::kStateStale) {
    if (computation_.get() != ignore) {
      signal_context()->RunComputation(computation_.get());
    }
  } else if (computation_->GetState() == ScopeState::kStatePending) {
    computation_->LookUpstream(ignore);
  }
}

}  // namespace tasm
}  // namespace lynx
