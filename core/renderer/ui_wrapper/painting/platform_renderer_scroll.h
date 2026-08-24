// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_PLATFORM_RENDERER_SCROLL_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_PLATFORM_RENDERER_SCROLL_H_

#include <cstdint>
#include <functional>
#include <string>

#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/value/base_value.h"
#include "base/include/vector.h"
#include "core/renderer/starlight/types/layout_types.h"

namespace lynx {
namespace tasm {

enum class DisplayListOpType : int32_t;
class PropBundle;
class PlatformRendererImpl;

struct ScrollProps {
  bool is_vertical{true};
  bool enable_scroll{true};
  bool scroll_bar_enabled{false};
  float upper_threshold{0.f};
  float lower_threshold{0.f};
};

struct ScrollContentOffset {
  float x{0.f};
  float y{0.f};
};

class ContentInfo {
 public:
  void Reset() {
    children_offset.clear();
    width = 0.f;
    height = 0.f;
  }

  base::InlineVector<ScrollContentOffset, starlight::kChildrenInlineVectorSize>
      children_offset;
  float width{0.f};
  float height{0.f};
};

class PlatformRendererScroll {
 public:
  explicit PlatformRendererScroll();
  virtual ~PlatformRendererScroll();

 protected:
  void UpdateScrollAttributes(const fml::RefPtr<PropBundle>& scroll_attributes);

  void GenerateContentInfoFromDisplayList(
      const fml::RefPtr<PlatformRendererImpl>& target);

  virtual void OnPropsUpdated(const ScrollProps& scroll_props) {}

  virtual void OnContentInfoUpdated(const ContentInfo& content_info) {}

 private:
  void UpdateScrollAttributeInternal(const std::string& key,
                                     const lepus::Value& value);

  struct LayoutOffset {
    float x = 0.f;
    float y = 0.f;
  };

  struct FrameInfo {
    int id = -1;
    float left = 0.f;
    float top = 0.f;
    float width = 0.f;
    float height = 0.f;
    float content_width = 0.f;
    float content_height = 0.f;
    int depth = 0;
  };

  enum class WalkAction {
    kContinue,
    kSkipSubtree,
    kStop,
  };

  static bool WalkFramesRelativeToContent(
      const fml::RefPtr<PlatformRendererImpl>& renderer, float base_x,
      float base_y, int depth_base, bool skip_root_begin_offset,
      const std::function<WalkAction(const FrameInfo&, DisplayListOpType)>&
          visitor);

 private:
  ScrollProps scroll_props_;
  ContentInfo content_info_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_PLATFORM_RENDERER_SCROLL_H_
