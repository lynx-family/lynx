// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/performance/native_memory_usage_query.h"

#include <memory>
#include <utility>

#include "core/renderer/dom/element_manager.h"
#include "core/renderer/template_assembler.h"
#include "core/runtime/js/jsi/jsi.h"
#include "core/services/event_report/event_tracker_platform_impl.h"
#include "core/shell/lynx_engine.h"
#include "core/shell/runtime/bts/bts_runtime.h"
#include "core/shell/runtime/mts/mts_runtime.h"

namespace lynx {
namespace tasm {
namespace performance {
namespace {

int64_t GetMainThreadRuntimeHeapSize(TemplateAssembler* tasm) {
  if (tasm == nullptr) {
    return 0;
  }
  auto entry = tasm->FindTemplateEntry(DEFAULT_ENTRY_NAME);
  if (!entry) {
    return 0;
  }
  const auto& runtime = entry->GetVm();
  if (!runtime) {
    return 0;
  }
  // This reads current heap stats only. It must not trigger GC because an
  // active query should not perturb the memory state that it is trying to
  // observe.
  return runtime->GetCurrentHeapSizeBytes();
}

void MergeNativeMemoryUsageSnapshot(NativeMemoryUsageSnapshot& target,
                                    const NativeMemoryUsageSnapshot& source) {
  // Engine and runtime actors own different memory categories, so additive
  // merge is enough except for the runtime group id, where the non-empty value
  // identifies a shared BTS runtime sample.
  target.element_bytes_ += source.element_bytes_;
  target.element_node_count_ += source.element_node_count_;
  target.main_thread_runtime_bytes_ += source.main_thread_runtime_bytes_;
  target.background_thread_runtime_bytes_ +=
      source.background_thread_runtime_bytes_;
  if (!source.bts_runtime_group_id_.empty()) {
    target.bts_runtime_group_id_ = source.bts_runtime_group_id_;
  }
}

void FinishNativeMemoryUsageQuery(NativeMemoryUsageCallback callback,
                                  NativeMemoryUsageSnapshot snapshot) {
  if (!callback) {
    return;
  }
  callback(std::move(snapshot));
}

template <typename ActorImpl>
bool CanCollectFromActor(
    const std::shared_ptr<shell::LynxActor<ActorImpl>>& actor) {
  return actor != nullptr && actor->Impl() != nullptr;
}

int CountPendingParts(bool has_engine_actor, bool has_runtime_actor) {
  // Keep the fan-in counter derived from actor availability instead of
  // hardcoding the current number of actor sources. This makes the query safer
  // to extend if another native memory source is added later.
  int pending_parts = 0;
  if (has_engine_actor) {
    ++pending_parts;
  }
  if (has_runtime_actor) {
    ++pending_parts;
  }
  return pending_parts;
}

}  // namespace

struct NativeMemoryUsageQuery::QueryState {
  QueryState(int pending_count, NativeMemoryUsageCallback callback)
      : pending_count_(pending_count), callback_(std::move(callback)) {}

  // This state exists only for multi-actor fan-in. Engine and runtime samples
  // are collected on their own actor threads, then posted back to the report
  // thread before touching the merged snapshot or pending counter. Keeping all
  // mutation on the report thread avoids adding locks around snapshot merge.
  NativeMemoryUsageSnapshot snapshot_;
  int pending_count_{0};
  NativeMemoryUsageCallback callback_;
};

void NativeMemoryUsageQuery::CollectAsync(
    const std::shared_ptr<shell::LynxActor<shell::LynxEngine>>& engine_actor,
    const std::shared_ptr<shell::LynxActor<shell::BTSRuntime>>& runtime_actor,
    NativeMemoryUsageCallback callback) {
  if (!callback) {
    return;
  }
  const bool has_engine_actor = CanCollectFromActor(engine_actor);
  const bool has_runtime_actor = CanCollectFromActor(runtime_actor);
  const int pending_part_count =
      CountPendingParts(has_engine_actor, has_runtime_actor);

  // LynxActor may exist while its implementation is absent or disabled, for
  // example when runtime creation is skipped or during teardown. Such actors
  // will not execute ActAsync callbacks, so they are excluded from the pending
  // count up front and the query returns an empty or partial snapshot instead
  // of waiting for a callback that can never arrive.
  if (pending_part_count == 0) {
    ReturnSnapshotOnReportThread(std::move(callback),
                                 NativeMemoryUsageSnapshot());
    return;
  }

  if (pending_part_count == 1) {
    // With only one executable actor, there is nothing to merge. Post the
    // actor's complete snapshot directly back to the report thread so callers
    // still observe the same callback thread without allocating shared fan-in
    // state.
    if (has_engine_actor) {
      CollectEngineSnapshotAsync(
          engine_actor, [callback = std::move(callback)](
                            NativeMemoryUsageSnapshot snapshot) mutable {
            ReturnSnapshotOnReportThread(std::move(callback),
                                         std::move(snapshot));
          });
    } else {
      CollectRuntimeSnapshotAsync(
          runtime_actor, [callback = std::move(callback)](
                             NativeMemoryUsageSnapshot snapshot) mutable {
            ReturnSnapshotOnReportThread(std::move(callback),
                                         std::move(snapshot));
          });
    }
    return;
  }

  auto query_state =
      std::make_shared<QueryState>(pending_part_count, std::move(callback));
  CollectEngineSnapshotAsync(
      engine_actor, [query_state](NativeMemoryUsageSnapshot snapshot) mutable {
        CompletePartOnReportThread(std::move(query_state), std::move(snapshot));
      });
  CollectRuntimeSnapshotAsync(
      runtime_actor, [query_state = std::move(query_state)](
                         NativeMemoryUsageSnapshot snapshot) mutable {
        CompletePartOnReportThread(std::move(query_state), std::move(snapshot));
      });
}

void NativeMemoryUsageQuery::CollectEngineSnapshotAsync(
    const std::shared_ptr<shell::LynxActor<shell::LynxEngine>>& engine_actor,
    NativeMemoryUsagePartCallback callback) {
  engine_actor->ActAsync([callback =
                              std::move(callback)](auto& engine) mutable {
    // The engine actor owns TemplateAssembler, so element-tree memory and the
    // main-thread runtime heap are sampled together on this thread. Missing
    // sub-objects are treated as zero because the query may race with early
    // initialization or teardown.
    NativeMemoryUsageSnapshot snapshot;
    auto* tasm = engine->GetTasm();
    if (tasm != nullptr && tasm->page_proxy() != nullptr &&
        tasm->page_proxy()->element_manager() != nullptr &&
        tasm->page_proxy()->element_manager()->node_manager() != nullptr) {
      auto* node_manager =
          tasm->page_proxy()->element_manager()->node_manager();
      snapshot.element_node_count_ =
          static_cast<int32_t>(node_manager->NodeCount());
      snapshot.element_bytes_ = node_manager->GetTotalMemoryUsage();
    }
    snapshot.main_thread_runtime_bytes_ = GetMainThreadRuntimeHeapSize(tasm);
    callback(std::move(snapshot));
  });
}

void NativeMemoryUsageQuery::CollectRuntimeSnapshotAsync(
    const std::shared_ptr<shell::LynxActor<shell::BTSRuntime>>& runtime_actor,
    NativeMemoryUsagePartCallback callback) {
  runtime_actor->ActAsync(
      [callback = std::move(callback)](auto& runtime) mutable {
        // The BTS runtime actor owns the background JS runtime. Its group id is
        // returned with the heap size so platform collectors can deduplicate
        // shared runtime samples across Lynx instances.
        NativeMemoryUsageSnapshot snapshot;
        auto* js_runtime = runtime->GetJSRuntimeWeak().Lock();
        if (js_runtime != nullptr) {
          snapshot.bts_runtime_group_id_ = js_runtime->getGroupId();
          snapshot.background_thread_runtime_bytes_ =
              js_runtime->GetCurrentHeapSizeBytes();
        }
        callback(std::move(snapshot));
      });
}

void NativeMemoryUsageQuery::CompletePartOnReportThread(
    std::shared_ptr<QueryState> query_state,
    NativeMemoryUsageSnapshot snapshot) {
  report::EventTrackerPlatformImpl::GetReportTaskRunner()->PostTask(
      [query_state = std::move(query_state),
       snapshot = std::move(snapshot)]() mutable {
        MergeNativeMemoryUsageSnapshot(query_state->snapshot_, snapshot);
        query_state->pending_count_--;
        if (query_state->pending_count_ == 0) {
          FinishNativeMemoryUsageQuery(std::move(query_state->callback_),
                                       std::move(query_state->snapshot_));
        }
      });
}

void NativeMemoryUsageQuery::ReturnSnapshotOnReportThread(
    NativeMemoryUsageCallback callback, NativeMemoryUsageSnapshot snapshot) {
  report::EventTrackerPlatformImpl::GetReportTaskRunner()->PostTask(
      [callback = std::move(callback),
       snapshot = std::move(snapshot)]() mutable {
        FinishNativeMemoryUsageQuery(std::move(callback), std::move(snapshot));
      });
}

}  // namespace performance
}  // namespace tasm
}  // namespace lynx
