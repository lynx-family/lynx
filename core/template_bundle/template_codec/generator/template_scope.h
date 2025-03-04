// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_GENERATOR_TEMPLATE_SCOPE_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_GENERATOR_TEMPLATE_SCOPE_H_

#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "core/template_bundle/template_codec/generator/source_generator.h"
#include "core/template_bundle/template_codec/generator/ttml_holder.h"

namespace lynx {
namespace tasm {

class TemplateScope {
 public:
  TemplateScope(SourceGenerator *context, Template *current) {
    context_ = context;
    pre_template_ = context->current_template_;
    context->current_template_ = current;
  }

  ~TemplateScope() { context_->current_template_ = pre_template_; }

 private:
  SourceGenerator *context_;
  Template *pre_template_;
};

class FragmentScope {
 public:
  FragmentScope(SourceGenerator *context, Fragment *current)
      : template_scope_(context, current) {
    context_ = context;
    pre_fragment_ = context->current_fragment_;
    context->current_fragment_ = current;
  }

  ~FragmentScope() { context_->current_fragment_ = pre_fragment_; }

 private:
  TemplateScope template_scope_;
  SourceGenerator *context_;
  Fragment *pre_fragment_;
};

class ComponentScope {
 public:
  ComponentScope(SourceGenerator *context, Component *current)
      : fragment_scope_(context, current) {
    context_ = context;
    pre_component_ = context->current_component_;
    context->current_component_ = current;
  }

  ~ComponentScope() { context_->current_component_ = pre_component_; }

 private:
  FragmentScope fragment_scope_;
  SourceGenerator *context_;
  Component *pre_component_;
};

class PageScope {
 public:
  PageScope(SourceGenerator *context, Page *current)
      : component_scope_(context, current) {
    context_ = context;
    pre_page_ = context->current_page_;
    context->current_page_ = current;
  }

  ~PageScope() { context_->current_page_ = pre_page_; }

 private:
  ComponentScope component_scope_;
  SourceGenerator *context_;
  Page *pre_page_;
};

class DynamicComponentScope {
 public:
  DynamicComponentScope(SourceGenerator *context, DynamicComponent *current)
      : component_scope_(context, current) {
    context_ = context;
    pre_dynamic_component_ = context->current_dynamic_component_;
    context->current_dynamic_component_ = current;
  }

  ~DynamicComponentScope() {
    context_->current_dynamic_component_ = pre_dynamic_component_;
  }

 private:
  ComponentScope component_scope_;
  SourceGenerator *context_;
  DynamicComponent *pre_dynamic_component_;
};

class DynamicNodeIndexScope {
 public:
  DynamicNodeIndexScope(int &index, bool need_reset_index = true)
      : origin_index_(index) {
    if (need_reset_index) {
      saved_index_ = origin_index_;
      origin_index_ = 0;
    }
  }

  DynamicNodeIndexScope() = delete;

  ~DynamicNodeIndexScope() {
    if (saved_index_ >= 0) {
      origin_index_ = saved_index_;
    }
  }

 private:
  int &origin_index_;
  int saved_index_{-1};
};

// A tool to help recording available components and marking whether
// there is component tag in template so that we can optimize template
// renderer generating
class TemplateHelper {
 public:
  enum IsComponentStatus { STATIC, DYNAMIC, MAYBE, UNDEFINED };

  void RecordAvailableInfo(Template *templ, Component *current_host) {
    RecordAvailableComponent(templ, current_host);
    RecordAvailableDynamicComponent(templ, current_host);
  }

  void RecordAvailableComponent(Template *templ, Component *current_host) {
    auto it = status_recorder_.find(templ);
    if (it == status_recorder_.end()) {
      status_recorder_[templ] = {};
      it = status_recorder_.find(templ);
    }
    auto &status_map = it->second;
    for (auto iit = status_map.begin(); iit != status_map.end(); ++iit) {
      auto has = current_host->dependent_components().find(iit->first);
      if (has == current_host->dependent_components().end()) {
        iit->second = IsComponentStatus::MAYBE;
      }
    }
    for (auto iit = current_host->dependent_components().begin();
         iit != current_host->dependent_components().end(); ++iit) {
      auto has = status_map.find(iit->first);
      if (has == status_map.end()) {
        status_map[iit->first] = IsComponentStatus::STATIC;
      }
    }
  }

  void RecordAvailableDynamicComponent(Template *templ,
                                       Component *current_host) {
    auto it = status_recorder_.find(templ);
    if (it == status_recorder_.end()) {
      status_recorder_[templ] = {};
      it = status_recorder_.find(templ);
    }
    auto &status_map = it->second;
    for (auto iit = status_map.begin(); iit != status_map.end(); ++iit) {
      auto has = current_host->dependent_dynamic_components().find(iit->first);
      if (has == current_host->dependent_dynamic_components().end()) {
        iit->second = IsComponentStatus::MAYBE;
      }
    }
    for (auto iit = current_host->dependent_dynamic_components().begin();
         iit != current_host->dependent_dynamic_components().end(); ++iit) {
      auto has = status_map.find(iit->first);
      if (has == status_map.end()) {
        status_map[iit->first] = IsComponentStatus::DYNAMIC;
      }
    }
  }

  bool MaybeKindOfComponent(Template *templ,
                            const std::string &component_name) {
    if (IsComponent(templ, component_name)) {
      return true;
    }
    if (IsDynamicComponent(templ, component_name)) {
      return true;
    }
    if (MaybeComponent(templ, component_name)) {
      return true;
    }
    return false;
  }

  // TODO(songshourui.null): opt me. If IsComponent, just generate the component
  // code.
  bool IsComponent(Template *templ, const std::string &component_name) {
    auto it = status_recorder_.find(templ);
    if (it != status_recorder_.end()) {
      auto iit = it->second.find(component_name);
      if (iit != it->second.end()) {
        return iit->second == IsComponentStatus::STATIC;
      }
    }
    return false;
  }

  bool IsDynamicComponent(Template *templ, const std::string &component_name) {
    auto it = status_recorder_.find(templ);
    if (it != status_recorder_.end()) {
      auto iit = it->second.find(component_name);
      if (iit != it->second.end()) {
        return iit->second == IsComponentStatus::DYNAMIC;
      }
    }
    return false;
  }

  bool MaybeComponent(Template *templ, const std::string &component_name) {
    auto it = status_recorder_.find(templ);
    if (it != status_recorder_.end()) {
      auto iit = it->second.find(component_name);
      if (iit != it->second.end()) {
        return iit->second == IsComponentStatus::MAYBE;
      }
    }
    return false;
  }

  bool HasComponentTag(Template *templ) {
    auto it = has_component_tag_.find(templ);
    if (it != has_component_tag_.end()) {
      return true;
    }
    return false;
  }

  void RecordTemplateHasComponentTag(Template *templ) {
    has_component_tag_.insert(templ);
  }

 private:
  std::unordered_map<Template *,
                     std::unordered_map<std::string, IsComponentStatus>>
      status_recorder_;
  std::unordered_set<Template *> has_component_tag_;
};

void FindNecessaryComponentInComponent(
    PackageInstance *instance, Component *cur_component,
    std::set<Component *> &necessary_components);

void FindNecessaryInComponent(std::set<Component *> &necessary_components,
                              std::set<Fragment *> &necessary_fragments,
                              std::set<Template *> &necessary_templates);

void FindNecessaryFragmentInFragment(Fragment *cur_fragment,
                                     std::set<Fragment *> &necessary_fragments);

void FindNecessaryInFragment(Fragment *cur_fragment,
                             std::set<Fragment *> &necessary_fragments,
                             std::set<Template *> &necessary_templates);
void FindNecessaryInComponent(Component *cur_component,
                              PackageInstance *instance,
                              std::set<Component *> &necessary_components,
                              std::set<Fragment *> &necessary_fragments,
                              std::set<Template *> &necessary_templates);

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_GENERATOR_TEMPLATE_SCOPE_H_
