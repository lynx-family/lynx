// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/view_registry.h"

#include "base/trace/native/trace_event.h"
#include "clay/ui/component/builtin_views.h"
#include "clay/ui/component/native_view.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/platform/native_view_tags.h"
#include "clay/ui/shadow/native_view_shadow_node.h"
#include "clay/ui/shadow/shadow_node.h"
#include "clay/ui/shadow/shadow_node_owner.h"
#include "core/base/trace/trace_event_def.h"

namespace clay {

namespace {

BaseView* CreateNativeViewIfAvailable(int32_t id, const std::string& tag_name,
                                      PageView* page_view) {
  TRACE_EVENT("clay", CLAY_VIEW_REGISTRY_CREATE_NATIVE_VIEW_IF_AVAILABLE, "id",
              id, "tag", tag_name.c_str());
  if (page_view == nullptr || page_view->GetServiceManager() == nullptr) {
    TRACE_EVENT("clay",
                CLAY_VIEW_REGISTRY_CREATE_NATIVE_VIEW_IF_AVAILABLE_RESULT, "id",
                id, "tag", tag_name.c_str(), "available", false, "reason",
                "missing_page_view_or_service_manager");
    return nullptr;
  }
  auto* view = new NativeView(id, tag_name, page_view);
  if (UNLIKELY(!view->IsNativeViewAvailable())) {
    FML_DLOG(ERROR) << "Create native view fail(tag:" << tag_name << ")";
    TRACE_EVENT("clay",
                CLAY_VIEW_REGISTRY_CREATE_NATIVE_VIEW_IF_AVAILABLE_RESULT, "id",
                id, "tag", tag_name.c_str(), "available", false, "reason",
                "native_view_unavailable");
    view->Destroy();
    delete view;
    return nullptr;
  }
  TRACE_EVENT("clay", CLAY_VIEW_REGISTRY_CREATE_NATIVE_VIEW_IF_AVAILABLE_RESULT,
              "id", id, "tag", tag_name.c_str(), "available", true);
  return view;
}

bool IsInternalPlatformViewTag(const std::string& tag_name) {
  const auto& tags = InternalPlatformViewTags();
  return tags.find(tag_name) != tags.end();
}

bool HasInternalPlatformViewShadowNode(const std::string& tag_name) {
  const auto& tags = InternalPlatformViewShadowNodeTags();
  return tags.find(tag_name) != tags.end();
}

ShadowNode* CreateNativeViewShadowNode(int32_t id, ShadowNodeOwner* owner,
                                       const std::string& tag_name) {
  return GetShadowNodeCreator<NativeViewShadowNode>()(id, owner, tag_name);
}

}  // namespace

ViewRegistry* ViewRegistry::GetInstance() {
  static ViewRegistry* instance = new ViewRegistry();
  return instance;
}

ViewRegistry::ViewRegistry() {
  keepBuiltinElements();
#if (defined(OS_OSX) || defined(OS_IOS))
  for (auto p = __start_clayview; p != __stop_clayview; ++p) {
    this->RegisterView(std::string(p->name), p->view_creator,
                       p->shadow_node_creator);
  }
#endif
}

void ViewRegistry::RegisterView(const std::string& tag,
                                ViewCreator view_creator,
                                ShadowNodeCreator shadow_node_creator,
                                bool is_native_view) {
  registry_[tag] = {view_creator, shadow_node_creator, is_native_view};
}

BaseView* ViewRegistry::CreateView(int32_t id, const std::string& tag_name,
                                   PageView* page_view) {
  TRACE_EVENT("clay", CLAY_VIEW_REGISTRY_CREATE_VIEW, "id", id, "tag",
              tag_name.c_str());
  // Native-view resolution priority:
  // InternalPlatformViewTags > C++ registered views > fallback native views.
  // InternalPlatformViewTags is platform-specific and intentionally shadows C++
  // entries only for tags that should be platform native views on that
  // platform. Example: Android keeps x-video-engine here to use the x-element
  // provider, while iOS omits it so the C++ VideoEngineView wins and uses
  // VideoPlayerServiceDarwin/VideoEngine.mm. On iOS, ClayPlatformViewRegistry
  // also filters the dynamic wrapped x-video-engine tag so the normal iOS
  // XElement video engine cannot override that C++ path.
  if (IsInternalPlatformViewTag(tag_name)) {
    return CreateNativeViewIfAvailable(id, tag_name, page_view);
  }

  auto itr = registry_.find(tag_name);
  if (itr != registry_.end()) {
    BaseView* view = itr->second.view_creator(id, page_view);
    if (UNLIKELY(view && itr->second.is_native_view &&
                 !static_cast<NativeView*>(view)->IsNativeViewAvailable())) {
      // Create native view failed. Ensure proper teardown to avoid leaking
      // platform resources.
      FML_DLOG(ERROR) << "Create native view fail(tag:" << tag_name << ")";
      view->Destroy();
      delete view;
      view = nullptr;
    }
    if (!view) {
      FML_DLOG(ERROR) << " unsupported view type: " << tag_name;
    }
    return view;
  }

  BaseView* view = nullptr;
  if (ShouldCreateFallbackNativeViewDirectly()) {
    view = CreateNativeViewIfAvailable(id, tag_name, page_view);
  }

  if (!view) {
    FML_DLOG(ERROR) << " unsupported view type: " << tag_name;
  }
  return view;
}

ShadowNode* ViewRegistry::CreateShadowNode(int32_t id, ShadowNodeOwner* owner,
                                           const std::string& tag_name) {
  TRACE_EVENT("clay", CLAY_VIEW_REGISTRY_CREATE_SHADOW_NODE, "id", id, "tag",
              tag_name.c_str());
  if (HasInternalPlatformViewShadowNode(tag_name)) {
    return CreateNativeViewShadowNode(id, owner, tag_name);
  }

  auto itr = registry_.find(tag_name);
  if (itr != registry_.end() && itr->second.shadow_node_creator) {
    return itr->second.shadow_node_creator(id, owner, tag_name);
  }
  if (itr == registry_.end() && ShouldCreateFallbackNativeViewDirectly()) {
    return CreateNativeViewShadowNode(id, owner, tag_name);
  }
  FML_DLOG(ERROR) << " unsupported shadownode type: " << tag_name;
  return nullptr;
}

int32_t ViewRegistry::GetTagInfo(const std::string& tag_name) {
  return GetTagInfo(tag_name, nullptr);
}

int32_t ViewRegistry::GetTagInfo(const std::string& tag_name,
                                 PageView* page_view) {
  const int32_t kTagInfoCustom = 1 << 2;
  // result can also store is_virtual information when it is needed.
  int32_t result = 0;
  if (HasInternalPlatformViewShadowNode(tag_name)) {
    return result | kTagInfoCustom;
  }
  auto itr = registry_.find(tag_name);
  if (itr != registry_.end() && itr->second.shadow_node_creator) {
    result |= kTagInfoCustom;
  }
  if (itr == registry_.end() && ShouldCreateFallbackNativeViewDirectly()) {
    result |= kTagInfoCustom;
  }
  return result;
}

bool ViewRegistry::HasView(const std::string& tag_name) const {
  return registry_.find(tag_name) != registry_.end();
}

}  // namespace clay
