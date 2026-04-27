// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_BASE_VIEW_EXTRA_DATA_H_
#define CLAY_UI_COMPONENT_BASE_VIEW_EXTRA_DATA_H_

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "clay/gfx/animation/animation_data.h"
#include "clay/gfx/animation/keyframes_manager.h"
#include "clay/gfx/animation/transition_manager.h"
#include "clay/gfx/geometry/float_point.h"
#include "clay/gfx/geometry/path.h"
#include "clay/gfx/geometry/sticky_info.h"
#include "clay/gfx/geometry/transform_origin.h"
#include "clay/gfx/geometry/transform_raw.h"
#include "clay/public/value.h"
#include "clay/ui/component/base_view_animation_mutator.h"
#include "clay/ui/component/mouse_cursor.h"
#include "clay/ui/event/gesture_event.h"
#include "clay/ui/gesture/gesture_recognizer.h"

namespace clay {

class BaseView;

// Fields that are optional for most nodes can be stored behind a single
// lazily-allocated pointer on BaseView, then further split into per-capability
// sub-packs so nodes only pay for the features they actually use.
struct BaseViewMetadataExtra {
  std::string id_selector;
  std::string ref_id_selector;
  std::string name;
  std::string item_key;
  clay::Value data_set = clay::Value(clay::Value::Map());
  std::optional<bool> ignore_focus;
};

struct BaseViewAnimationExtra {
  std::unique_ptr<KeyframesManager> keyframes_mgr;
  std::unique_ptr<TransitionManager> transition_mgr;
  std::optional<std::vector<AnimationData>> animation;
  std::unique_ptr<BaseViewAnimationMutator> animation_mutator;
  std::optional<TransformOrigin> transform_origin;
  std::optional<std::vector<TransformRaw>> transform_raw;
  std::optional<float> perspective_value;
};

struct BaseViewEventExtra {
  std::optional<std::vector<std::string>> events;
  std::vector<std::unique_ptr<GestureRecognizer>> gesture_recognizers;
  std::optional<bool> event_through;
  std::vector<std::pair<float, float>> consume_slide_event_ranges;
  std::function<void(BaseView*)> destruct_listener = nullptr;
};

struct BaseViewVisualExtra {
  std::optional<ClipPathData> clip_path_data;
  std::optional<OffsetPathData> offset_path_data;
  std::optional<StickyInfo> sticky = std::nullopt;
  std::optional<FloatPoint> post_translation;
  std::unique_ptr<MouseCursor> cursor = nullptr;
};

struct BaseViewPlatformExtra {
  std::string app_region;
  PointerEvent event_draggable{};
  bool can_draggable = false;
};

struct BaseViewExtraData {
  std::unique_ptr<BaseViewMetadataExtra> metadata;
  std::unique_ptr<BaseViewAnimationExtra> animation;
  std::unique_ptr<BaseViewEventExtra> event;
  std::unique_ptr<BaseViewVisualExtra> visual;
  std::unique_ptr<BaseViewPlatformExtra> platform;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_BASE_VIEW_EXTRA_DATA_H_
