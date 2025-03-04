// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_SIGNAL_SIGNAL_CONTEXT_H_
#define CORE_RENDERER_SIGNAL_SIGNAL_CONTEXT_H_

#include <list>
#include <memory>
#include <unordered_set>

#include "base/include/fml/memory/ref_counted.h"
#include "base/include/vector.h"
#include "core/renderer/signal/scope.h"

namespace lynx {
namespace tasm {

class Computation;
class Scope;

class SignalContext {
 public:
  SignalContext();

  void PushScope(BaseScope* scope);

  void PopScope();

  BaseScope* GetTopScope();

  void PushComputation(Computation* scope);

  void PopComputation();

  Computation* GetTopComputation();

  void MarkUnTrack(bool enable_un_track) { enable_un_track_ = enable_un_track; }

  void RunUpdates(std::function<void()>&& func);

  void CompleteUpdates(bool wait);

  void EnqueueComputation(Computation* computation);

  void RunComputation(Computation* computation);

  void RunComputation(
      std::shared_ptr<std::list<fml::RefPtr<Computation>>>&& list);

  void UpdateComputation(Computation* computation);

  void WillDestroy();

  void RecordScope(Scope* scope);

  void EraseScope(Scope* scope);

 private:
  void EnsurePureComputationList();
  void EnsureMemoComputationList();

  bool IsScopeActiveComputation(BaseScope* scope);

  bool enable_un_track_{false};

  int32_t exec_count_{0};

  base::Vector<BaseScope*> scope_stack_;

  base::Vector<Computation*> computation_stack_;

  std::shared_ptr<std::list<fml::RefPtr<Computation>>>
      pure_computation_list_ptr_;

  std::shared_ptr<std::list<fml::RefPtr<Computation>>>
      memo_computation_list_ptr_;

  std::unordered_set<Scope*> scope_set_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_SIGNAL_SIGNAL_CONTEXT_H_
