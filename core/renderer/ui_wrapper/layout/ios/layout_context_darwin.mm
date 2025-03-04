// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/ui_wrapper/layout/ios/layout_context_darwin.h"

#include "core/build/gen/lynx_sub_error_code.h"
#include "core/renderer/ui_wrapper/common/ios/platform_extra_bundle_darwin.h"
#include "core/renderer/ui_wrapper/common/ios/prop_bundle_darwin.h"

#import "LynxFontFaceManager.h"
#include "base/include/debug/lynx_assert.h"

namespace lynx {
namespace tasm {

LayoutContextDarwin::LayoutContextDarwin(LynxShadowNodeOwner* owner) : nodeOwner(owner) {}

LayoutContextDarwin::~LayoutContextDarwin() {}

int LayoutContextDarwin::CreateLayoutNode(int sign, const std::string& tag,
                                          PropBundle* painting_data, bool allow_inline) {
  PropBundleDarwin* pda = static_cast<PropBundleDarwin*>(painting_data);
  @try {
    return static_cast<int>([nodeOwner
             createNodeWithSign:sign
                        tagName:[[NSString alloc] initWithUTF8String:tag.c_str()]
                          props:pda ? pda->dictionary() : nullptr
                       eventSet:pda ? pda->event_set() : nil
                  lepusEventSet:pda ? pda->lepus_event_set() : nil
        isParentInlineContainer:allow_inline]);

  } @catch (NSException* e) {
    LynxWarning(e, error::E_LAYOUT_INTERNAL,
                [[NSString stringWithFormat:@"%@:%@", [e name], [e reason]] UTF8String]);
  }
  return 0;
}

void LayoutContextDarwin::InsertLayoutNode(int parent, int child, int index) {
  @try {
    [nodeOwner insertNode:child toParent:parent atIndex:index];
  } @catch (NSException* e) {
    LynxWarning(e, error::E_LAYOUT_INTERNAL,
                [[NSString stringWithFormat:@"%@:%@", [e name], [e reason]] UTF8String]);
  }
}

void LayoutContextDarwin::RemoveLayoutNode(int parent, int child, int index) {
  [nodeOwner removeNode:child fromParent:parent atIndex:index];
}

void LayoutContextDarwin::MoveLayoutNode(int parent, int child, int from_index, int to_index) {
  [nodeOwner moveNode:child inParent:parent fromIndex:from_index toIndex:to_index];
}

void LayoutContextDarwin::DestroyLayoutNodes(const std::unordered_set<int>& ids) {
  for (int sign : ids) {
    [nodeOwner destroyNode:sign];
  }
}

void LayoutContextDarwin::UpdateLayoutNode(int sign, PropBundle* painting_data) {
  PropBundleDarwin* pda = static_cast<PropBundleDarwin*>(painting_data);
  [nodeOwner updateNodeWithSign:sign
                          props:pda->dictionary()
                       eventSet:pda ? pda->event_set() : nil
                  lepusEventSet:pda ? pda->lepus_event_set() : nil];
}

void LayoutContextDarwin::OnLayoutBefore(int id) { [nodeOwner didLayoutStartOnNode:id]; }

void LayoutContextDarwin::OnLayout(int sign, float left, float top, float width, float height,
                                   const std::array<float, 4>& paddings,
                                   const std::array<float, 4>& borders) {
  [nodeOwner didUpdateLayoutLeft:left top:top width:width height:height onNode:sign];
}

void LayoutContextDarwin::ScheduleLayout(base::closure callback) {
  [nodeOwner.layoutTick requestLayout];
}

void LayoutContextDarwin::Destroy() { [nodeOwner destroy]; }

void LayoutContextDarwin::UpdateRootSize(float width, float height) {
  [nodeOwner updateRootSize:width height:height];
}

std::unique_ptr<PlatformExtraBundle> LayoutContextDarwin::GetPlatformExtraBundle(
    int32_t signature) {
  LynxShadowNode* node = [nodeOwner nodeWithSign:signature];

  if (node == nil) {
    return std::unique_ptr<PlatformExtraBundle>();
  }

  id bundle = [node getExtraBundle];

  if (bundle == nil) {
    return std::unique_ptr<PlatformExtraBundle>();
  }

  return std::make_unique<PlatformExtraBundleDarwin>(signature, nullptr, bundle);
}

std::unique_ptr<PlatformExtraBundleHolder> LayoutContextDarwin::ReleasePlatformBundleHolder() {
  // iOS platform not use bunlde holder
  return std::unique_ptr<PlatformExtraBundleHolder>();
}

void LayoutContextDarwin::SetFontFaces(const FontFacesMap& fontFaces) {
  for (auto fontfaceIter = fontFaces.begin(); fontfaceIter != fontFaces.end(); ++fontfaceIter) {
    NSMutableDictionary<NSString*, NSString*>* dict = [NSMutableDictionary new];
    const FontFaceAttrsMap& map = fontfaceIter->second[0].get()->second;
    for (auto iter = map.begin(); iter != map.end(); iter++) {
      NSString* dictValue = [NSString stringWithUTF8String:iter->second.c_str()];
      NSString* dictKey = [NSString stringWithUTF8String:iter->first.c_str()];
      [dict setValue:dictValue forKey:dictKey];
    }
    NSString* fontFamily = [dict valueForKey:@"font-family"];
    NSString* src = [dict valueForKey:@"src"];
    if (fontFamily == nil || src == nil) {
      continue;
    }
    LynxFontFace* face = [[LynxFontFace alloc] initWithFamilyName:fontFamily
                                                           andSrc:src
                                                  withLynxContext:nodeOwner.uiContext.lynxContext];
    if (face == nil) {
      continue;
    }
    [nodeOwner.uiContext.fontFaceContext addFontFace:face];
  }
}

void LayoutContextDarwin::SetLayoutNodeManager(LayoutNodeManager* layout_node_manager) {
  [nodeOwner setLayoutNodeManager:layout_node_manager];
}

}  // namespace tasm
}  // namespace lynx
