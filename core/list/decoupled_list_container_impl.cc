// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/list/decoupled_list_container_impl.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

#include "base/trace/native/trace_event.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/list/decoupled_batch_list_adapter.h"
#include "core/list/decoupled_default_list_adapter.h"
#include "core/list/decoupled_grid_layout_manager.h"
#include "core/list/decoupled_linear_layout_manager.h"
#include "core/list/decoupled_staggered_grid_layout_manager.h"
#include "core/renderer/trace/renderer_trace_event_def.h"

namespace lynx {
namespace list {

namespace {

bool ReadStageDuration(const pub::Value& value, int32_t& duration_ms) {
  if (value.IsNumber()) {
    const double number = value.Number();
    if (std::isfinite(number)) {
      // Store integer milliseconds: clamp negatives, truncate fractions, and
      // check range before conversion.
      const double milliseconds = std::max(0.0, std::trunc(number));
      if (milliseconds <= std::numeric_limits<int32_t>::max()) {
        duration_ms = static_cast<int32_t>(milliseconds);
        return true;
      }
    }
  }
  return false;
}

bool HasValidStageDurations(const std::vector<AnimationStageEntries>& stages) {
  int64_t total_duration_ms = 0;
  for (const auto& stage : stages) {
    int32_t stage_duration_ms = 0;
    for (const auto& entry : stage) {
      stage_duration_ms = std::max(stage_duration_ms, entry.duration_ms);
    }
    total_duration_ms += stage_duration_ms;
    if (total_duration_ms > std::numeric_limits<int32_t>::max()) {
      return false;
    }
  }
  return true;
}

/*
 Parse animation configuration, for example:
 (1) A single stage runs all animations concurrently.
 [{ animations: ['remove', 'move', 'add', 'change'], durations: 200 }]
 (2) Three stages, with move and change running concurrently for
 100ms and 200ms, respectively.
 [
   { animations: ['remove'], durations: 120 },
   { animations: ['move', 'change'], durations: [100, 200] },
   { animations: ['add'], durations: 200 },
 ]
 */
bool ParseUpdateAnimationStages(const pub::Value& value,
                                std::vector<AnimationStageEntries>& stages) {
  if (!value.IsArray() || value.Length() < 1 || value.Length() > 4) {
    return false;
  }
  constexpr uint8_t kRemove = 1;
  constexpr uint8_t kMove = 2;
  constexpr uint8_t kAdd = 4;
  constexpr uint8_t kChange = 8;
  uint8_t parsed_animation_types_mask = 0;

  // Track parsed types across stages to prevent duplicate scheduling.
  auto parse_animations = [&parsed_animation_types_mask](
                              const pub::Value& animations,
                              AnimationStageEntries& entries) {
    if (!animations.IsArray() || animations.Length() < 1 ||
        animations.Length() > 4) {
      return false;
    }
    for (int j = 0; j < animations.Length(); ++j) {
      auto name = animations.GetValueAtIndex(static_cast<uint32_t>(j));
      if (!name || !name->IsString()) {
        return false;
      }
      ItemAnimationType type;
      uint8_t bit = 0;
      if (name->str() == kUpdateAnimationTypeRemove) {
        type = ItemAnimationType::kDisappearance;
        bit = kRemove;
      } else if (name->str() == kUpdateAnimationTypeMove) {
        type = ItemAnimationType::kPersistence;
        bit = kMove;
      } else if (name->str() == kUpdateAnimationTypeAdd) {
        type = ItemAnimationType::kAppearance;
        bit = kAdd;
      } else if (name->str() == kUpdateAnimationTypeChange) {
        type = ItemAnimationType::kChange;
        bit = kChange;
      } else {
        return false;
      }
      if ((parsed_animation_types_mask & bit) != 0) {
        return false;
      }
      parsed_animation_types_mask |= bit;
      entries.push_back({type, 0});
    }
    return true;
  };
  auto parse_durations = [](const pub::Value& durations,
                            AnimationStageEntries& entries) {
    if (durations.IsNumber()) {
      // A numeric duration applies to every entry in the stage.
      int32_t duration_ms = 0;
      if (!ReadStageDuration(durations, duration_ms)) {
        return false;
      }
      for (auto& entry : entries) {
        entry.duration_ms = duration_ms;
      }
      return true;
    } else if (durations.IsArray() &&
               durations.Length() == static_cast<int>(entries.size())) {
      // Array durations correspond to entries by index.
      for (size_t j = 0; j < entries.size(); ++j) {
        auto duration = durations.GetValueAtIndex(static_cast<uint32_t>(j));
        if (!duration ||
            !ReadStageDuration(*duration, entries[j].duration_ms)) {
          return false;
        }
      }
      return true;
    }
    return false;
  };

  std::vector<AnimationStageEntries> parsed_stages;
  for (int i = 0; i < value.Length(); ++i) {
    auto stage = value.GetValueAtIndex(static_cast<uint32_t>(i));
    if (!stage || !stage->IsMap()) {
      return false;
    }
    auto animations = stage->GetValueForKey(kUpdateAnimationStageAnimations);
    auto durations = stage->GetValueForKey(kUpdateAnimationStageDurations);
    AnimationStageEntries entries;
    if (!animations || !durations || !parse_animations(*animations, entries) ||
        !parse_durations(*durations, entries)) {
      return false;
    }
    parsed_stages.push_back(std::move(entries));
  }
  if (parsed_animation_types_mask == 0 ||
      !HasValidStageDurations(parsed_stages)) {
    return false;
  }
  // Replace stages only after the entire configuration passes parsing and
  // validation.
  stages = std::move(parsed_stages);
  return true;
}

UpdateAnimationConfig ParseUpdateAnimationConfig(const pub::Value& value) {
  // Start with defaults, then apply valid user configuration.
  UpdateAnimationConfig config;
  config.stages = MakeDefaultAnimationStages();
  if (auto enable = value.GetValueForKey(kUpdateAnimationConfigEnable);
      enable && enable->IsBool()) {
    config.enable = enable->Bool();
  }
  if (value.Contains(kUpdateAnimationConfigStages)) {
    auto stages = value.GetValueForKey(kUpdateAnimationConfigStages);
    if (!stages || !ParseUpdateAnimationStages(*stages, config.stages)) {
      DLIST_LOGE("Invalid update-animation stages; using default scheduling.");
    }
  }
  return config;
}

}  // namespace

ListContainerImpl::ListContainerImpl(
    ElementDelegate* list_delegate,
    const std::shared_ptr<pub::PubValueFactory>& value_factory)
    : list_delegate_(list_delegate),
      list_layout_manager_(std::make_unique<LinearLayoutManager>(this)),
      list_adapter_(std::make_unique<DefaultListAdapter>(this)),
      list_children_helper_(std::make_unique<ListChildrenHelper>()),
      list_event_manager_(std::make_unique<ListEventManager>(this)),
      list_animation_manager_(CreateListAnimationManager(this)),
      animation_manager_(CreateAnimationManager(this)),
      value_factory_(value_factory) {
  DLIST_LOGI("ListContainerImpl::ListContainerImpl() this=" << this);
  list_layout_manager_->InitLayoutManager(list_children_helper_.get(),
                                          Orientation::kVertical);
  if (!list_delegate->IsAttachToElementManager()) {
    return;
  }
  physical_pixels_per_layout_unit_ =
      list_delegate_->GetPhysicalPixelsPerLayoutUnit();
  if (base::FloatsEqual(physical_pixels_per_layout_unit_, 0.f)) {
    physical_pixels_per_layout_unit_ = 1.f;
  }
}

ListContainerImpl::~ListContainerImpl() {
  if (animation_manager_) {
    // Detach animation callbacks and clear transaction-owned resources before
    // the other container members are destroyed. This prevents an animator
    // from calling back after dependencies such as ListAdapter and
    // ListChildrenHelper become invalid.
    animation_manager_->Destroy();
  }
  DLIST_LOGI("ListContainerImpl::~ListContainerImpl this=" << this);
}

void ListContainerImpl::OnAttachToElementManager() {
  physical_pixels_per_layout_unit_ =
      list_delegate_->GetPhysicalPixelsPerLayoutUnit();
  if (base::FloatsEqual(physical_pixels_per_layout_unit_, 0.f)) {
    physical_pixels_per_layout_unit_ = 1.f;
  }
}

void ListContainerImpl::FinishBindItemHolder(
    ItemElementDelegate* list_item_delegate,
    const std::shared_ptr<tasm::PipelineOptions>& options) {
  if (list_adapter_) {
    list_adapter_->OnFinishBindItemHolder(list_item_delegate, options);
  }
}

void ListContainerImpl::FinishBindItemHolders(
    const std::vector<ItemElementDelegate*>& list_item_delegate_array,
    const std::shared_ptr<tasm::PipelineOptions>& options) {
  if (list_adapter_) {
    list_adapter_->OnFinishBindItemHolders(list_item_delegate_array, options);
  }
}

void ListContainerImpl::OnNextFrame() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LIST_CONTAINER_ON_NEXT_FRAME);
  list_layout_manager_->PreloadSection();
  need_preload_section_on_next_frame_ = false;
}

void ListContainerImpl::OnListItemLayoutUpdated(
    ItemElementDelegate* list_item_delegate) {
  if (list_item_delegate) {
    const auto& attached_delegate_item_holder_map =
        list_children_helper_->attached_delegate_item_holder_map();
    if (auto it = attached_delegate_item_holder_map.find(list_item_delegate);
        it != attached_delegate_item_holder_map.end()) {
      list_adapter_->UpdateLayoutInfoToItemHolder(list_item_delegate,
                                                  it->second);
    }
  }
}

float ListContainerImpl::RoundValueToPixelGrid(const float value) {
  return std::roundf(value * physical_pixels_per_layout_unit_) /
         physical_pixels_per_layout_unit_;
}

// Get count of data source.
int ListContainerImpl::GetDataCount() const {
  return list_adapter_ ? list_adapter_->GetDataCount() : 0;
}

// Get the ItemHolder for the specified index.
ItemHolder* ListContainerImpl::GetItemHolderForIndex(int index) {
  return list_adapter_ ? list_adapter_->GetItemHolderForIndex(index) : nullptr;
}

// Flush all children's layout info patching to plaform.
void ListContainerImpl::FlushPatching() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LIST_CONTAINER_FLUSH_PATCHING);
  bool should_flush_finish_layout = should_flush_finish_layout_;
  should_flush_finish_layout_ = false;
  list_delegate_->FlushPatching(should_flush_finish_layout);
}

// Update content offset and size to platform view.
void ListContainerImpl::UpdateContentOffsetAndSizeToPlatform(
    float content_size, float delta_x, float delta_y,
    bool is_init_scroll_offset, bool from_layout) {
  list_delegate_->UpdateContentOffsetAndSizeToPlatform(
      content_size, delta_x, delta_y, is_init_scroll_offset, from_layout);
}

// Update scroll info to platform view.
void ListContainerImpl::UpdateScrollInfo(float estimated_offset, bool smooth,
                                         bool scrolling) {
  if (smooth) {
    list_delegate_->UpdateScrollInfo(estimated_offset, smooth, scrolling);
  }
}

// This function should be called before any code that may trigger list's
// OnListElementUpdated() to enable list avoid reacting to additional redundant
// OnListElementUpdated() calls.
void ListContainerImpl::StartInterceptListElementUpdated() {
  intercept_depth_++;
}

// This method should be called after any code that may trigger list's
// OnListElementUpdated().
void ListContainerImpl::StopInterceptListElementUpdated() {
  if (intercept_depth_ < 1) {
    intercept_depth_ = 1;
  }
  intercept_depth_--;
}

void ListContainerImpl::UpdateListLayoutManager(LayoutType layout_type) {
  int span_count = list_layout_manager_->span_count();
  Orientation orientation = list_layout_manager_->orientation();
  float main_axis_gap = list_layout_manager_->main_axis_gap();
  float cross_axis_gap = list_layout_manager_->cross_axis_gap();
  float preload_buffer_count = list_layout_manager_->preload_buffer_count();
  float content_size = list_layout_manager_->content_size();
  // Store the previous content_offset_ or the delta calculation may be
  // incorrect
  float content_offset = list_layout_manager_->content_offset();
  bool enable_preload_section = list_layout_manager_->enable_preload_section();
  if (layout_type == LayoutType::kSingle) {
    list_layout_manager_ = std::make_unique<LinearLayoutManager>(this);
  } else if (layout_type == LayoutType::kFlow) {
    list_layout_manager_ = std::make_unique<GridLayoutManager>(this);
  } else if (layout_type == LayoutType::kWaterFall) {
    list_layout_manager_ = std::make_unique<StaggeredGridLayoutManager>(this);
  }
  list_layout_manager_->InitLayoutManager(list_children_helper_.get(),
                                          orientation);
  list_layout_manager_->SetSpanCount(span_count);
  list_layout_manager_->SetMainAxisGap(main_axis_gap);
  list_layout_manager_->SetCrossAxisGap(cross_axis_gap);
  list_layout_manager_->ResetContentOffsetAndContentSize(content_offset,
                                                         content_size);
  list_layout_manager_->SetPreloadBufferCount(preload_buffer_count);
  list_layout_manager_->SetEnablePreloadSection(enable_preload_section);
  list_adapter_->OnDataSetChanged();
  need_recycle_all_item_holders_before_layout_ = true;
}

bool ListContainerImpl::ResolveAttribute(const pub::Value& key,
                                         const pub::Value& value) {
  if (!key.IsString()) {
    DLIST_LOGE("[" << this
                   << "] ListContainerImpl::ResolveAttribute: non string key");
    return true;
  }
  const std::string& key_str = key.str();
  // TODO(dingwang.wxx): using more save trace event.
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LIST_CONTAINER_RESOLVE_ATTRIBUTE, "key",
              key_str.c_str());
  bool should_set_props = true;
  bool should_mark_layout_dirty = false;
  bool should_cancel_animation = false;
  if (key_str == kPropCustomListName && value.IsString()) {
    // custom-list-container
    if (value.str() == kPropValueListContainer) {
      list_delegate_->UpdateListLayoutNodeAttribute();
    }
  } else if (key_str == kPropVerticalOrientation) {
    // vertical-orientation
    // TODO: @deprecated vertical-orientation
    Orientation orientation = Orientation::kVertical;
    if (value.IsBool()) {
      orientation =
          value.Bool() ? Orientation::kVertical : Orientation::kHorizontal;
    } else if (value.IsString()) {
      orientation = value.str() == kPropValueTrue ? Orientation::kVertical
                                                  : Orientation::kHorizontal;
      std::string error_msg = "Error type for attribute vertical-orientation";
      std::string suggestion =
          "Please use bool value or use scroll-orientation";
      auto error = lynx::base::LynxError(
          error::E_COMPONENT_LIST_INVALID_PROPS_ARG, std::move(error_msg),
          std::move(suggestion), base::LynxErrorLevel::Warn);
      list_delegate_->OnErrorOccurred(std::move(error));
    }
    should_cancel_animation =
        list_layout_manager_->orientation() != orientation;
    list_layout_manager_->SetOrientation(orientation);
    list_layout_manager_->CreateOrUpdateListAnchorManager();
  } else if (key_str == kPropScrollOrientation && value.IsString()) {
    // scroll-orientation
    // Note: if value is any illegal string, using vertical by default.
    Orientation orientation =
        value.str() == kPropValueScrollOrientationHorizontal
            ? Orientation::kHorizontal
            : Orientation::kVertical;
    should_cancel_animation =
        list_layout_manager_->orientation() != orientation;
    list_layout_manager_->SetOrientation(orientation);
    list_layout_manager_->CreateOrUpdateListAnchorManager();
  } else if (key_str == kPropEnableDynamicSpanCount && value.IsBool()) {
    // enable-dynamic-span-count
    enable_dynamic_span_count_ = value.Bool();
    should_set_props = false;
  } else if ((key_str == kPropSpanCount || key_str == kPropColumnCount) &&
             value.IsNumber()) {
    // span-count / column-count
    // TODO: @deprecated column-count
    int span_count = static_cast<int>(value.Number());
    if (span_count <= 0) {
      span_count = 1;
    }
    if (list_layout_manager_->span_count() != span_count) {
      should_cancel_animation = true;
      span_count_changed_ = true;
    }
    list_layout_manager_->SetSpanCount(span_count);
    should_mark_layout_dirty = true;
    should_set_props = false;
  } else if (key_str == kPropAnchorPriority && value.IsString()) {
    // anchor-priority
    list_layout_manager_->SetAnchorPriorityFromBegin(
        value.str() == kPropValueAnchorPriorityFromBegin);
    should_set_props = false;
  } else if (key_str == kPropAnchorAlign && value.IsString()) {
    // anchor-align
    list_layout_manager_->SetAnchorAlignToBottom(value.str() ==
                                                 kPropValueAnchorAlignToBottom);
    should_set_props = false;
  } else if (key_str == kPropAnchorVisibility && value.IsString()) {
    // anchor-visibility
    const std::string& value_str = value.str();
    if (value_str == kPropValueAnchorVisibilityHide) {
      list_layout_manager_->SetAnchorVisibility(
          AnchorVisibility::kAnchorVisibilityHide);
    } else if (value_str == kPropValueAnchorVisibilityShow) {
      list_layout_manager_->SetAnchorVisibility(
          AnchorVisibility::kAnchorVisibilityShow);
    } else {
      list_layout_manager_->SetAnchorVisibility(
          AnchorVisibility::kAnchorVisibilityNoAdjustment);
    }
    should_set_props = false;
  } else if ((key_str == kPropRadonListPlatformInfo ||
              key_str == kPropFiberUpdateListInfo) &&
             value.IsMap()) {
    // list-platform-info / update-list-info
    std::pair<ListAdapterDiffResult, bool> result;
    if (key_str == kPropRadonListPlatformInfo) {
      result = list_adapter_->UpdateRadonDataSource(value);
    } else if (key_str == kPropFiberUpdateListInfo) {
      result = list_adapter_->UpdateFiberDataSource(value);
    }
    if (result.first != ListAdapterDiffResult::kNone) {
      should_mark_layout_dirty = true;
    }
    has_valid_diff_ = should_mark_layout_dirty;
    need_preload_section_on_next_frame_ = should_mark_layout_dirty;
    if (should_mark_layout_dirty) {
      list_layout_manager_->UpdateDiffAnchorReference();
    }
    should_set_props = false;
    need_update_item_holders_ = true;
    animation_diff_result_ =
        result.second ? result.first : ListAdapterDiffResult::kNone;
  } else if (key_str == kPropUpdateAnimation && value.IsString()) {
    // update-animation
    update_animation_ = value.str() == kPropValueUpdateAnimationDefault;
    should_set_props = false;
  } else if (key_str == kPropExperimentalUseNewUpdateAnimation &&
             value.IsBool()) {
    // Select the implementation when this property is parsed for the first
    // time. The new pipeline does not yet support batch rendering or parallel
    // element flushing, so either mode forces the selection to false if it is
    // already enabled at that point. Ignore subsequent dynamic updates once
    // the selection is made.
    if (!use_new_update_animation_.has_value()) {
      // TODO: Support the new update animation with batch rendering and
      // parallel element flushing.
      use_new_update_animation_ =
          value.Bool() && !enable_batch_render() && !enable_parallel_element();
    }
    should_set_props = false;
  } else if (key_str == kPropExperimentalUpdateAnimation && value.IsMap()) {
    // Parse and cache the complete configuration, then apply it from
    // PropsUpdateFinish after the new pipeline has been selected. This makes
    // the result independent of the order in which the selection and
    // configuration properties are parsed.
    new_update_animation_config_ = ParseUpdateAnimationConfig(value);
    should_set_props = false;
  } else if (key_str == kPropListType && value.IsString()) {
    // list-type
    LayoutType last_layout_type = layout_type_;
    const std::string& value_str = value.str();
    if (value_str == kPropValueListTypeSingle) {
      layout_type_ = LayoutType::kSingle;
    } else if (value_str == kPropValueListTypeFlow) {
      layout_type_ = LayoutType::kFlow;
    } else if (value_str == kPropValueListTypeWaterFall) {
      layout_type_ = LayoutType::kWaterFall;
    } else {
      layout_type_ = LayoutType::kSingle;
    }
    if (layout_type_ != last_layout_type) {
      // Replacing the LayoutManager changes coordinate calculation and
      // recycling behavior, so an existing transaction cannot continue
      // across the replacement.
      should_cancel_animation = true;
      UpdateListLayoutManager(layout_type_);
    }
    should_mark_layout_dirty = true;
    should_set_props = false;
  } else if (key_str == kPropInitialScrollIndex && value.IsNumber()) {
    // initial-scroll-index
    initial_scroll_index_ = static_cast<int>(value.Number());
    should_set_props = false;
  } else if (key_str == kPropUpperThresholdItemCount && value.IsNumber()) {
    // upper-threshold-item-count
    list_event_manager_->SetUpperThresholdItemCount(
        static_cast<int>(value.Number()));
    should_set_props = false;
  } else if (key_str == kPropLowerThresholdItemCount && value.IsNumber()) {
    // lower-threshold-item-count
    list_event_manager_->SetLowerThresholdItemCount(
        static_cast<int>(value.Number()));
    should_set_props = false;
  } else if (key_str == kPropNeedLayoutCompleteInfo && value.IsBool()) {
    // need-layout-complete-info
    list_event_manager_->SetNeedLayoutCompleteInfo(value.Bool());
    should_set_props = false;
  } else if (key_str == kPropLayoutId && value.IsNumber()) {
    // layout-id
    layout_id_ = static_cast<int>(value.Number());
    should_set_props = false;
  } else if (key_str == kPropScrollEventThrottle && value.IsNumber()) {
    // scroll-event-throttle
    list_event_manager_->SetScrollEventThrottleMS(
        static_cast<int>(value.Number()));
    should_set_props = false;
  } else if ((key_str == kPropNeedsVisibleCells ||
              key_str == kPropNeedVisibleItemInfo) &&
             value.IsBool()) {
    // need-visible-item-info / needs-visible-cells
    // TODO: @deprecated needs-visible-cells
    list_event_manager_->SetVisibleCell(value.Bool());
  } else if (key_str == kPropShouldRequestStateRestore && value.IsBool()) {
    // should-request-state-restore
    should_request_state_restore_ = value.Bool();
    should_set_props = false;
  } else if (key_str == kPropStickyOffset && value.IsNumber()) {
    // sticky-offset
    sticky_offset_ = value.Number();
  } else if (key_str == kPropSticky && value.IsBool()) {
    // sticky
    sticky_enabled_ = value.Bool();
  } else if (key_str == kPropExperimentalRecycleStickyItem && value.IsBool()) {
    // experimental-recycle-sticky-item
    // TODO(dingwang.wxx): experimental prop, the default value is true in
    // release3.4.
    recycle_sticky_item_ = value.Bool();
  } else if (key_str == kPropStickyBufferCount && value.IsNumber()) {
    // sticky-buffer-count
    sticky_buffer_count_ = static_cast<int>(value.Number());
  } else if (key_str == kPropEnablePreloadSection && value.IsBool()) {
    // experimental-enable-preload-section
    list_layout_manager_->SetEnablePreloadSection(value.Bool());
    should_set_props = false;
  } else if (key_str == kPropPreloadBufferCount && value.IsNumber()) {
    // preload-buffer-count
    should_mark_layout_dirty = list_layout_manager_->SetPreloadBufferCount(
        static_cast<int>(value.Number()));
    should_set_props = false;
  } else if (key_str == kPropEnableInsertPlatformViewOperation &&
             value.IsBool()) {
    // enable-insert-platform-view-operation
    enable_insert_platform_view_operation_ = value.Bool();
  } else if (key_str == kPropExperimentalBatchRenderStrategy) {
    // experimental-batch-render-strategy
    // Note: If parse experimental-batch-render-strategy in list property, we
    // should block flush this property to platform because before parsing all
    // properties of list element, we has pushed this property to prop_bundle.
    should_set_props = false;
  } else if (key_str == kPropListDebugInfoLevel && value.IsNumber()) {
    // list-debug-info-level
    list_event_manager_->SetListDebugInfoLevel(
        std::min(ListDebugInfoLevel::kListDebugInfoLevelVerbose,
                 static_cast<ListDebugInfoLevel>(value.Number())));
    should_set_props = false;
  } else if (key_str == kPropExperimentalRecycleAvailableItemBeforeLayout &&
             value.IsBool()) {
    // experimental-recycle-available-item-before-layout
    recycle_available_item_before_layout_ = value.Bool();
    should_set_props = false;
  } else if (key_str == kPropExperimentalSearchRefAnchorStrategy &&
             value.IsNumber()) {
    // experimental-search-ref-anchor-strategy
    search_ref_anchor_strategy_ =
        static_cast<SearchRefAnchorStrategy>(static_cast<int>(value.Number()));
    should_set_props = false;
  }
  if (should_mark_layout_dirty) {
    list_delegate_->MarkListElementLayoutDirty();
  }
  if (should_cancel_animation && use_new_update_animation()) {
    animation_manager_->CancelAnimationTransaction(
        AnimationCancelReason::kLayoutInvalidated);
  }
  return should_set_props;
};

void ListContainerImpl::OnLayoutChildren(
    const std::shared_ptr<tasm::PipelineOptions>& options) {
  // TODO: Remove the legacy update-animation path.
  if (!use_new_update_animation()) {
    if (update_animation_ != list_animation_manager_->UpdateAnimation()) {
      list_animation_manager_->SetUpdateAnimation(update_animation_);
    }
    if (list_animation_manager_->UpdateAnimation() &&
        animation_diff_result_ != ListAdapterDiffResult::kNone) {
      list_animation_manager_->UpdateDiffResult(animation_diff_result_);
    }
    animation_diff_result_ = ListAdapterDiffResult::kNone;
  }

  if (list_layout_manager_) {
    if (options->need_timestamps) {
      list_delegate_->MarkTiming(ListTiming::kRenderChildrenStart);
    }
    if (need_recycle_all_item_holders_before_layout_) {
      if (use_new_update_animation()) {
        // Changes such as list-type or span-count invalidate the coordinate
        // system used by existing PRE and POST state. Cancel the transaction
        // before recycling every holder.
        animation_manager_->CancelAnimationTransaction(
            AnimationCancelReason::kLayoutInvalidated);
      }
      list_adapter_->RecycleAllItemHolders();
      // Note: if list-type is changed, we will recycle all items before layout,
      // so here need to clear last_binding_children set to make sure we can
      // calculate new insert items after layout.
      list_children_helper_->ClearLastBindingChildren();
      need_recycle_all_item_holders_before_layout_ = false;
    }
    if (intercept_depth_ == 0) {
      // Note: we should reset should_flush_finish_layout_ to
      // options->has_layout to make sure invoke FinishLayoutOperation() to
      // trigger layoutDidFinished lifecycle of all list's children.
      should_flush_finish_layout_ = options->has_layout;

      // The new pipeline consumes the transaction's PRE target snapshot from
      // OnPrepareForLayoutChildren. Skip the pre-layout enqueue optimization:
      // it could otherwise recycle platform nodes, detach ItemElementDelegate,
      // and prevent later animation updates from being submitted.
      if (!use_new_update_animation() &&
          recycle_available_item_before_layout_) {
        // Try to enqueue all available items before layout.
        list_adapter_->EnqueueElementsIfNeeded();
      }

      // 2. Layout Children
      if (!enable_batch_render()) {
        list_layout_manager_->OnLayoutChildren();
      } else {
        list_layout_manager_->OnBatchLayoutChildren();
      }
      // Record completion of the initial layout.
      has_completed_first_layout_ = true;
    }
    if (options->need_timestamps) {
      list_delegate_->MarkTiming(ListTiming::kRenderChildrenEnd);
      const float list_main_size = list_layout_manager_->main_axis_size();
      const float content_size = list_layout_manager_->content_size();
      if (GetDataCount() > 0 && base::FloatsLarger(list_main_size, 0.f) &&
          base::FloatsLargerOrEqual(content_size, list_main_size)) {
        list_delegate_->MarkTiming(ListTiming::kFullFillRenderChildrenEnd);
      }
    }
  }
}

void ListContainerImpl::PropsUpdateFinish() {
  // Handle initial-scroll-index attr.
  if ((initial_scroll_index_ >= 0 && initial_scroll_index_ < GetDataCount()) &&
      initial_scroll_index_status_ == InitialScrollIndexStatus::kUnset) {
    initial_scroll_index_status_ = InitialScrollIndexStatus::kSet;
    list_layout_manager_->SetInitialScrollIndex(initial_scroll_index_);
  }

  // If the selection property was not parsed before the first
  // PropsUpdateFinish, permanently select the legacy pipeline. An explicit
  // selection is already locked by its first ResolveAttribute call and is not
  // changed here.
  if (!use_new_update_animation_.has_value()) {
    use_new_update_animation_ = false;
  }

  if (use_new_update_animation()) {
    // Apply any pending configuration, including requests to disable
    // animations.
    if (new_update_animation_config_.has_value()) {
      DLIST_LOGI("[" << this
                     << "] ListContainerImpl::PropsUpdateFinish: "
                        "new_update_animation_config="
                     << new_update_animation_config_.value().ToString());
      animation_manager_->SetUpdateAnimationConfig(
          new_update_animation_config_.value());
      // Clear the pending config to avoid reapplying it on subsequent
      // PropsUpdateFinish calls.
      new_update_animation_config_.reset();
    }
  } else {
    // TODO: Remove the legacy update-animation path.
    // Handle update-animation attr.
    if (layout_type_ == LayoutType::kWaterFall) {
      // Consider the order of resolving list-type and update-animation is not
      // fixed, we should move this logic in PropsUpdateFinish().
      // TODO(dongjiajian): support update animation in waterfall.
      update_animation_ = false;
    }
    if (update_animation_ != list_animation_manager_->UpdateAnimation()) {
      list_delegate_->MarkListElementLayoutDirty();
    }
  }

  // Handle enable-dynamic-span-count attr and reset span_count_changed_.
  if (span_count_changed_) {
    span_count_changed_ = false;
    if (!enable_dynamic_span_count_) {
      // Note: OnDataSetChanged() should be invoked before
      // UpdateItemHolderToLatest() if needed.
      list_adapter_->OnDataSetChanged();
      need_recycle_all_item_holders_before_layout_ = true;
    }
  }

  // Record diff result in PropsUpdateFinish() because we need to parse
  // need-layout-complete-info.
  list_event_manager_->RecordDiffResultIfNeeded();
  list_event_manager_->SendDiffDebugEventIfNeeded();

  // Handle experimental-batch-render-strategy attr.
  // Note: need to move from DefaultListAdapter to BatchListAdapter before
  // invoke UpdateItemHolderToLatest().
  if (enable_batch_render() && !batch_adapter_initialized_) {
    // Move construct from DefaultListAdapter to BatchListAdapter.
    list_adapter_ =
        std::make_unique<BatchListAdapter>(std::move(*list_adapter_));
    // Note: set new list adapter to AnchorManager.
    list_layout_manager_->CreateOrUpdateListAnchorManager();
    batch_adapter_initialized_ = true;
  }

  // Update all item holders if needed.
  if (need_update_item_holders_) {
    list_adapter_->UpdateItemHolderToLatest(list_children_helper_.get());
    need_update_item_holders_ = false;
  }

  // Handle sticky-buffer-count attr.
  if (sticky_buffer_count_ > kInvalidItemCount) {
    if (!recycle_sticky_item_) {
      // Note: A valid sticky buffer count means need to recycle sticky
      // item.
      recycle_sticky_item_ = true;
    }
    list_children_helper_->SetCustomStickyItemHolderCapacity(
        sticky_buffer_count_);
  }
  list_children_helper_->SetRecycleStickyItem(recycle_sticky_item_);

  // Clear diff result.
  list_adapter_->ClearDiffResult();
}

void ListContainerImpl::ScrollByPlatformContainer(float content_offset_x,
                                                  float content_offset_y,
                                                  float original_x,
                                                  float original_y) {
  if (list_layout_manager_) {
    // reset should_flush_finish_layout_ flag to false.
    should_flush_finish_layout_ = false;
    list_layout_manager_->ScrollByPlatformContainer(
        content_offset_x, content_offset_y, original_x, original_y);
  }
}

void ListContainerImpl::ScrollToPosition(int index, float offset, int align,
                                         bool smooth) {
  if (list_layout_manager_) {
    list_layout_manager_->ScrollToPosition(index, offset, align, smooth);
  }
}

void ListContainerImpl::ScrollStopped() {
  if (list_layout_manager_) {
    list_layout_manager_->ScrollStopped();
  }
}

void ListContainerImpl::OnAttachedToElementManager() {
  physical_pixels_per_layout_unit_ =
      list_delegate_->GetPhysicalPixelsPerLayoutUnit();
  if (base::FloatsEqual(physical_pixels_per_layout_unit_, 0.f)) {
    physical_pixels_per_layout_unit_ = 1.f;
  }
}

void ListContainerImpl::ResolveListAxisGap(tasm::CSSPropertyID id, float gap) {
  bool should_cancel_animation = false;
  if (tasm::CSSPropertyID::kPropertyIDListMainAxisGap == id) {
    if (base::FloatsNotEqual(gap, list_layout_manager_->main_axis_gap())) {
      should_cancel_animation = true;
      list_layout_manager_->SetMainAxisGap(gap);
      list_delegate_->MarkListElementLayoutDirty();
    }
  } else if (tasm::CSSPropertyID::kPropertyIDListCrossAxisGap == id) {
    if (base::FloatsNotEqual(gap, list_layout_manager_->cross_axis_gap())) {
      should_cancel_animation = true;
      list_layout_manager_->SetCrossAxisGap(gap);
      list_delegate_->MarkListElementLayoutDirty();
    }
  }
  if (should_cancel_animation && use_new_update_animation()) {
    animation_manager_->CancelAnimationTransaction(
        AnimationCancelReason::kLayoutInvalidated);
  }
}

std::unique_ptr<ContainerDelegate> CreateListContainerDelegate(
    ElementDelegate* list_delegate,
    const std::shared_ptr<pub::PubValueFactory>& value_factory) {
  return std::make_unique<ListContainerImpl>(list_delegate, value_factory);
}

}  // namespace list
}  // namespace lynx
