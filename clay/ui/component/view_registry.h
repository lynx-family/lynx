// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_VIEW_REGISTRY_H_
#define CLAY_UI_COMPONENT_VIEW_REGISTRY_H_

#include <functional>
#include <string>
#include <unordered_map>

#include "build/build_config.h"

#ifdef CLAY_ELEMENTS_HEADER
#define TO_STR_(x) #x
#define TO_STR(x) TO_STR_(x)
#include TO_STR(CLAY_ELEMENTS_HEADER)
#undef TO_STR_
#undef TO_STR
#endif

namespace clay {

#if defined(OS_ANDROID)
#define KEEP_SYMBOL __attribute__((used))
#elif defined(__clang__) || defined(__GNUC__)
#define KEEP_SYMBOL __attribute__((used, retain))
#else
#define KEEP_SYMBOL __attribute__((used))
#endif

#define CONCAT2_(A, B) A##B
#define CONCAT2(A, B) CONCAT2_(A, B)
#define UNIQUE_NAME(base) CONCAT2(base, __COUNTER__)

#define REGISTER_VIEW_TAG(TAG, VIEW, SHADOW_NODE)                             \
  KEEP_SYMBOL inline static const bool UNIQUE_NAME(VIEW##_registered_) = [] { \
    ViewRegistry::GetInstance()->RegisterView(                                \
        TAG, GetViewCreator<VIEW>(), GetShadowNodeCreator<SHADOW_NODE>());    \
    return true;                                                              \
  }();

class BaseView;
class ShadowNode;
class PageView;
class ShadowNodeOwner;

using ViewCreator = std::function<BaseView*(int id, PageView* page_view)>;
using ShadowNodeCreator = std::function<ShadowNode*(
    int id, ShadowNodeOwner* owner, const std::string& tag_name)>;

template <typename T>
inline ViewCreator GetViewCreator() {
  return [](int id, PageView* page_view) { return new T(id, page_view); };
}

template <typename T>
inline ShadowNodeCreator GetShadowNodeCreator() {
  return [](int id, ShadowNodeOwner* owner, const std::string& tag_name) {
    return new T(owner, tag_name, id);
  };
}

template <>
inline ShadowNodeCreator GetShadowNodeCreator<void>() {
  return nullptr;
}

struct ViewRegistryDescription {
  std::string_view name;
  ViewCreator view_creator;
  ShadowNodeCreator shadow_node_creator;
};

#if (defined(OS_OSX) || defined(OS_IOS))

// section name for Mac and iOS
#define CLAYVIEW_SEC_STR "__DATA,__clayview"

// section start and end ptr
extern "C" const ViewRegistryDescription __start_clayview[] __asm(
    "section$start$__DATA$__clayview");
extern "C" const ViewRegistryDescription __stop_clayview[] __asm(
    "section$end$__DATA$__clayview");

// regist to secitons
#define REGISTER_CLAY_ELEMENT(TAG, VIEW, SHADOW_NODE)         \
  extern "C" __attribute__((used, section(CLAYVIEW_SEC_STR))) \
  const ViewRegistryDescription UNIQUE_NAME(VIEW##_desc){     \
      TAG, GetViewCreator<VIEW>(), GetShadowNodeCreator<SHADOW_NODE>()};
#else
#define REGISTER_CLAY_ELEMENT REGISTER_VIEW_TAG
#endif

class ViewRegistry {
 public:
  struct Entry {
    ViewCreator view_creator;
    ShadowNodeCreator shadow_node_creator;
    bool is_native_view;
  };

  static ViewRegistry* GetInstance();

  // NOTE: Use macro `REGISTER_VIEW_TAG` to register views.
  void RegisterView(const std::string& tag, ViewCreator view_creator,
                    ShadowNodeCreator shadow_node_creator = nullptr,
                    bool is_native_view = false);

  BaseView* CreateView(int32_t id, const std::string& tag_name,
                       PageView* page_view);

  ShadowNode* CreateShadowNode(int32_t id, ShadowNodeOwner* owner,
                               const std::string& tag_name);

  int32_t GetTagInfo(const std::string& tag_name);

 private:
  ViewRegistry();

  std::unordered_map<std::string, Entry> registry_;
#ifdef CLAY_ELEMENTS_REF_FUNC_DEFINES
  CLAY_ELEMENTS_REF_FUNC_DEFINES
#endif
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_VIEW_REGISTRY_H_
