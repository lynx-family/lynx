// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/vdom/radon/radon_dispatch_option.h"

#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/vdom/radon/radon_base.h"
#include "core/renderer/page_proxy.h"
#include "core/renderer/utils/lynx_env.h"

namespace lynx {
namespace tasm {

DispatchOption::DispatchOption(PageProxy* page_proxy)
    : need_notify_devtool_(page_proxy->element_manager()->GetDevToolFlag() &&
                           page_proxy->element_manager()->IsDomTreeEnabled()) {
  if (page_proxy->IsServerSideRendering() || page_proxy->HasSSRRadonPage()) {
    need_update_element_ = false;
  }
}

bool operator==(const ListComponentDispatchOption& lhs,
                const ListComponentDispatchOption& rhs) {
  return (lhs.global_properties_changed_ == rhs.global_properties_changed_) &&
         (lhs.css_variable_changed_ == rhs.css_variable_changed_) &&
         (lhs.force_diff_entire_tree_ == rhs.force_diff_entire_tree_) &&
         (lhs.use_new_component_data_ == rhs.use_new_component_data_) &&
         (lhs.refresh_lifecycle_ == rhs.refresh_lifecycle_);
}

bool operator!=(const ListComponentDispatchOption& lhs,
                const ListComponentDispatchOption& rhs) {
  return !(lhs == rhs);
}

void ListComponentDispatchOption::reset() {
  global_properties_changed_ = false;
  css_variable_changed_ = false;
  force_diff_entire_tree_ = false;
  use_new_component_data_ = false;
  refresh_lifecycle_ = false;
}

DispatchOptionObserverForInspector::DispatchOptionObserverForInspector(
    const DispatchOption& option, RadonBase* radon_base)
    : option_(option), radon_base_(radon_base) {
  if (option_.need_notify_devtool_ && !radon_base_->dispatched_ &&
      radon_base_->element()) {
    need_notify_devtool_ = true;
    const_cast<DispatchOption&>(option_).need_notify_devtool_ = false;
  }
}

DispatchOptionObserverForInspector::~DispatchOptionObserverForInspector() {
  if (need_notify_devtool_) {
    radon_base_->NotifyElementNodeAdded();
    const_cast<DispatchOption&>(option_).need_notify_devtool_ = true;
  } else if (radon_base_->GetDevToolFlag() && radon_base_->element() &&
             radon_base_->element()->is_fixed() && !radon_base_->dispatched_) {
    radon_base_->NotifyElementNodeAdded();
  }
}

}  // namespace tasm
}  // namespace lynx
