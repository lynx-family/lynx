// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_IMPERATIVE_ANIMATION_STATE_H_
#define CORE_RENDERER_DOM_IMPERATIVE_ANIMATION_STATE_H_

#include <cstdint>

#include "base/include/value/base_string.h"
#include "base/include/vector.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/css_property_bitset.h"
#include "core/renderer/starlight/style/css_type.h"

namespace lynx {
namespace starlight {
class ComputedCSSStyle;
}  // namespace starlight

namespace tasm {

class CSSKeyframesToken;

class ImperativeAnimationState {
 public:
  enum class Source : uint8_t {
    kAnimate,
    kAnimateV2,
  };

  struct Mutation {
    CSSIDBitset cleanup_properties;
    base::Vector<base::String> keyframes_to_remove;
  };

  Mutation RecordStart(Source source, const base::String& js_name,
                       const base::String& animation_name,
                       bool owns_generated_keyframe,
                       const StyleMap& timing_styles,
                       CSSKeyframesToken* keyframes_token);
  void UpdatePlayState(Source source, const base::String& name,
                       const StyleMap& timing_styles, bool paused);
  Mutation Cancel(Source source, const base::String& name);
  Mutation Finish(Source source, const base::String& name);
  Mutation ClearForStyleAnimationUpdate();
  Mutation Clear();

  void ReplayToStyle(starlight::ComputedCSSStyle& computed_style) const;
  CSSIDBitset TakePendingCleanupProperties();
  bool HasRecords() const { return !records_.empty(); }
  bool HasAnimationName(const base::String& animation_name) const;

 private:
  struct Record {
    Source source{Source::kAnimate};
    base::String js_name;
    base::String animation_name;
    StyleMap timing_styles;
    CSSIDBitset affected_properties;
    starlight::AnimationFillModeType fill_mode{
        starlight::AnimationFillModeType::kNone};
    bool owns_generated_keyframe{false};
  };

  static bool MatchesName(const Record& record, const base::String& name);
  static bool MatchesIdentity(const Record& record, Source source,
                              const base::String& js_name,
                              const base::String& animation_name);
  static bool ShouldReplaceOnStart(const Record& record, Source source,
                                   const base::String& js_name,
                                   const base::String& animation_name);

  void QueueCleanup(const CSSIDBitset& properties, Mutation& mutation);
  void QueueOwnedKeyframeRemoval(const Record& record, Mutation& mutation);

  base::Vector<Record> records_;
  CSSIDBitset pending_cleanup_properties_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_IMPERATIVE_ANIMATION_STATE_H_
