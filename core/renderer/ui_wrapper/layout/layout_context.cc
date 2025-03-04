// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/layout/layout_context.h"

#include <chrono>
#include <cmath>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "base/include/fml/synchronization/waitable_event.h"
#include "base/include/log/logging.h"
#include "base/include/no_destructor.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/public/layout_node_value.h"
#include "core/renderer/dom/attribute_holder.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/lynx_env_config.h"
#include "core/renderer/starlight/layout/box_info.h"
#include "core/renderer/starlight/layout/layout_global.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/renderer/starlight/style/default_layout_style.h"
#include "core/renderer/starlight/types/layout_constraints.h"
#include "core/renderer/starlight/types/layout_directions.h"
#include "core/renderer/starlight/types/nlength.h"
#include "core/renderer/ui_wrapper/layout/layout_context_data.h"
#include "core/renderer/ui_wrapper/layout/no_needed_layout_list.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/vm/lepus/json_parser.h"
#include "core/runtime/vm/lepus/table.h"
#include "core/services/long_task_timing/long_task_monitor.h"
#include "core/services/recorder/recorder_controller.h"
#include "core/services/timing_handler/timing_constants.h"
namespace lynx {
namespace tasm {

LayoutContext::LayoutContext(
    std::unique_ptr<Delegate> delegate,
    std::unique_ptr<LayoutCtxPlatformImpl> platform_impl,
    const LynxEnvConfig& lynx_env_config, int32_t instance_id)
    : platform_impl_(std::move(platform_impl)),
      delegate_(std::move(delegate)),
      root_(nullptr),
      layout_wanted_(false),
      has_viewport_ready_(false),
      enable_layout_(false),
      has_layout_required_(false),
      viewport_(),
      init_css_style_(std::make_unique<starlight::ComputedCSSStyle>(
          lynx_env_config.LayoutsUnitPerPx(),
          lynx_env_config.PhysicalPixelsPerLayoutUnit())),
      lynx_env_config_(lynx_env_config),
      instance_id_(instance_id) {
  // TODO(chennengshi), add test for lynx_shell_builder_unittest, then the
  // condition can be deleted.
  if (platform_impl_) {
    platform_impl_->SetLayoutNodeManager(this);
  }
}

LayoutContext::~LayoutContext() {
  if (platform_impl_ != nullptr) {
    DestroyPlatformNodesIfNeeded();
    platform_impl_->Destroy();
  }
  SetRootInner(nullptr);
}

void LayoutContext::SetMeasureFunc(int32_t id,
                                   std::unique_ptr<MeasureFunc> measure_func) {
  auto node = FindNodeById(id);
  if (node == nullptr) {
    return;
  }
  node->SetMeasureFunc(std::move(measure_func));
}

void LayoutContext::MarkDirtyAndRequestLayout(int32_t id) {
  auto node = FindNodeById(id);
  if (node == nullptr) {
    return;
  }
  node->MarkDirtyAndRequestLayout();
}

void LayoutContext::MarkDirtyAndForceLayout(int32_t id) {
  auto node = FindNodeById(id);
  if (node == nullptr) {
    return;
  }
  node->slnode()->MarkDirtyAndRequestLayout(true);
}

bool LayoutContext::IsDirty(int32_t id) {
  auto node = FindNodeById(id);
  if (node == nullptr) {
    return false;
  }
  return node->IsDirty();
}

FlexDirection LayoutContext::GetFlexDirection(int32_t id) {
  auto node = FindNodeById(id);
  starlight::FlexDirectionType result =
      node ? node->slnode()->GetCSSMutableStyle()->GetFlexDirection()
           : starlight::FlexDirectionType::kColumn;
  return static_cast<FlexDirection>(result);
}

float LayoutContext::GetWidth(int32_t id) {
  auto node = FindNodeById(id);
  if (node == nullptr) {
    return 0.0f;
  }
  // FIXME(liting): vw vh percentage
  auto width = starlight::NLengthToFakeLayoutUnit(
      node->slnode()->GetCSSStyle()->GetWidth());
  return width.ClampIndefiniteToZero().ToFloat();
}

float LayoutContext::GetHeight(int32_t id) {
  auto node = FindNodeById(id);
  if (node == nullptr) {
    return 0.0f;
  }
  // FIXME(liting): vw vh percentage
  auto height = lynx::starlight::NLengthToFakeLayoutUnit(
      node->slnode()->GetCSSStyle()->GetHeight());
  return height.ClampIndefiniteToZero().ToFloat();
}

float LayoutContext::GetPaddingLeft(int32_t id) {
  auto node = FindNodeById(id);
  if (node == nullptr) {
    return 0.0f;
  }
  const auto& style = node->slnode()->GetCSSStyle();
  return starlight::NLengthToFakeLayoutUnit(style->GetPaddingLeft())
      .ClampIndefiniteToZero()
      .ToFloat();
}

float LayoutContext::GetPaddingTop(int32_t id) {
  auto node = FindNodeById(id);
  if (node == nullptr) {
    return 0.0f;
  }
  const auto& style = node->slnode()->GetCSSStyle();
  return starlight::NLengthToFakeLayoutUnit(style->GetPaddingTop())
      .ClampIndefiniteToZero()
      .ToFloat();
}

float LayoutContext::GetPaddingRight(int32_t id) {
  auto node = FindNodeById(id);
  if (node == nullptr) {
    return 0.0f;
  }
  const auto& style = node->slnode()->GetCSSStyle();
  return starlight::NLengthToFakeLayoutUnit(style->GetPaddingRight())
      .ClampIndefiniteToZero()
      .ToFloat();
}

float LayoutContext::GetPaddingBottom(int32_t id) {
  auto node = FindNodeById(id);
  if (node == nullptr) {
    return 0.0f;
  }
  const auto& style = node->slnode()->GetCSSStyle();
  return starlight::NLengthToFakeLayoutUnit(style->GetPaddingBottom())
      .ClampIndefiniteToZero()
      .ToFloat();
}

float LayoutContext::GetMarginLeft(int32_t id) {
  auto node = FindNodeById(id);
  if (node == nullptr) {
    return 0.0f;
  }
  const auto& style = node->slnode()->GetCSSStyle();
  return starlight::NLengthToFakeLayoutUnit(style->GetMarginLeft())
      .ClampIndefiniteToZero()
      .ToFloat();
}

float LayoutContext::GetMarginTop(int32_t id) {
  auto node = FindNodeById(id);
  if (node == nullptr) {
    return 0.0f;
  }
  const auto& style = node->slnode()->GetCSSStyle();
  return starlight::NLengthToFakeLayoutUnit(style->GetMarginTop())
      .ClampIndefiniteToZero()
      .ToFloat();
}

float LayoutContext::GetMarginRight(int32_t id) {
  auto node = FindNodeById(id);
  if (node == nullptr) {
    return 0.0f;
  }
  const auto& style = node->slnode()->GetCSSStyle();
  return starlight::NLengthToFakeLayoutUnit(style->GetMarginRight())
      .ClampIndefiniteToZero()
      .ToFloat();
}

float LayoutContext::GetMarginBottom(int32_t id) {
  auto node = FindNodeById(id);
  if (node == nullptr) {
    return 0.0f;
  }
  const auto& style = node->slnode()->GetCSSStyle();
  return starlight::NLengthToFakeLayoutUnit(style->GetMarginBottom())
      .ClampIndefiniteToZero()
      .ToFloat();
}

float LayoutContext::GetMinWidth(int32_t id) {
  auto node = FindNodeById(id);
  if (node == nullptr) {
    return LayoutNodeStyle::UNDEFINED_MIN_SIZE;
  }
  const auto& style = node->slnode()->GetCSSStyle();
  return starlight::NLengthToFakeLayoutUnit(style->GetMinWidth())
      .ClampIndefiniteToZero()
      .ToFloat();
}

float LayoutContext::GetMaxWidth(int32_t id) {
  auto node = FindNodeById(id);
  if (node == nullptr) {
    return LayoutNodeStyle::UNDEFINED_MAX_SIZE;
  }
  const auto& style = node->slnode()->GetCSSStyle();
  return starlight::NLengthToFakeLayoutUnit(style->GetMaxWidth())
      .ClampIndefiniteToZero()
      .ToFloat();
}

float LayoutContext::GetMinHeight(int32_t id) {
  auto node = FindNodeById(id);
  if (node == nullptr) {
    return LayoutNodeStyle::UNDEFINED_MIN_SIZE;
  }
  const auto& style = node->slnode()->GetCSSStyle();
  return starlight::NLengthToFakeLayoutUnit(style->GetMinHeight())
      .ClampIndefiniteToZero()
      .ToFloat();
}

float LayoutContext::GetMaxHeight(int32_t id) {
  auto node = FindNodeById(id);
  if (node == nullptr) {
    return LayoutNodeStyle::UNDEFINED_MAX_SIZE;
  }
  const auto& style = node->slnode()->GetCSSStyle();
  return starlight::NLengthToFakeLayoutUnit(style->GetMaxHeight())
      .ClampIndefiniteToZero()
      .ToFloat();
}

LayoutResult LayoutContext::UpdateMeasureByPlatform(int32_t id, float width,
                                                    int width_mode,
                                                    float height,
                                                    int height_mode,
                                                    bool final_measure) {
  auto node = FindNodeById(id);
  if (node == nullptr) {
    return LayoutResult{0, 0};
  }
  starlight::Constraints constraints;
  constraints[starlight::kHorizontal] = starlight::OneSideConstraint(
      width, static_cast<SLMeasureMode>(width_mode));
  constraints[starlight::kVertical] = starlight::OneSideConstraint(
      height, static_cast<SLMeasureMode>(height_mode));
  FloatSize result = node->UpdateMeasureByPlatform(constraints, final_measure);
  return LayoutResult{result.width_, result.height_, result.baseline_};
}

void LayoutContext::AlignmentByPlatform(int32_t id, float offset_top,
                                        float offset_left) {
  auto node = FindNodeById(id);
  if (node == nullptr) {
    return;
  }
  node->AlignmentByPlatform(offset_top, offset_left);
}

void LayoutContext::UpdateLayoutNodeProps(
    int32_t id, const std::shared_ptr<PropBundle>& props) {
  auto node = FindNodeById(id);
  UpdateLayoutNodePropsInner(node, props);
}

void LayoutContext::UpdateLayoutNodePropsInner(
    LayoutNode* node, const std::shared_ptr<PropBundle>& props) {
  if (node->is_common() && !node->is_inline_view()) {
    return;
  }
  platform_impl_->UpdateLayoutNode(node->id(), props.get());
}

void LayoutContext::UpdateLayoutNodeFontSize(int32_t id,
                                             double cur_node_font_size,
                                             double root_node_font_size,
                                             double font_scale) {
  auto node = FindNodeById(id);
  UpdateLayoutNodeFontSizeInner(node, cur_node_font_size, root_node_font_size,
                                font_scale);
}

void LayoutContext::UpdateLayoutNodeFontSizeInner(LayoutNode* node,
                                                  double cur_node_font_size,
                                                  double root_node_font_size,
                                                  double font_scale) {
  node->ConsumeFontSize(cur_node_font_size, root_node_font_size, font_scale);
}

void LayoutContext::UpdateLayoutNodeStyle(int32_t id, CSSPropertyID css_id,
                                          const tasm::CSSValue& value) {
  auto node = FindNodeById(id);
  if (node == nullptr || node->slnode() == nullptr) {
    LOGE("[LayoutContext] UpdateLayoutNodeStyle for null, id :"
         << id << " css_id: " << css_id << " value: " << value.AsJsonString());
    LYNX_ERROR(error::E_LAYOUT_INTERNAL, "FindNodeById is null",
               "This error is caught by native, please ask Lynx for help");
    base::ErrorStorage::GetInstance().AddCustomInfoToError("id",
                                                           std::to_string(id));
  }
  UpdateLayoutNodeStyleInner(node, css_id, value);
}

void LayoutContext::UpdateLayoutNodeStyleInner(LayoutNode* node,
                                               lynx::tasm::CSSPropertyID css_id,
                                               const tasm::CSSValue& value) {
  node->ConsumeStyle(css_id, value);
  if (node->slnode()->GetEnableFixedNew()) {
    CheckFixed(node);
  }
}

void LayoutContext::ResetLayoutNodeStyle(int32_t id, CSSPropertyID css_id) {
  auto node = FindNodeById(id);
  ResetLayoutNodeStyleInner(node, css_id);
}

void LayoutContext::ResetLayoutNodeStyleInner(
    LayoutNode* node, lynx::tasm::CSSPropertyID css_id) {
  if (node->slnode()->IsNewFixed()) {
    UpdateFixedNodeSet(node, false);
    node->slnode()->SetIsFixedBefore(false);
  }
  node->ConsumeStyle(css_id, CSSValue::Empty(), true);
}

void LayoutContext::UpdateLayoutNodeAttribute(int32_t id,
                                              starlight::LayoutAttribute key,
                                              const lepus::Value& value) {
  auto node = FindNodeById(id);
  UpdateLayoutNodeAttributeInner(node, key, value);
}

void LayoutContext::UpdateLayoutNodeAttributeInner(
    LayoutNode* node, starlight::LayoutAttribute key,
    const lepus::Value& value) {
  node->ConsumeAttribute(key, value);
}

void LayoutContext::ResetLayoutNodeAttribute(int32_t id,
                                             starlight::LayoutAttribute key) {
  auto node = FindNodeById(id);
  node->ConsumeAttribute(key, lepus::Value(), true);
}

void LayoutContext::UpdateLayoutNodeByBundle(
    int32_t id, std::unique_ptr<LayoutBundle> bundle) {
  auto target_node = bundle->is_create_bundle
                         ? InitLayoutNodeWithBundle(id, bundle.get())
                         : FindNodeById(id);

  if (!target_node) {
    LOGE("[LayoutContext] UpdateLayoutNodeByBundle for null node, id :" << id);
    return;
  }

  if (bundle->cur_node_font_size >= 0 && bundle->root_node_font_size >= 0) {
    UpdateLayoutNodeFontSizeInner(target_node, bundle->cur_node_font_size,
                                  bundle->root_node_font_size,
                                  bundle->font_scale);
  }

  for (const auto& reset_id : bundle->reset_styles) {
    ResetLayoutNodeStyleInner(target_node, reset_id);
  }

  for (const auto& pair : bundle->styles) {
    UpdateLayoutNodeStyleInner(target_node, pair.first, pair.second);
  }

  for (const auto& pair : bundle->attrs) {
    UpdateLayoutNodeAttributeInner(target_node, pair.first, pair.second);
  }

  if (bundle->is_create_bundle) {
    return;
  }

  for (const auto& prop_bundle : bundle->update_prop_bundles) {
    UpdateLayoutNodePropsInner(target_node, prop_bundle);
  }

  if (bundle->is_dirty) {
    target_node->MarkDirty();
  }
}

LayoutNode* LayoutContext::InitLayoutNodeWithBundle(int32_t id,
                                                    LayoutBundle* bundle) {
  auto target_node = CreateLayoutNode(id, bundle->tag);
  if (bundle->is_root) {
    SetRootInner(target_node);
  }

  AttachLayoutNodeTypeInner(target_node, bundle->tag, bundle->allow_inline,
                            bundle->shadownode_prop_bundle);
  return target_node;
}

LayoutNode* LayoutContext::CreateLayoutNode(int32_t id,
                                            const base::String& tag) {
  auto layout_configs = GetLayoutConfigs();
  LayoutNode* layoutNode =
      &layout_nodes_
           .emplace(std::piecewise_construct, std::forward_as_tuple(id),
                    std::forward_as_tuple(id, layout_configs, lynx_env_config_,
                                          *init_css_style_))
           .first->second;
  layoutNode->SetTag(tag);
  if (tag.str() == kListNodeTag) {
    layoutNode->slnode()->MarkList();
  }
  if (layout_configs.enable_fixed_new_ && root_) {
    layoutNode->slnode()->SetRoot(root_->slnode());
  }
  layoutNode->slnode()->SetEventHandler(this);
  if (hierarchy_observer_) {
    hierarchy_observer_->OnLayoutNodeCreated(id, layoutNode);
  }
  return layoutNode;
}

void LayoutContext::InsertLayoutNode(int32_t parent_id, int32_t child_id,
                                     int index) {
  auto parent = FindNodeById(parent_id);
  auto child = FindNodeById(child_id);
  parent->InsertNode(child, index);
  if (!parent->is_common() &&
      (!child->is_common() || child->is_inline_view())) {
    platform_impl_->InsertLayoutNode(parent->id(), child->id(), index);
  }
}

void LayoutContext::RemoveLayoutNodeAtIndex(int32_t parent_id, int index) {
  auto parent = FindNodeById(parent_id);
  auto child = parent->RemoveNodeAtIndex(index);
  if (child == nullptr) return;
  if (child->slnode()->IsNewFixed()) {
    UpdateFixedNodeSet(child, false);
  }
  if (parent && !parent->is_common()) {
    platform_impl_->RemoveLayoutNode(parent->id(), child->id(), index);
  }
}

void LayoutContext::MoveLayoutNode(int32_t parent_id, int32_t child_id,
                                   int from_index, int to_index) {
  auto parent = FindNodeById(parent_id);
  auto child = FindNodeById(child_id);
  parent->MoveNode(child, from_index, to_index);
  if (!parent->is_common()) {
    platform_impl_->MoveLayoutNode(parent->id(), child->id(), from_index,
                                   to_index);
  }
}

void LayoutContext::InsertLayoutNodeBefore(int32_t parent_id, int32_t child_id,
                                           int32_t ref_id) {
  auto parent = FindNodeById(parent_id);
  auto ref_node = FindNodeById(ref_id);
  int index = 0;
  if (ref_node == nullptr) {
    // null ref node indicates to append the child to the end
    index = static_cast<int>(parent->children().size());
  } else {
    index = GetIndexForChild(parent, ref_node);
    if (index < 0) {
      LOGE("LayoutContext::InsertLayoutNodeBefore can not find child!!");
      return;
    }
  }
  InsertLayoutNode(parent_id, child_id, index);
}

void LayoutContext::RemoveLayoutNode(int32_t parent_id, int32_t child_id) {
  auto parent = FindNodeById(parent_id);
  auto child = FindNodeById(child_id);
  int index = GetIndexForChild(parent, child);
  if (index < 0) {
    LOGE("LayoutContext::RemoveLayoutNode can not find child!!");
    return;
  }
  RemoveLayoutNodeAtIndex(parent_id, index);
}

void LayoutContext::DestroyLayoutNode(int32_t id) {
  auto node = FindNodeById(id);
  if (node != nullptr) {
    bool has_platform_shadownode = !node->is_common() || node->is_inline_view();
    if (has_platform_shadownode) {
      destroyed_platform_nodes_.insert(id);
    }
    if (node == root_) {
      // The root node will be destroyed, so we need to set root_ to nullptr
      // to avoid accessing destroyed root node.
      root_ = nullptr;
    }
    layout_nodes_.erase(id);
  }
}
int LayoutContext::GetIndexForChild(LayoutNode* parent, LayoutNode* child) {
  int index = 0;
  bool found = false;
  for (const auto& node : parent->children()) {
    if (node == child) {
      found = true;
      break;
    }
    ++index;
  }
  if (found) {
    return index;
  }
  return -1;
}

void LayoutContext::AttachLayoutNodeType(
    int32_t id, const base::String& tag, bool allow_inline,
    const std::shared_ptr<PropBundle>& props) {
  auto node = FindNodeById(id);
  AttachLayoutNodeTypeInner(node, tag, allow_inline, props);
}

bool LayoutContext::NoNeedPlatformLayoutNode(
    const base::String& tag, const std::shared_ptr<PropBundle>& props) {
  // This map is used to store node tag names and props names without creating a
  // platform layer Layoutnode key: tag name value: props name
  const static base::NoDestructor<
      std::unordered_map<std::string, std::unordered_set<std::string>>>
      kCollection({{kImageComponent, {kAutoSizeAttribute}}});
  bool no_needed = false;
  auto it = kCollection->find(tag.str());
  bool found = it != kCollection->end();
  if (found) {
    bool has_required = false;
    for (const auto& attribute : it->second) {
      if (props->Contains(attribute.c_str())) {
        has_required = true;
      }
    }
    if (!has_required) {
      no_needed = true;
    }
  }
  return no_needed;
}

void LayoutContext::AttachLayoutNodeTypeInner(
    LayoutNode* node, const base::String& tag, bool allow_inline,
    const std::shared_ptr<PropBundle>& props) {
  if (node == nullptr) {
    return;
  }
  auto it = node_type_recorder_.find(tag);
  bool found = it != node_type_recorder_.end();
  if (found) {
    node->set_type(static_cast<LayoutNodeType>(it->second));
    if (node->is_common() && !allow_inline) {
      return;
    }
  }
  if (root() && node->id() == root()->id()) {
    node->set_type(LayoutNodeType::COMMON);
    return;
  }
  if (NoNeedPlatformLayoutNode(tag, props)) {
    node->set_type(LayoutNodeType::COMMON);
    return;
  }
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, "LayoutContext.CreateLayoutNode");
  int result = platform_impl_->CreateLayoutNode(node->id(), tag.str(),
                                                props.get(), allow_inline);
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);
  LayoutNodeType type = static_cast<LayoutNodeType>(result);
  node->set_type(type);
  // INLINE type should not be cached, since different parent will change the
  // result
  if (!found) {
    if (!(type & INLINE)) {
      node_type_recorder_.emplace(tag, type);
    }
#if ENABLE_TESTBENCH_RECORDER
    tasm::recorder::TestBenchBaseRecorder::GetInstance().RecordComponent(
        tag.c_str(), type, record_id_);
#endif
  }
}

void LayoutContext::MarkDirty(int32_t id) {
  auto node = FindNodeById(id);
  auto layout_node = node->FindNonVirtualNode();
  if (layout_node) {
    layout_node->MarkDirty();
  }
}

LayoutNode* LayoutContext::FindNodeById(int32_t id) {
  auto it = layout_nodes_.find(id);
  if (it != layout_nodes_.end()) {
    return &it->second;
  }
  return nullptr;
}

void LayoutContext::DispatchLayoutUpdates(const PipelineOptions& options) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LayoutContext::DispatchLayoutUpdates");
  tasm::timing::LongTaskMonitor::Scope longTaskScope(
      instance_id_, tasm::timing::kNativeFuncTask,
      "LayoutContext::DispatchLayoutUpdates");
  tasm::TimingCollector::Scope<Delegate> scope(delegate_.get(), options);
  enable_layout_ = true;
  DestroyPlatformNodesIfNeeded();
  if (nullptr == root_ || !root_->slnode()) {
    return;
  }
  // The results of Lynx C++ Layout need to be consumed during the platform
  // layout cycle. Therefore, request platform layout first, and then execute
  // Lynx Layout.
  RequestLayout(options);
  Layout(options);
}

void LayoutContext::SetEnableLayout() {
  enable_layout_ = true;
  DestroyPlatformNodesIfNeeded();
}

void LayoutContext::UpdateFixedNodeSet(LayoutNode* node, bool is_insert) {
  if (is_insert) {
    fixed_node_set_.insert(node->slnode());
  } else {
    fixed_node_set_.erase(node->slnode());
  }
}

void LayoutContext::CheckFixed(LayoutNode* node) {
  // If PositionType has been changed, update the fixed node set.
  if (node->slnode()->IsFixed() != node->slnode()->IsFixedBefore()) {
    node->slnode()->SetIsFixedBefore(node->slnode()->IsFixed());
    if (node->slnode()->IsFixed()) {
      UpdateFixedNodeSet(node, true);
    } else {
      UpdateFixedNodeSet(node, false);
    }
  }
}

void LayoutContext::SetFontFaces(const FontFacesMap& fontfaces) {
  platform_impl_->SetFontFaces(fontfaces);
}

void LayoutContext::SetLayoutEarlyExitTiming(const PipelineOptions& options) {
  if (options.need_timestamps) {
    auto* timing_collector = tasm::TimingCollector::Instance();
    timing_collector->Mark(tasm::timing::kLayoutStart);
    timing_collector->Mark(tasm::timing::kLayoutEnd);
  }
}

void LayoutContext::Layout(const PipelineOptions& options) {
  std::string view_port_info_str = base::FormatString(
      " for viewport, size: %.1f, %.1f; mode: %d, %d", viewport_.width,
      viewport_.height, viewport_.width_mode, viewport_.height_mode);

  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, "LayoutContext.Layout",
              [&options](lynx::perfetto::EventContext ctx) {
                options.UpdateTraceDebugInfo(ctx.event());
              });
  if (root_ == nullptr || root_->slnode() == nullptr ||
      !root_->slnode()->IsDirty()) {
    if (root_ == nullptr || root_->slnode() == nullptr) {
      LOGW(
          "[Layout] Element or LayoutObject is not initialized "
          "when Layout is called"
          << view_port_info_str);
    } else {
      LOGD("[Layout] Root is clean when layout is called"
           << view_port_info_str);
    }
    SetLayoutEarlyExitTiming(options);
    delegate_->OnLayoutAfter(options);
    return;
  }
  if (!enable_layout_ || !has_viewport_ready_) {
    layout_wanted_ = true;
    LOGI(
        "[Layout] Layout is disabled or view port isn't ready when "
        "Layout is called"
        << view_port_info_str);
    SetLayoutEarlyExitTiming(options);
    delegate_->OnLayoutAfter(options);
    return;
  };

  UNUSED_LOG_VARIABLE auto time_begin = std::chrono::steady_clock::now();

  if (SetViewportSizeToRootNode()) {
    root()->MarkDirty();
  }

  if (options.need_timestamps) {
    tasm::TimingCollector::Instance()->Mark(tasm::timing::kLayoutStart);
  }
  if (options.enable_report_list_item_life_statistic_ &&
      options.IsRenderListItem()) {
    options.list_item_life_option_.start_layout_time_ =
        base::CurrentTimeMicroseconds();
  }

  // Dispatch OnLayoutBefore
  LOGD("[Layout] Layout start" << view_port_info_str);
  {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "DispatchLayoutBeforeRecursively");
    DispatchLayoutBeforeRecursively(root_);
  }
  // CalculateLayout
  LOGV("[Layout] Computing layout" << view_port_info_str);
  {
    TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, "CalculateLayout");
    root_->CalculateLayout(GetFixedNodeSet());
  }
  LOGV("[Layout] Updating layout result" << view_port_info_str);
  {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "LayoutRecursively");
    LayoutRecursively(root(), options);
  }
  LOGV("[Layout] Dispatch layout after" << view_port_info_str);

  if (options.need_timestamps) {
    tasm::TimingCollector::Instance()->Mark(tasm::timing::kLayoutEnd);
  }
  if (options.enable_report_list_item_life_statistic_ &&
      options.IsRenderListItem()) {
    options.list_item_life_option_.end_layout_time_ =
        base::CurrentTimeMicroseconds();
  }

  TRACE_EVENT(LYNX_TRACE_CATEGORY, "OnLayoutAfter");

  auto root_size = root()->slnode()->GetLayoutResult().size_;
  platform_impl_->UpdateRootSize(root_size.width_, root_size.height_);
  // bundle_holder is transfer and captured by this layout finish callback
  // and it is auto released at the end of this tasm loop
  auto holder = platform_impl_->ReleasePlatformBundleHolder();
  delegate_->OnLayoutAfter(options, std::move(holder), true);

  has_layout_required_ = false;
  layout_wanted_ = false;

  // TODO(huzhanbo.luc): remove this when `OnFirstMeaningfulLayout` is removed
  if (!has_first_page_layout_) {
    // Set the flag first to avoid calling `OnFirstMeaningfulLayout` twice
    has_first_page_layout_ = true;

    delegate_->OnFirstMeaningfulLayout();
  }

  const auto& layout_result = root()->slnode()->GetLayoutResult();
  // Notify that viewport / root size has changed
  if ((calculated_viewport_.width != layout_result.size_.width_ ||
       calculated_viewport_.height != layout_result.size_.height_)) {
    calculated_viewport_.width = layout_result.size_.width_;
    calculated_viewport_.height = layout_result.size_.height_;
    CalculatedViewport viewport;
    viewport.width =
        calculated_viewport_.width / lynx_env_config_.LayoutsUnitPerPx();
    viewport.height =
        calculated_viewport_.height / lynx_env_config_.LayoutsUnitPerPx();
    delegate_->OnCalculatedViewportChanged(viewport, root_id());

    // update LynxView's size info for EventReporter
    report::EventTracker::UpdateGenericInfo(instance_id_, "lynxview_height",
                                            calculated_viewport_.height);
    report::EventTracker::UpdateGenericInfo(instance_id_, "lynxview_width",
                                            calculated_viewport_.width);
  }

  TRACE_EVENT_INSTANT(
      LYNX_TRACE_CATEGORY, "LayoutContext.LayoutResult",
      [&](lynx::perfetto::EventContext ctx) {
        ctx.event()->add_debug_annotations(
            "width", base::FormatString("%.1f", layout_result.size_.width_));
        ctx.event()->add_debug_annotations(
            "height", base::FormatString("%.1f", layout_result.size_.height_));
        ctx.event()->add_debug_annotations("viewport", view_port_info_str);
      });
  UNUSED_LOG_VARIABLE auto time_end = std::chrono::steady_clock::now();
  LOGI("[Layout] layout finish with result size: "
       << layout_result.size_.width_ << ", " << layout_result.size_.height_
       << view_port_info_str << " Time taken: "
       << std::chrono::duration_cast<std::chrono::nanoseconds>(time_end -
                                                               time_begin)
              .count()
       << " ns");
}

void LayoutContext::DispatchLayoutBeforeRecursively(LayoutNode* node) {
  if (node == nullptr || !node->IsDirty()) {
    return;
  }
  if (node->slnode()->GetSLMeasureFunc()) {
    platform_impl_->OnLayoutBefore(node->id());
  }
  for (auto& child : node->children()) {
    DispatchLayoutBeforeRecursively(child);
  }
}

void LayoutContext::UpdateLayoutInfo(LayoutNode* node) {
  // Faster than use YGTransferLayoutOutputsRecursive in YGJNI.cc by 0.5 times
  auto sl_node = node->slnode();
  if (!sl_node) return;
  const auto& layout_result = sl_node->GetLayoutResult();
  float width = layout_result.size_.width_;
  float height = layout_result.size_.height_;
  float top = layout_result.offset_.Y();
  float left = layout_result.offset_.X();
  std::array<float, 4> paddings;
  std::array<float, 4> margins;
  std::array<float, 4> borders;
  // paddings
  paddings[0] = layout_result.padding_[starlight::kLeft];
  paddings[1] = layout_result.padding_[starlight::kTop];
  paddings[2] = layout_result.padding_[starlight::kRight];
  paddings[3] = layout_result.padding_[starlight::kBottom];
  // margins
  margins[0] = layout_result.margin_[starlight::kLeft];
  margins[1] = layout_result.margin_[starlight::kTop];
  margins[2] = layout_result.margin_[starlight::kRight];
  margins[3] = layout_result.margin_[starlight::kBottom];
  // borders
  borders[0] = layout_result.border_[starlight::kLeft];
  borders[1] = layout_result.border_[starlight::kTop];
  borders[2] = layout_result.border_[starlight::kRight];
  borders[3] = layout_result.border_[starlight::kBottom];

  std::array<float, 4>* sticky_positions = nullptr;
  std::array<float, 4> sticky_pos_array;
  if (sl_node->IsSticky()) {
    sticky_pos_array[0] = layout_result.sticky_pos_[starlight::kLeft];
    sticky_pos_array[1] = layout_result.sticky_pos_[starlight::kTop];
    sticky_pos_array[2] = layout_result.sticky_pos_[starlight::kRight];
    sticky_pos_array[3] = layout_result.sticky_pos_[starlight::kBottom];
    sticky_positions = &sticky_pos_array;
  }

  delegate_->OnLayoutUpdate(
      node->id(), left, top, width, height, paddings, margins, borders,
      sticky_positions, sl_node->GetCSSStyle()->GetMaxHeight().GetRawValue());

  if (node->slnode()->GetSLMeasureFunc()) {
    // Dispatch OnLayoutAfter to those nodes that have custom measure
    platform_impl_->OnLayout(node->id(), left, top, width, height, paddings,
                             borders);
    delegate_->OnNodeLayoutAfter(node->id());

    // if node has custom measure function, it may by need pass some bundle to
    auto bundle = platform_impl_->GetPlatformExtraBundle(node->id());

    if (!bundle) {
      return;
    }

    delegate_->PostPlatformExtraBundle(node->id(), std::move(bundle));
  }
}

void LayoutContext::LayoutRecursively(LayoutNode* node,
                                      const PipelineOptions& options) {
  if (!node->IsDirty() && !node->is_virtual()) {
    return;
  }

  if (IfNeedsUpdateLayoutInfo(node)) {
    UpdateLayoutInfo(node);
  }

  for (auto& child : node->children()) {
    LayoutRecursively(child, options);
  }

  node->MarkUpdated();
  if (node->is_list_container()) {
    static bool enable_native_list_nested =
        LynxEnv::GetInstance().EnableNativeListNested();
    if ((!enable_native_list_nested && options.operation_id == 0) ||
        (enable_native_list_nested && node->id() != options.list_id_)) {
      // Note: we should avoid to add parent list node to
      // options.updated_list_elements_ when rendering list item.
      options.updated_list_elements_.emplace_back(node->id());
    }
  }
}

void LayoutContext::DestroyPlatformNodesIfNeeded() {
  if (!destroyed_platform_nodes_.empty()) {
    platform_impl_->DestroyLayoutNodes(destroyed_platform_nodes_);
    destroyed_platform_nodes_.clear();
  }
}

void LayoutContext::SetRoot(int32_t id) {
  auto target_node = FindNodeById(id);
  SetRootInner(target_node);
}

void LayoutContext::SetRootInner(LayoutNode* node) {
  root_ = node;

  if (!root_) return;

  // The default flex direction is column for root
  root_->slnode()->GetCSSMutableStyle()->SetFlexDirection(
      starlight::FlexDirectionType::kColumn);

  root_->slnode()->SetContext(this);
  root_->slnode()->SetSLRequestLayoutFunc([](void* context) {
    static_cast<LayoutContext*>(context)->RequestLayout();
  });

  // We should update viewport when root and layout scheduler are attached,
  // as viewport has been set before.
  if (has_viewport_ready_) {
    UpdateViewport(viewport_.width, viewport_.width_mode, viewport_.height,
                   viewport_.height_mode);
  }
}

void LayoutContext::SetPageConfigForLayoutThread(
    const std::shared_ptr<PageConfig>& config) {
  page_config_ = config;
  lynx_env_config_.SetFontScaleSpOnly(GetLayoutConfigs().font_scale_sp_only_);
  delegate_->SetEnableAirStrictMode(page_config_->GetLynxAirMode() ==
                                    CompileOptionAirMode::AIR_MODE_STRICT);
}

bool LayoutContext::SetViewportSizeToRootNode() {
  if (!root() || !has_viewport_ready_) {
    return false;
  }

  bool is_dirty = false;
  switch (viewport_.width_mode) {
    case SLMeasureModeDefinite:
      is_dirty |= root()->slnode()->GetCSSMutableStyle()->SetWidth(
          starlight::NLength::MakeUnitNLength(viewport_.width));
      is_dirty |= root()->slnode()->GetCSSMutableStyle()->SetMaxWidth(
          starlight::DefaultLayoutStyle::SL_DEFAULT_MAX_WIDTH());
      break;
    case SLMeasureModeAtMost:
      // When max width is set, the pre width mode must be clear
      is_dirty |= root()->slnode()->GetCSSMutableStyle()->SetWidth(
          starlight::NLength::MakeAutoNLength());
      is_dirty |= root()->slnode()->GetCSSMutableStyle()->SetMaxWidth(
          starlight::NLength::MakeUnitNLength(viewport_.width));
      break;
    case SLMeasureModeIndefinite:
      is_dirty |= root()->slnode()->GetCSSMutableStyle()->SetWidth(
          starlight::NLength::MakeAutoNLength());
      is_dirty |= root()->slnode()->GetCSSMutableStyle()->SetMaxWidth(
          starlight::DefaultLayoutStyle::SL_DEFAULT_MAX_WIDTH());
      break;
  }

  switch (viewport_.height_mode) {
    case SLMeasureModeDefinite:
      is_dirty |= root()->slnode()->GetCSSMutableStyle()->SetHeight(
          starlight::NLength::MakeUnitNLength(viewport_.height));
      is_dirty |= root()->slnode()->GetCSSMutableStyle()->SetMaxHeight(
          starlight::DefaultLayoutStyle::SL_DEFAULT_MAX_HEIGHT());
      break;
    case SLMeasureModeAtMost:
      // When max height is set, the pre height mode must be clear
      is_dirty |= root()->slnode()->GetCSSMutableStyle()->SetHeight(
          starlight::NLength::MakeAutoNLength());
      is_dirty |= root()->slnode()->GetCSSMutableStyle()->SetMaxHeight(
          starlight::NLength::MakeUnitNLength(viewport_.height));
      break;
    case SLMeasureModeIndefinite:
      is_dirty |= root()->slnode()->GetCSSMutableStyle()->SetHeight(
          starlight::NLength::MakeAutoNLength());
      is_dirty |= root()->slnode()->GetCSSMutableStyle()->SetMaxHeight(
          starlight::DefaultLayoutStyle::SL_DEFAULT_MAX_HEIGHT());
      break;
  }
  return is_dirty;
}

void LayoutContext::UpdateViewport(float width, int width_mode, float height,
                                   int height_mode, bool need_layout) {
  viewport_.UpdateViewport(width, width_mode, height, height_mode);
  has_viewport_ready_ = true;
  std::string view_port_info_str = base::FormatString(
      "size: %.1f, %.1f; mode: %d, %d", viewport_.width, viewport_.height,
      viewport_.width_mode, viewport_.height_mode);
  TRACE_EVENT_INSTANT(LYNX_TRACE_CATEGORY, "LayoutContext.UpdateViewport",
                      [&](lynx::perfetto::EventContext ctx) {
                        ctx.event()->add_debug_annotations("viewport",
                                                           view_port_info_str);
                      });
  LOGI("[Layout] UpdateViewport :" << view_port_info_str);

  if (SetViewportSizeToRootNode() || (root() && root()->slnode()->IsDirty())) {
    circular_layout_detector_.DetectCircularLayoutDependency();
    root()->slnode()->MarkDirty();
    if (need_layout) {
      RequestLayout();
    }
  }
}

void LayoutContext::UpdateLynxEnvForLayoutThread(LynxEnvConfig env) {
  lynx_env_config_ = env;

  if (!root()) {
    return;
  }

  root()->UpdateLynxEnv(lynx_env_config_);
}

void LayoutContext::RequestLayout(const PipelineOptions& options) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LayoutContext.RequestLayout",
              [&options](lynx::perfetto::EventContext ctx) {
                options.UpdateTraceDebugInfo(ctx.event());
              });
  if (root() && root()->slnode()->IsDirty()) {
    if (layout_wanted_) {
      Layout(options);
    } else if (!has_layout_required_) {
      has_layout_required_ = true;
      platform_impl_->ScheduleLayout([this]() { request_layout_callback_(); });
    }
  }
}

void LayoutContext::CircularLayoutDependencyDetector::
    DetectCircularLayoutDependency() {
  const auto now = lynx::base::CurrentTimeMilliseconds();
  if (last_viewport_update_time_ == -1) {
    continuous_viewport_update_start_time_ = last_viewport_update_time_ = now;
    return;
  }
  if (now - last_viewport_update_time_ > kContinuousViewportUpdateMaxGap) {
    continuous_viewport_update_start_time_ = now;
  }
  if (now - continuous_viewport_update_start_time_ > kTimeWindow) {
    if (!in_error_state_) {
      const auto msg = base::FormatString(
          "Viewport update is triggered continuously through %lld[ms].",
          kTimeWindow);
      LYNX_ERROR(error::E_LAYOUT_PERF_INFINITE_LOOP, msg, "");
      in_error_state_ = true;
    }
  } else {
    in_error_state_ = false;
  }
  last_viewport_update_time_ = now;
}

void LayoutContext::OnLayoutEvent(const starlight::LayoutObject* node,
                                  starlight::LayoutEventType type,
                                  const starlight::LayoutEventData& data) {
  switch (type) {
    case starlight::LayoutEventType::UpdateMeasureBegin: {
      TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, "UpdateMeasure");
    } break;
    case starlight::LayoutEventType::UpdateAlignmentBegin: {
      TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, "UpdateAlignment");
    } break;
    case starlight::LayoutEventType::RemoveAlgorithmRecursiveBegin: {
      TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, "RemoveAlgorithmRecursive");
    } break;
    case starlight::LayoutEventType::RoundToPixelGridBegin: {
      TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, "RoundToPixelGrid");
    } break;
    case starlight::LayoutEventType::UpdateMeasureEnd:
    case starlight::LayoutEventType::UpdateAlignmentEnd:
    case starlight::LayoutEventType::RemoveAlgorithmRecursiveEnd:
    case starlight::LayoutEventType::RoundToPixelGridEnd: {
      TRACE_EVENT_END(LYNX_TRACE_CATEGORY);
    } break;
    case starlight::LayoutEventType::LayoutStyleError: {
      const starlight::LayoutErrorData& error =
          static_cast<const starlight::LayoutErrorData&>(data);
      auto exception = base::LynxError(
          error::E_CSS_UNSUPPORTED_VALUE, error.getErrorMsg(),
          error.getFixSuggestion(), base::LynxErrorLevel::Fatal);
      exception.should_abort_ = true;
      lynx::base::ErrorStorage::GetInstance().SetError(std::move(exception));
    } break;
    case starlight::LayoutEventType::FeatureCountOnGridDisplay: {
      tasm::report::FeatureCounter::Instance()->Count(
          tasm::report::LynxFeature::CPP_USE_GRID_DISPLAY);
    } break;
    case starlight::LayoutEventType::FeatureCountOnRelativeDisplay: {
      tasm::report::FeatureCounter::Instance()->Count(
          tasm::report::LynxFeature::CPP_USE_RELATIVE_DISPLAY);
    } break;
    default:
      break;
  }
}

std::unordered_map<int32_t, LayoutInfoArray>
LayoutContext::GetSubTreeLayoutInfo(int32_t root_id, Viewport viewport) {
  std::unordered_map<int32_t, LayoutInfoArray> result;
  auto node = FindNodeById(root_id);
  if (node) {
    DispatchLayoutBeforeRecursively(node);
    if (viewport.width_mode != MeasureMode::Indefinite ||
        viewport.height_mode != MeasureMode::Indefinite) {
      starlight::Constraints constraints =
          ConvertViewportToOneSideConstraint(viewport);
      node->CalculateLayoutWithConstraints(constraints, GetFixedNodeSet());
    } else {
      node->CalculateLayout(GetFixedNodeSet());
    }

    GetLayoutInfoRecursively(result, node);
  }
  return result;
}

void LayoutContext::GetLayoutInfoRecursively(
    std::unordered_map<int32_t, LayoutInfoArray>& result, LayoutNode* node) {
  if (node == nullptr || node->slnode() == nullptr || !node->IsDirty()) return;
  node->MarkUpdated();
  const auto& layout_result = node->slnode()->GetLayoutResult();
  LayoutInfoArray layout_info;

  layout_info[LayoutInfo::kWidth] = layout_result.size_.width_;
  layout_info[LayoutInfo::kHeight] = layout_result.size_.height_;
  layout_info[LayoutInfo::kLeft] = layout_result.offset_.Y();
  layout_info[LayoutInfo::kTop] = layout_result.offset_.X();

  layout_info[LayoutInfo::kPaddingLeft] =
      layout_result.padding_[starlight::kLeft];
  layout_info[LayoutInfo::kPaddingTop] =
      layout_result.padding_[starlight::kTop];
  layout_info[LayoutInfo::kPaddingRight] =
      layout_result.padding_[starlight::kRight];
  layout_info[LayoutInfo::kPaddingBottom] =
      layout_result.padding_[starlight::kBottom];

  layout_info[LayoutInfo::kMarginLeft] =
      layout_result.margin_[starlight::kLeft];
  layout_info[LayoutInfo::kMarginTop] = layout_result.margin_[starlight::kTop];
  layout_info[LayoutInfo::kMarginRight] =
      layout_result.margin_[starlight::kRight];
  layout_info[LayoutInfo::kMarginBottom] =
      layout_result.margin_[starlight::kBottom];

  if (node->is_list_container()) {
    layout_info[LayoutInfo::kIsUpdatedListElement] = 1.0;
  } else {
    layout_info[LayoutInfo::kIsUpdatedListElement] = 0.0;
  }
  result.emplace(node->id(), std::move(layout_info));
  if (!node->children().empty()) {
    for (auto& child : node->children()) {
      GetLayoutInfoRecursively(result, child);
    }
  }
}

starlight::Constraints LayoutContext::ConvertViewportToOneSideConstraint(
    Viewport viewport) {
  SLMeasureMode width_mode;
  SLMeasureMode height_mode;
  switch (viewport.width_mode) {
    case MeasureMode::Indefinite:
      width_mode = SLMeasureModeIndefinite;
      break;
    case MeasureMode::Definite:
      width_mode = SLMeasureModeDefinite;
      break;
    case MeasureMode::AtMost:
      width_mode = SLMeasureModeAtMost;
      break;
    default:
      width_mode = SLMeasureModeIndefinite;
      break;
  };
  switch (viewport.height_mode) {
    case MeasureMode::Indefinite:
      height_mode = SLMeasureModeIndefinite;
      break;
    case MeasureMode::Definite:
      height_mode = SLMeasureModeDefinite;
      break;
    case MeasureMode::AtMost:
      height_mode = SLMeasureModeAtMost;
      break;
    default:
      height_mode = SLMeasureModeIndefinite;
      break;
  }
  starlight::Constraints constraints;
  constraints[starlight::kHorizontal] =
      starlight::OneSideConstraint(viewport.width, width_mode);
  constraints[starlight::kVertical] =
      starlight::OneSideConstraint(viewport.height, height_mode);
  return constraints;
}

bool LayoutContext::IfNeedsUpdateLayoutInfo(LayoutNode* node) {
  if (node == nullptr || node->slnode() == nullptr) {
    return false;
  }

  if (node->slnode()->GetHasNewLayout()) {
    // common nodes and no-parent nodes must layouted by C++, layout results are
    // meaningful
    if (node->is_common() || node->parent() == nullptr) {
      return true;
    } else {
      // otherwise, whether a node is layouted by C++ depends on its parent node
      // of layout_object.
      auto parent_of_layout_object_tree =
          node->parent()->FindNonVirtualNode()->slnode();
      return !parent_of_layout_object_tree->GetSLMeasureFunc() ||
             node->is_inline_view();
    }
  }
  return false;
}

}  // namespace tasm
}  // namespace lynx
