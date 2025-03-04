// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/template_codec/generator/template_scope.h"

#include <functional>

namespace {
bool HolderCompare(const std::string &left_path, int lid,
                   const std::string &right_path, int rid) {
  return left_path + std::to_string(lid) < right_path + std::to_string(rid);
}
}  // namespace

namespace std {
template <>
struct less<lynx::tasm::Template *> {
  bool operator()(const lynx::tasm::Template *lhs,
                  const lynx::tasm::Template *rhs) const {
    return HolderCompare(lhs->path(), lhs->id(), rhs->path(), rhs->id());
  }
};

template <>
struct less<lynx::tasm::Fragment *> {
  bool operator()(const lynx::tasm::Fragment *lhs,
                  const lynx::tasm::Fragment *rhs) const {
    return HolderCompare(lhs->path(), lhs->id(), rhs->path(), rhs->id());
  }
};

template <>
struct less<lynx::tasm::Component *> {
  bool operator()(const lynx::tasm::Component *lhs,
                  const lynx::tasm::Component *rhs) const {
    return HolderCompare(lhs->path(), lhs->id(), rhs->path(), rhs->id());
  }
};
}  // namespace std

namespace lynx {
namespace tasm {

void FindNecessaryComponentInComponent(
    PackageInstance *instance, Component *cur_component,
    std::set<Component *> &necessary_components) {
  if (necessary_components.find(cur_component) != necessary_components.end()) {
    return;
  }
  necessary_components.insert(cur_component);
  auto &dependent_components = cur_component->dependent_components();
  for (auto it = dependent_components.begin(); it != dependent_components.end();
       ++it) {
    auto dc = instance->GetComponent(it->second);
    dc->set_name(it->first);
    FindNecessaryComponentInComponent(instance, dc, necessary_components);
  }
}

void FindNecessaryInComponent(std::set<Component *> &necessary_components,
                              std::set<Fragment *> &necessary_fragments,
                              std::set<Template *> &necessary_templates) {
  for (const auto &component : necessary_components) {
    FindNecessaryFragmentInFragment(component, necessary_fragments);
  }

  for (const auto &fragment : necessary_fragments) {
    for (auto &it : fragment->templates()) {
      necessary_templates.insert(it.second.get());
    }
    for (auto &it : fragment->include_templates()) {
      necessary_templates.insert(it.second.get());
    }
  }
}

void FindNecessaryFragmentInFragment(
    Fragment *cur_fragment, std::set<Fragment *> &necessary_fragments) {
  if (necessary_fragments.find(cur_fragment) != necessary_fragments.end()) {
    return;
  }
  necessary_fragments.insert(cur_fragment);
  auto &dependent_fragments = cur_fragment->dependent_fragments();
  for (auto it = dependent_fragments.begin(); it != dependent_fragments.end();
       ++it) {
    FindNecessaryFragmentInFragment(it->second.get(), necessary_fragments);
  }
}

// TODO(songshorui.null): the following functions will be used in Randon, after
//  fix bug in issue:#2641, the functions may be removed.
void FindNecessaryInFragment(Fragment *cur_fragment,
                             std::set<Fragment *> &necessary_fragments,
                             std::set<Template *> &necessary_templates) {
  if (necessary_fragments.find(cur_fragment) != necessary_fragments.end()) {
    return;
  }

  auto &dependent_templates = cur_fragment->templates();
  for (auto it = dependent_templates.begin(); it != dependent_templates.end();
       ++it) {
    auto templ = it->second.get();
    necessary_templates.insert(templ);
  }

  necessary_fragments.insert(cur_fragment);
  auto &dependent_fragments = cur_fragment->dependent_fragments();
  for (auto it = dependent_fragments.begin(); it != dependent_fragments.end();
       ++it) {
    FindNecessaryInFragment(it->second.get(), necessary_fragments,
                            necessary_templates);
  }
}

void FindNecessaryInComponent(Component *cur_component,
                              PackageInstance *instance,
                              std::set<Component *> &necessary_components,
                              std::set<Fragment *> &necessary_fragments,
                              std::set<Template *> &necessary_templates) {
  if (necessary_components.find(cur_component) != necessary_components.end()) {
    return;
  }

  auto &dependent_components = cur_component->dependent_components();
  FindNecessaryInFragment(cur_component, necessary_fragments,
                          necessary_templates);

  necessary_components.insert(cur_component);
  for (auto it = dependent_components.begin(); it != dependent_components.end();
       ++it) {
    auto dc = instance->GetComponent(it->second);
    dc->set_name(it->first);
    FindNecessaryInComponent(dc, instance, necessary_components,
                             necessary_fragments, necessary_templates);
  }
}

}  // namespace tasm
}  // namespace lynx
