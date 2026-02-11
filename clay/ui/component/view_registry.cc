// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/view_registry.h"

#include "clay/ui/component/builtin_views.h"
#include "clay/ui/component/native_view.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/shadow/shadow_node.h"

namespace clay {

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
  BaseView* view = nullptr;

  auto itr = registry_.find(tag_name);
  if (itr != registry_.end()) {
    view = itr->second.view_creator(id, page_view);
  }

  if (UNLIKELY(view && itr->second.is_native_view &&
               !static_cast<NativeView*>(view)->IsNativeViewAvailable())) {
    // Create native view failed. We will destroy this view so that we can
    // avoid costly performance overhead in NativeView without many if
    // statements.
    FML_DLOG(ERROR) << "Create native view fail(tag:" << tag_name << ")";
    delete view;
    view = nullptr;
  }
  if (!view) {
    FML_DLOG(ERROR) << " unsupported view type: " << tag_name;
  }
  return view;
}

ShadowNode* ViewRegistry::CreateShadowNode(int32_t id, ShadowNodeOwner* owner,
                                           const std::string& tag_name) {
  auto itr = registry_.find(tag_name);
  if (itr != registry_.end() && itr->second.shadow_node_creator) {
    return itr->second.shadow_node_creator(id, owner, tag_name);
  }
  FML_DLOG(ERROR) << " unsupported shadownode type: " << tag_name;
  return nullptr;
}

int32_t ViewRegistry::GetTagInfo(const std::string& tag_name) {
  const int32_t kTagInfoCustom = 1 << 2;
  // result can also store is_virtual information when it is needed.
  int32_t result = 0;
  auto itr = registry_.find(tag_name);
  if (itr != registry_.end() && itr->second.shadow_node_creator) {
    result |= kTagInfoCustom;
  }
  return result;
}

bool ViewRegistry::HasView(const std::string& tag_name) const {
  return registry_.find(tag_name) != registry_.end();
}

}  // namespace clay
