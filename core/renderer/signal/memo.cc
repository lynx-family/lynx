// Inspired by S.js by Adam Haile, https://github.com/adamhaile/S
/**
The MIT License (MIT)

Copyright (c) 2017 Adam Haile

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/signal/memo.h"

#include "core/renderer/signal/computation.h"
#include "core/renderer/signal/signal_context.h"

namespace lynx {
namespace tasm {

Memo::Memo(SignalContext* signal_context, lepus::Context* vm_context,
           const lepus::Value& value)
    : Signal(signal_context, vm_context, value) {}

Memo::~Memo() {}

void Memo::InitComputation(const lepus::Value& closure) {
  computation_ = fml::MakeRefCounted<Computation>(signal_context_, vm_context_,
                                                  closure, value_, false, this);
}

void Memo::CleanUp() { computation_->CleanUp(); }

void Memo::OnCleanUp(const lepus::Value& block) {
  computation_->OnCleanUp(block);
}

void Memo::OnInvoked(const lepus::Value& value) { SetValue(value); }

void Memo::MarkDownStream() {
  for (auto computation : computation_list_) {
    if (computation->GetState() == ScopeState::kStateNone) {
      computation->SetState(ScopeState::kStatePending);

      signal_context()->EnqueueComputation(computation);

      computation->MarkDownStream();
    }
  }
}

void Memo::LookUpstream(Computation* ignore) {
  computation_->LookUpstream(ignore);
}

}  // namespace tasm
}  // namespace lynx
