// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_VDOM_RADON_RADON_DISPATCH_OPTION_H_
#define CORE_RENDERER_DOM_VDOM_RADON_RADON_DISPATCH_OPTION_H_

#include <string>
#include <unordered_set>
#include <vector>

#include "core/renderer/dom/attribute_holder.h"

namespace lynx {
namespace tasm {

class RadonBase;
class RadonNode;
class PageProxy;

class ClassTransmitOption {
 public:
  template <typename Iter>
  void RemoveClass(Iter begin, Iter end) {
    for (auto &it = begin; it != end; it++) {
      if (added_classes_.find(*it) == added_classes_.end())
        removed_classes_.insert(*it);
    }
  }

  void AddClass(const base::String &clazz) {
    removed_classes_.erase(clazz);
    added_classes_.insert(clazz);
  }

  bool IsEmpty() { return removed_classes_.empty() && added_classes_.empty(); }

  std::unordered_set<base::String> &removed_classes() {
    return removed_classes_;
  }
  std::unordered_set<base::String> &added_classes() { return added_classes_; }

 private:
  std::unordered_set<base::String> removed_classes_;
  std::unordered_set<base::String> added_classes_;
};

class DispatchOption {
 public:
  DispatchOption(PageProxy *page_proxy);
  mutable ClassTransmitOption class_transmit_;
  // need_notify_devtool_ option is used in devtool.
  bool need_notify_devtool_ = false;
  // global_props_changed_ will be true if __globalProps changed.
  bool global_properties_changed_{false};
  bool css_variable_changed_{false};
  // force_diff_entire_tree_ will be true if
  // UpdatePageOption.reload_template == true.
  // Should re-render this component and continue diff'ing its children if it's
  // true.
  bool force_diff_entire_tree_{false};
  // TODO(kechenglong): The above three options can be merged?
  // use_old_component_data_ will be true if
  // UpdatePageOption.reload_template == true.
  // Should use new rendered component's data when component diffs with
  // component.
  bool use_new_component_data_{false};
  // refresh_lifecycle_ will be true if
  // UpdatePageOption.reload_template == true.
  // Should refresh the whole tree's lifecycle like a new loaded template.
  bool refresh_lifecycle_{false};

  // has_patched_ will be set to true when element is created, removed or
  // updated. Need to call OnPatchFinishInner if has_patched_ is true.
  mutable bool has_patched_{false};

  // The options following are used only in radon diff list new arch.
  // During diff, if only_swap_element_ option is true, we will not destruct old
  // radon tree's structure. When diff component, we will diff a complete and
  // determined radon component (reuser) without element with an old radon
  // component with element (reusee). We just reuse the element of old
  // component.
  bool only_swap_element_{false};
  // When need_update_element_ option is false, we will only handle radon tree
  // structure, but ignore elements' logic
  bool need_update_element_{true};
  // When ignore_component_lifecycle_ is true, we will not handle component's
  // lifecycle even if the component is added or updated.
  bool ignore_component_lifecycle_{false};
  // list-related; When 'need_create_js_counterpart_' is true, we force 'Fresh'
  // the component and in this way the JS counterpart of the current component
  // will be created.
  bool need_create_js_counterpart_{false};

  // force call $renderComponent function and ignore component lifecycle, save
  // data and properties
  bool force_update_this_component{false};

  // Indicate if the current dispatched process is hydrating a ssr page.
  // While hydrating the page, events will need to be flush to platform
  bool ssr_hydrating_{false};
  // While hydrating there is a chance that we can predict the dom structures
  // are identical between the one rendered on server side and the one rendered
  // on client side. In this case, diff can be skipped for better performance
  bool need_diff_{true};

  // ShouldForceUpdate will return true if the component has been
  // updated outside the component itself, even if the component's data and
  // properties are not changed. Should re-render this component and continue
  // diff'ing its children.
  // ShouldForceUpdate can also be used in OptimizedShouldFlushStyle logic.
  // When config or css_variable changed, we should re-calculate the css.
  bool ShouldForceUpdate() const {
    return force_diff_entire_tree_ || css_variable_changed_ ||
           !class_transmit_.IsEmpty() || global_properties_changed_;
  }

 private:
  DispatchOption(const DispatchOption &other) = delete;
  DispatchOption &operator=(const DispatchOption &other) = delete;
  DispatchOption(DispatchOption &&other) = delete;
  DispatchOption &operator=(DispatchOption &&other) = delete;
};

// The usage of ListComponentDispatchOption is the same as DispatchOption.
// But why need ListComponentDispatchOption?
// The databinding process of list sub-component is triggered by platform list,
// hence we need to store some dispatch_option in the list_component_info
// when we update the list.
// When the platform notify radon to update the sub-component, we can
// reuse these dispatchOptions.
// TODO(kechenglong): merge ListComponentDispatchOption and DispatchOption.
class ListComponentDispatchOption {
 public:
  void reset();
  friend bool operator==(const ListComponentDispatchOption &lhs,
                         const ListComponentDispatchOption &rhs);
  friend bool operator!=(const ListComponentDispatchOption &lhs,
                         const ListComponentDispatchOption &rhs);
  // global_props_changed_ will be true if __globalProps changed.
  bool global_properties_changed_{false};
  bool css_variable_changed_{false};
  // force_diff_entire_tree_ will be true if
  // UpdatePageOption.reload_template == true.
  // Should re-render this component and continue diff'ing its children if it's
  // true.
  bool force_diff_entire_tree_{false};
  // TODO(kechenglong): The above three options can be merged?
  // use_old_component_data_ will be true if
  // UpdatePageOption.reload_template == true.
  // Should use new rendered component's data when component diffs with
  // component.
  bool use_new_component_data_{false};
  // refresh_lifecycle_ will be true if
  // UpdatePageOption.reload_template == true.
  // Should refresh the whole tree's lifecycle like a new loaded template.
  bool refresh_lifecycle_{false};
};

class DispatchOptionObserverForInspector {
 public:
  DispatchOptionObserverForInspector(const DispatchOption &option,
                                     RadonBase *radon_base);
  ~DispatchOptionObserverForInspector();

 private:
  bool need_notify_devtool_ = false;
  const DispatchOption &option_;
  RadonBase *radon_base_;  // not owned
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_VDOM_RADON_RADON_DISPATCH_OPTION_H_
