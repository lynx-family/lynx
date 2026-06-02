// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_PLATFORM_RENDERER_DARWIN_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_PLATFORM_RENDERER_DARWIN_H_

#include "core/renderer/dom/fragment/display_list.h"
#include "core/renderer/ui_wrapper/painting/platform_renderer_impl.h"

#import <Lynx/LynxRendererHost.h>
#import <UIKit/UIKit.h>

@class LynxUIOwner;

namespace lynx {

namespace tasm {

class PlatformRendererContextDarwin;

// iOS specific implementation of PlatformRendererImpl.
class PlatformRendererDarwin : public PlatformRendererImpl {
 public:
  explicit PlatformRendererDarwin(PlatformRendererContextDarwin* context, int id,
                                  PlatformRendererType type);
  explicit PlatformRendererDarwin(PlatformRendererContextDarwin* context, int id,
                                  PlatformRendererType type,
                                  const fml::RefPtr<PropBundle>& init_data);
  explicit PlatformRendererDarwin(PlatformRendererContextDarwin* context, int id,
                                  const base::String& tag_name);
  explicit PlatformRendererDarwin(PlatformRendererContextDarwin* context, int id,
                                  const base::String& tag_name,
                                  const fml::RefPtr<PropBundle>& init_data);
  explicit PlatformRendererDarwin(PlatformRendererContextDarwin* context, int id,
                                  PlatformRendererType type, const base::String& tag_name);
  explicit PlatformRendererDarwin(PlatformRendererContextDarwin* context, int id,
                                  PlatformRendererType type, const base::String& tag_name,
                                  const fml::RefPtr<PropBundle>& init_data);

  ~PlatformRendererDarwin() override;

  // PlatformRendererImpl interface
  void OnUpdateDisplayList(DisplayList display_list) override;
  void OnUpdateAttributes(const fml::RefPtr<PropBundle>& attributes,
                          bool tends_to_flatten) override;
  void OnAddChild(PlatformRenderer* child) override;
  void OnRemoveFromParent() override;
  void OnUpdateSubtreeProperties(const DisplayList& subtree_properties) override;
  void UpdatePlatformExtraBundle(id platform_extra_bundle);

  void InitializeUIView();
  void InitializeUIView(const fml::RefPtr<PropBundle>& init_data);

  UIView<LynxRendererHost>* GetUIView() { return _view; }

 private:
  bool ShouldCreatePlatformExtendedRenderer(const fml::RefPtr<PropBundle>& init_data) const;
  bool InitializeUIOwnerRenderer(const base::String& tag_name,
                                 const fml::RefPtr<PropBundle>& init_data);
  void InitializeRendererForView(UIView<LynxRendererHost>* view, NSDictionary* initial_props = nil);
  void UpdateUIOwnerLayout(CGRect frame);
  bool HasUIOwnerNode(int sign) const;
  void CleanupUIView();

  UIView<LynxRendererHost>* _view = nil;

  PlatformRendererContextDarwin* context_ = nullptr;
  LynxUIOwner* ui_owner_ = nil;
};

}  // namespace tasm

}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_PLATFORM_RENDERER_DARWIN_H_
