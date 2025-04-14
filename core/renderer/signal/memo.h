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

#ifndef CORE_RENDERER_SIGNAL_MEMO_H_
#define CORE_RENDERER_SIGNAL_MEMO_H_

#include "core/renderer/signal/lynx_signal.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/ref_counted_class.h"
#include "core/runtime/vm/lepus/ref_type.h"

namespace lynx {

namespace tasm {

class SignalContext;
class Computation;

class Memo : public Signal {
 public:
  Memo(SignalContext* signal_context, lepus::Context* vm_context,
       const lepus::Value& value);

  virtual ~Memo();

  void InitComputation(const lepus::Value& closure);

  lepus::RefType GetRefType() const override { return lepus::RefType::kMemo; }

  void CleanUp();

  void OnCleanUp(const lepus::Value& block);

  void OnInvoked(const lepus::Value& value);

  void MarkDownStream();

  void LookUpstream(Computation* ignore) override;

  Computation* GetComputation() { return computation_.get(); }

 private:
  fml::RefPtr<Computation> computation_{nullptr};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_SIGNAL_MEMO_H_
