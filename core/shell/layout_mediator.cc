// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/layout_mediator.h"

#include <utility>

#include "base/include/fml/make_copyable.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/tasm/config.h"
#include "core/runtime/piper/js/lynx_runtime.h"
#include "core/runtime/piper/js/runtime_constant.h"
#include "core/services/long_task_timing/long_task_monitor.h"
#if ENABLE_AIR
#include "core/renderer/dom/air/air_element/air_element.h"
#endif

namespace lynx {
namespace shell {
#pragma mark LayoutMediator
LayoutMediator::LayoutMediator(
    const std::shared_ptr<TASMOperationQueue> &operation_queue)
    : operation_queue_(operation_queue), air_node_manager_{nullptr} {}

void LayoutMediator::OnLayoutUpdate(
    int tag, float x, float y, float width, float height,
    const std::array<float, 4> &paddings, const std::array<float, 4> &margins,
    const std::array<float, 4> &borders,
    const std::array<float, 4> *sticky_positions, float max_height) {
  std::array<float, 4> sticky_positions_clone;
  bool has_sticky = false;
  if (sticky_positions) {
    sticky_positions_clone = *sticky_positions;
    has_sticky = true;
  }

  // If node map is empty, no need to EnqueueOperation.
  // IsActive means if there is any node in node_manager(if node_manager is
  // empty means the process of loadTemplate doesn't create any node, maybe some
  // error occurs or it's in air mode, so the next step is checking if
  // air_node_manager works normally)
  // TODO(renpengcheng): when air_element was deleted, check the rationality of
  // this logic
  if (node_manager_ != nullptr && !enable_air_strict_mode_) {
    operation_queue_->EnqueueOperation(
        [node_manager = node_manager_, tag, x, y, width, height, paddings,
         margins, borders, sticky_positions_clone, has_sticky, max_height]() {
          auto *node = node_manager->Get(tag);
          if (node != nullptr) {
            if (has_sticky) {
              node->UpdateLayout(x, y, width, height, paddings, margins,
                                 borders, &sticky_positions_clone, max_height);
            } else {
              node->UpdateLayout(x, y, width, height, paddings, margins,
                                 borders, nullptr, max_height);
            }
          }
        });
  }
#if ENABLE_AIR
  else if (air_node_manager_ != nullptr) {
    operation_queue_->EnqueueOperation(
        [tag, x, y, width, height, paddings, margins, borders, max_height,
         air_node_manager = air_node_manager_]() {
          std::shared_ptr<tasm::AirElement> node = air_node_manager->Get(tag);
          if (node) {
            node.get()->UpdateLayout(x, y, width, height, paddings, margins,
                                     borders, nullptr, max_height);
          }
        });
  }
#endif
}

void LayoutMediator::OnNodeLayoutAfter(int32_t id) {
  operation_queue_->EnqueueOperation([catalyzer = catalyzer_, id]() {
    if (catalyzer != nullptr) {
      catalyzer->painting_context()->OnCollectExtraUpdates(id);
    }
  });
}

void LayoutMediator::OnLayoutAfter(
    const tasm::PipelineOptions &options,
    std::unique_ptr<tasm::PlatformExtraBundleHolder> holder, bool has_layout) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LayoutMediator.OnLayoutAfter");
  bool is_first_layout = false;
  if (!has_first_layout_ && has_layout) {
    has_first_layout_ = true;
    is_first_layout = true;
  }
  // Pass the opions to the tasm thread through the tasm queue, and mount them
  // on the PaintingContext. The UI Flush stage reads the opions from the
  // PaintingContext for collecting timing, and clears the opions at the end.
  if (catalyzer_ != nullptr &&
      (options.is_first_screen || options.is_reload_template ||
       options.need_timestamps)) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY,
                "OnLayoutAfter.EnqueueOperation.AppendOptionsForTiming",
                [&options](lynx::perfetto::EventContext ctx) {
                  options.UpdateTraceDebugInfo(ctx.event());
                });
    operation_queue_->EnqueueTrivialOperation(
        [catalyzer = catalyzer_, options = options]() {
          if (catalyzer != nullptr) {
            TRACE_EVENT(LYNX_TRACE_CATEGORY,
                        "PaintingContext.AppendOptionsForTiming",
                        [&options](lynx::perfetto::EventContext ctx) {
                          options.UpdateTraceDebugInfo(ctx.event());
                        });
            catalyzer->painting_context()->AppendOptionsForTiming(options);
          }
        });
  }
  if (!engine_actor_->CanRunNow()) {
    operation_queue_->AppendPendingTask();

    // FIXME(heshan):when renderTemplate has no patch, is_first_screen
    // will be false forever.
    // need to mark the flag as true in ElementManager::Delegate.
    if (is_first_layout) {
      operation_queue_->has_first_screen_ = true;
      operation_queue_->first_screen_cv_.notify_one();
    }
  }

  engine_actor_->Act([queue = operation_queue_.get(), catalyzer = catalyzer_,
                      options = options, h = std::move(holder), has_layout,
                      is_first_layout, facade_actor = facade_actor_,
                      node_manager = node_manager_](auto &engine) mutable {
    options.has_layout = has_layout;
    HandlePendingLayoutTask(queue, catalyzer, options);
    HandleListOrComponentUpdated(node_manager, options);

    if (options.has_layout) {
      // TODO(heshan): now trigger onFirstScreen when first layout,
      // but it is inconsistent with options.is_first_screen.
      facade_actor->Act([is_first_screen = is_first_layout](auto &facade) {
        facade->OnPageChanged(is_first_screen);
      });
    }
  });

  // As the layout is triggered twice in the layout process, and the second
  // layout copies the options, the HandleListOrComponentUpdated may also
  // trigger twice. Therefore, updated_list_elements_ has to be cleared after
  // the first call.
  options.updated_list_elements_.clear();
  if (is_first_layout) {
    runtime_actor_->ActAsync([](auto &runtime) {
      runtime::MessageEvent event(runtime::kMessageEventTypeOnAppFirstScreen,
                                  runtime::ContextProxy::Type::kCoreContext,
                                  runtime::ContextProxy::Type::kJSContext,
                                  lepus::Value());
      runtime->OnReceiveMessageEvent(std::move(event));
    });
  }
}

void LayoutMediator::PostPlatformExtraBundle(
    int32_t id, std::unique_ptr<tasm::PlatformExtraBundle> bundle) {
  operation_queue_->EnqueueOperation(fml::MakeCopyable(
      [catalyzer = catalyzer_, id, platform_bundle = std::move(bundle)]() {
        if (catalyzer != nullptr) {
          catalyzer->painting_context()->UpdatePlatformExtraBundle(
              id, platform_bundle.get());
        }
      }));
}

void LayoutMediator::OnCalculatedViewportChanged(
    const tasm::CalculatedViewport &viewport, int tag) {
  // Send onWindowResize to front-end
  auto arguments = lepus::CArray::Create();
  auto params = lepus::CArray::Create();
  params->emplace_back(viewport.width);
  params->emplace_back(viewport.height);
  // name
  BASE_STATIC_STRING_DECL(kOnWindowResize, "onWindowResize");
  arguments->emplace_back(kOnWindowResize);
  // params
  arguments->emplace_back(std::move(params));

  auto arguments_value = lepus_value(std::move(arguments));

  runtime_actor_->ActAsync([arguments_value](auto &runtime) {
    runtime->CallJSFunction("GlobalEventEmitter", "emit", arguments_value);
  });

  // trigger page onResize
  engine_actor_->Act([arguments_value, tag](auto &engine) {
    engine->SendCustomEvent("resize", tag, arguments_value, "params");
  });
}

void LayoutMediator::SetTiming(tasm::Timing timing) {
  timing_actor_->Act(
      [timing = std::move(timing)](auto &timing_handler) mutable {
        timing_handler->SetTiming(std::move(timing));
      });
}

// @note: run on tasm thread
void LayoutMediator::HandlePendingLayoutTask(TASMOperationQueue *queue,
                                             tasm::Catalyzer *catalyzer,
                                             tasm::PipelineOptions options) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LayoutMediator.HandlePendingLayoutTask");
  if (catalyzer == nullptr) {
    return;
  }
  tasm::timing::LongTaskMonitor::Scope longTaskScope(
      catalyzer->GetInstanceId(), tasm::timing::kNativeFuncTask,
      tasm::timing::kTaskNameHandlePendingLayoutTask);
  // If Flush return false, means layout has no change.
  bool layout_changed = queue->Flush() || catalyzer->NeedUpdateLayout();
  catalyzer->painting_context()->MarkLayoutUIOperationQueueFlushStartIfNeed();
  if (layout_changed) {
    catalyzer->UpdateLayoutRecursively();
    catalyzer->painting_context()->UpdateLayoutPatching();
    catalyzer->painting_context()->OnFirstScreen();
  } else {
    catalyzer->UpdateLayoutRecursivelyWithoutChange();
  }

  catalyzer->painting_context()->UpdateNodeReadyPatching();

  // ensure FinishLayoutOperation in the end before Flush
  catalyzer->painting_context()->FinishLayoutOperation(options);
  catalyzer->painting_context()->Flush();

#if ENABLE_TRACE_PERFETTO
  catalyzer->DumpElementTree();
#endif
}

// @note: run on tasm thread
// Trigger list element update or notify list element that child component has
// finished rendering.
void LayoutMediator::HandleListOrComponentUpdated(
    tasm::NodeManager *node_manager, const tasm::PipelineOptions &options) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "LayoutMediator.HandleListOrComponentUpdated");
  if (node_manager) {
    static bool enable_native_list_nested =
        tasm::LynxEnv::GetInstance().EnableNativeListNested();
    if (!options.updated_list_elements_.empty() &&
        (enable_native_list_nested ||
         (!enable_native_list_nested && options.list_id_ == 0 &&
          options.operation_id == 0 && options.list_comp_id_ == 0))) {
      TRACE_EVENT(LYNX_TRACE_CATEGORY, "OnLayoutAfter.OnListElementUpdated",
                  [&options](lynx::perfetto::EventContext ctx) {
                    options.UpdateTraceDebugInfo(ctx.event());
                  });
      for (auto tag : options.updated_list_elements_) {
        auto *node = node_manager->Get(tag);
        if (node) {
          node->OnListElementUpdated(options);
        }
      }
    }
    if (options.list_id_ != 0 && options.operation_id != 0 &&
        options.list_comp_id_ != 0) {
      TRACE_EVENT(LYNX_TRACE_CATEGORY, "OnLayoutAfter.OnComponentFinished",
                  [&options](lynx::perfetto::EventContext ctx) {
                    options.UpdateTraceDebugInfo(ctx.event());
                  });
      auto *list_node = node_manager->Get(options.list_id_);
      auto *component_node = node_manager->Get(options.list_comp_id_);
      if (list_node) {
        list_node->OnComponentFinished(component_node, options);
      }
    } else if (options.list_id_ != 0 && !options.operation_ids_.empty() &&
               !options.list_item_ids_.empty() &&
               options.operation_ids_.size() == options.list_item_ids_.size()) {
      TRACE_EVENT(LYNX_TRACE_CATEGORY, "OnLayoutAfter.OnListItemBatchFinished",
                  [&options](lynx::perfetto::EventContext ctx) {
                    options.UpdateTraceDebugInfo(ctx.event());
                  });
      auto *list_element = node_manager->Get(options.list_id_);
      if (list_element) {
        list_element->OnListItemBatchFinished(options);
      }
    }
  }
}

// @note: run on tasm thread
// Should be work on the thread that tasm work on. And must be called after
// notifying safepoint.
void LayoutMediator::HandleLayoutVoluntarily(TASMOperationQueue *queue,
                                             tasm::Catalyzer *catalyzer) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LayoutMediator.HandleLayoutVoluntarily",
              "has_first_screen_", queue->has_first_screen_.load());
  // when part on layout, usually, layout is faster than create ui.
  // even if layout slower, at most a few ms.
  // for first screen, we need ensure sync fetch layout result.
  // so just set a time out for 50ms avoid anr if some error occurred.
  // for other case, like update data, just try fetch layout result.
  using namespace std::chrono_literals;  // NOLINT
  static constexpr auto kFirstScreenWaitTimeout = 50ms;
  tasm::PipelineOptions options;

  if (!queue->has_first_screen_) {
    std::mutex first_screen_cv_mutex;
    std::unique_lock<std::mutex> local_lock(first_screen_cv_mutex);
    if (!queue->first_screen_cv_.wait_for(
            local_lock, kFirstScreenWaitTimeout,
            [queue] { return queue->has_first_screen_.load(); })) {
      LOGE("HandleLayoutVoluntarily wait layout finish failed.");
    }
  }

  HandlePendingLayoutTask(queue, catalyzer, std::move(options));
}

void LayoutMediator::OnFirstMeaningfulLayout() {
  engine_actor_->Act([catalyzer = catalyzer_](auto &engine) {
    if (catalyzer != nullptr) {
      catalyzer->painting_context()->OnFirstMeaningfulLayout();
    }
  });
}

}  // namespace shell
}  // namespace lynx
