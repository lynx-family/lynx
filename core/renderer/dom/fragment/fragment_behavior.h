// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FRAGMENT_FRAGMENT_BEHAVIOR_H_
#define CORE_RENDERER_DOM_FRAGMENT_FRAGMENT_BEHAVIOR_H_

#include "base/include/fml/memory/ref_ptr.h"
#include "core/public/prop_bundle.h"
#include "core/renderer/dom/fragment/display_list_builder.h"
#include "core/renderer/dom/fragment/layout_info.h"
#include "core/renderer/ui_wrapper/painting/native_painting_context.h"

namespace lynx::tasm {
class PaintingContext;
class Fragment;
// FragmentBehavior provides platform-specific rendering behaviors for
// Fragments.
//
// IMPORTANT: Resource Cleanup Policy
// ----------------------------
// Do NOT implement resource cleanup in destructors of subclasses. The
// destructor is marked 'final' to enforce this policy.
//
// WHY: FragmentBehavior holds raw pointers to painting_context_ and fragment_,
// which are owned by ElementManager -> Catalyzer -> PaintingContext. During
// ElementManager destruction, Catalyzer is destroyed BEFORE all Elements/
// Fragments are destroyed. This creates a use-after-free if subclasses try to
// access painting_context_ in their destructors.
//
// CORRECT APPROACH: Override OnElementDestroying() to release all platform
// resources (native views, text bundles, etc.). This method is called by
// Fragment::Destroy() while ElementManager and PaintingContext are still alive
// and all pointers remain valid.
//
// Example:
//   class MyBehavior : public FragmentBehavior {
//    public:
//     void OnElementDestroying() override {
//       if (painting_context_) {
//         painting_context_->DestroyMyResource(fragment_->id());
//       }
//     }
//   };
//
class FragmentBehavior {
 public:
  explicit FragmentBehavior(Fragment* fragment);
  // The destructor is 'final' to prevent subclasses from defining their own.
  // This ensures all resource cleanup happens in OnElementDestroying() where
  // painting_context_ and fragment_ are guaranteed to be valid.
  virtual ~FragmentBehavior() {}

  void Destroy();

  // Called by Fragment::Destroy() when the element is being destroyed.
  //
  // This is the ONLY safe place to release platform resources (native views,
  // text bundles, image resources, etc.). At this point, ElementManager and
  // PaintingContext are still alive, so painting_context_ and fragment_
  // pointers remain valid.
  //
  // DO NOT wait until the destructor to clean up resources - by then the
  // PaintingContext may have already been destroyed, causing use-after-free
  // crashes when trying to access painting_context_.
  //
  // Override this method in subclasses to perform cleanup:
  //   void OnElementDestroying() override {
  //     if (painting_context_ && has_resource_) {
  //       painting_context_->DestroyResource(fragment_->id());
  //       has_resource_ = false;
  //     }
  //   }
  virtual void OnElementDestroying() {}

  virtual void CreatePlatformRenderer(
      const fml::RefPtr<PropBundle>& attributes);
  // TODO(zhongyr): TO be implemented for basic <view>.
  virtual void OnAttributeUpdate(const fml::RefPtr<PropBundle>& attributes){};

  virtual void OnDraw(DisplayListBuilder& display_list_builder){};

  virtual void OnUpdateLayout(const LayoutInfoForDraw& layout_result) {}

  Fragment* fragment() { return fragment_; }

  NativePaintingContext* painting_context() { return painting_context_; }

  virtual void SetTextBundle(intptr_t bundle) {}

 protected:
  // Used for other painting related operations.
  Fragment* fragment_;
  NativePaintingContext* painting_context_;
};
}  // namespace lynx::tasm

#endif  // CORE_RENDERER_DOM_FRAGMENT_FRAGMENT_BEHAVIOR_H_
