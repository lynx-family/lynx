// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FRAGMENT_PLATFORM_EXTENDED_FRAGMENT_BEHAVIOR_H_
#define CORE_RENDERER_DOM_FRAGMENT_PLATFORM_EXTENDED_FRAGMENT_BEHAVIOR_H_

#include "core/renderer/dom/fragment/fragment_behavior.h"

namespace lynx::tasm {

class DisplayListBuilder;

// PlatformExtendedFragmentBehavior bridges tag names and attributes to the
// PlatformRenderer for extended platform components that need custom attribute
// handling and rendering
class PlatformExtendedFragmentBehavior : public FragmentBehavior {
 public:
  explicit PlatformExtendedFragmentBehavior(Fragment* fragment,
                                            const base::String& tag_name);

  void CreatePlatformRenderer(
      const fml::RefPtr<PropBundle>& attributes) override;

  // Get the tag name for this platform extended component
  const base::String& tag_name() const { return tag_name_; }

 private:
  base::String tag_name_;
};

}  // namespace lynx::tasm

#endif  // CORE_RENDERER_DOM_FRAGMENT_PLATFORM_EXTENDED_FRAGMENT_BEHAVIOR_H_
