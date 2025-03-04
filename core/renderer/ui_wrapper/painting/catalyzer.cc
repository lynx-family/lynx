// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/catalyzer.h"

#include <utility>

#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/starlight/layout/layout_object.h"
#include "core/renderer/ui_wrapper/painting/painting_context.h"
#if ENABLE_TRACE_PERFETTO
#include "core/renderer/dom/snapshot_element.h"
#include "core/services/event_report/event_tracker_platform_impl.h"
#endif
#if ENABLE_AIR
#include "core/renderer/dom/air/air_element/air_element.h"
#endif

namespace lynx {
namespace tasm {

class NodeIndexPair {
 public:
  Element* node;
  int index;
  NodeIndexPair(Element* node, int index) {
    this->node = node;
    this->index = index;
  }
};

Catalyzer::Catalyzer(std::unique_ptr<PaintingContext> painting_context,
                     int32_t instance_id)
    : painting_context_(std::move(painting_context)),
      instance_id_(instance_id) {}

bool Catalyzer::NeedUpdateLayout() { return root_ && root_->need_update(); }

void Catalyzer::UpdateLayoutRecursively() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "UpdateLayoutRecursively");
  if (root_) {
    root_->element_container()->UpdateLayout(root_->left(), root_->top());
  }
#if ENABLE_AIR
  else if (air_root_) {
    air_root_->element_container()->UpdateLayout(air_root_->left(),
                                                 air_root_->top());
  }
#endif
}

void Catalyzer::UpdateLayoutRecursivelyWithoutChange() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "Catalyzer::TriggerOnNodeReady");
  if (root_ && root_->element_container()) {
    root_->element_container()->UpdateLayoutWithoutChange();
  }
#if ENABLE_AIR
  else if (air_root_ && air_root_->element_container()) {
    air_root_->element_container()->UpdateLayoutWithoutChange();
  }
#endif
}

std::vector<float> Catalyzer::getBoundingClientOrigin(Element* node) {
  return painting_context_->getBoundingClientOrigin(node->impl_id());
}

std::vector<float> Catalyzer::getWindowSize(Element* node) {
  return painting_context_->getWindowSize(node->impl_id());
}

std::vector<float> Catalyzer::GetRectToWindow(Element* node) {
  return painting_context_->GetRectToWindow(node->impl_id());
}

std::vector<float> Catalyzer::GetRectToLynxView(Element* node) {
  return painting_context_->GetRectToLynxView(node->impl_id());
}

std::vector<float> Catalyzer::ScrollBy(int64_t id, float width, float height) {
  return painting_context_->ScrollBy(id, width, height);
}

// 1 - active, 2 - fail, 3 - end
void Catalyzer::SetGestureDetectorState(int64_t id, int32_t gesture_id,
                                        int32_t state) {
  painting_context_->SetGestureDetectorState(id, gesture_id, state);
}

void Catalyzer::ConsumeGesture(int64_t id, int32_t gesture_id,
                               const pub::Value& params) {
  painting_context_->ConsumeGesture(id, gesture_id, params);
}

void Catalyzer::Invoke(
    int64_t id, const std::string& method, const pub::Value& params,
    const std::function<void(int32_t code, const pub::Value& data)>& callback) {
  return painting_context_->Invoke(id, method, params, callback);
}

#if ENABLE_TRACE_PERFETTO

void Catalyzer::DumpElementTree() {
  if (!TRACE_EVENT_CATEGORY_ENABLED("dom") || !root_) {
    return;
  }
  int64_t now = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
  if (last_dump_time_ > 0 &&
      now - last_dump_time_ < kDumpThresholdMilliseconds) {
    return;
  }
  last_dump_time_ = now;

  if (root_) {
    uint64_t flow_id = TRACE_FLOW_ID();
    TRACE_EVENT("dom", "ConstructElementTree",
                [flow_id](lynx::perfetto::EventContext ctx) {
                  ctx.event()->add_flow_ids(flow_id);
                });

    dom::SnapshotElement* new_root = dom::constructSnapshotElementTree(root_);

    lynx::tasm::report::EventTrackerPlatformImpl::GetReportTaskRunner()
        ->PostTask([this, new_root, flow_id]() {
          rapidjson::Document dumped_document;
          rapidjson::Value dumped_tree =
              dom::DumpSnapshotElementTreeRecursively(new_root,
                                                      dumped_document);
          dumped_document.Swap(dumped_tree);
          rapidjson::StringBuffer buffer;
          rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
          dumped_document.Accept(writer);
          TRACE_EVENT(
              "dom", "DumpElementTree",
              [this, &buffer, flow_id](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_debug_annotations("content",
                                                   buffer.GetString());
                ctx.event()->add_debug_annotations(
                    "instance_id", std::to_string(this->GetInstanceId()));
                ctx.event()->add_terminating_flow_ids(flow_id);
              });
        });
  }
}
#endif

}  // namespace tasm
}  // namespace lynx
