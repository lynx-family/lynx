// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_IMPERATIVE_ANIMATION_METADATA_H_
#define CORE_RENDERER_DOM_IMPERATIVE_ANIMATION_METADATA_H_

#include "base/include/value/base_string.h"
#include "base/include/vector.h"
#include "core/renderer/dom/imperative_animation_source.h"

namespace lynx {
namespace tasm {

// Tracks only the identity needed to classify animations created by
// imperative animation APIs. Runtime animation state is owned elsewhere.
class ImperativeAnimationMetadata {
 public:
  // Records the identity of an animation before CSSKeyframeManager creates its
  // Animation object. Animate replaces its previous entry, while AnimateV2
  // replaces only an entry with the same identity.
  void RecordStart(ImperativeAnimationSource source,
                   const base::String& js_name,
                   const base::String& animation_name);

  // Removes entries matched by either their JS-facing name or final animation
  // name. Source is also matched so the two imperative APIs remain isolated.
  void Cancel(ImperativeAnimationSource source, const base::String& name);

  // Ends source tracking even when runtime fill state must remain. An Animation
  // object that has already been created keeps its Origin on the object itself.
  void Finish(ImperativeAnimationSource source, const base::String& name);

  // Clears all source metadata when animation data or the owning platform node
  // is discarded.
  void Clear();

  // Returns whether an active imperative entry owns |animation_name|. This is
  // queried only when a new Animation object needs its initial Origin.
  bool HasAnimationName(const base::String& animation_name) const;

 private:
  // Identity required to correlate an imperative API operation with the final
  // CSS animation name consumed by CSSKeyframeManager. Runtime timing and fill
  // state intentionally remain in ImperativeAnimationState.
  struct Record {
    // API variant that created this entry.
    ImperativeAnimationSource source{ImperativeAnimationSource::kAnimate};
    // Handle supplied by the JavaScript API and used by later operations.
    base::String js_name;
    // Final animation-name written into style and used to create Animation.
    base::String animation_name;
  };

  // Active entries that have started but have not been canceled, finished, or
  // invalidated by an owning Element lifecycle change.
  base::Vector<Record> records_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_IMPERATIVE_ANIMATION_METADATA_H_
