// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_TREE_RESOLVER_H_
#define CORE_RENDERER_DOM_FIBER_TREE_RESOLVER_H_

#include <memory>
#include <utility>

#include "base/include/fml/memory/ref_ptr.h"
#include "core/renderer/dom/fiber/fiber_element.h"
#include "core/runtime/vm/lepus/table.h"

namespace lynx {
namespace tasm {

class ElementManager;
class CSSStyleSheetManager;

class TreeResolver {
 public:
  // Representing depth levels of cloning operations.
  enum class CloningDepth {
    kSingle = 0,     // Clone only the current element.
    kTemplateScope,  // Clone the current template scope.
    kTree,           // Clone the entire subtree.
  };

  static void NotifyNodeInserted(FiberElement* insertion_point,
                                 FiberElement* node);
  static void NotifyNodeRemoved(FiberElement* insertion_point,
                                FiberElement* node);

  static void AttachChildToTargetParentForWrapper(FiberElement* parent,
                                                  FiberElement* child,
                                                  FiberElement* ref_node);
  static void AttachChildToTargetContainerRecursive(FiberElement* parent,
                                                    FiberElement* child,
                                                    FiberElement* wrapper);

  /**
   *  find the Real parent for a wrapper parent
   * @param parent the parent in the Element tree
   * @param child  current child node
   * @param ref the ref node to be inserted before, null means to append to the
   * end
   * @return return the real parent and the index in LayoutNode tree
   */
  static std::pair<FiberElement*, int> FindParentForChildForWrapper(
      FiberElement* parent, FiberElement* child, FiberElement* ref);
  static int GetLayoutIndexForChildForWrapper(FiberElement* parent,
                                              FiberElement* child);

  static size_t GetLayoutChildrenCountForWrapper(FiberElement* node);

  static void RemoveFromParentForWrapperChild(FiberElement* parent,
                                              FiberElement* child);

  static FiberElement* FindFirstChildOrSiblingAsRefNode(FiberElement* ref);

  static void RemoveChildRecursively(FiberElement* parent, FiberElement* child);

  static FiberElement* FindTheRealParent(FiberElement* node);

  static fml::RefPtr<lepus::Dictionary> GetTemplateParts(
      const fml::RefPtr<FiberElement>& template_element);

  // Construct element tree according to the element-template info.
  static base::Vector<fml::RefPtr<FiberElement>> FromTemplateInfo(
      const ElementTemplateInfo& info);

  static lepus::Value InitElementTree(
      base::Vector<fml::RefPtr<FiberElement>>&& elements, int64_t pid,
      ElementManager* manager,
      const std::shared_ptr<CSSStyleSheetManager>& style_manager);

  // Clone the elements and attach it to the current page's element manager
  // recursively.
  static fml::RefPtr<FiberElement> CloneElements(
      const fml::RefPtr<FiberElement>& root,
      const std::shared_ptr<CSSStyleSheetManager>& style_manager,
      bool clone_resolved_props, CloningDepth cloning_depth);

  // Clone the elements recursively but not yet attached to the current page's
  // element manager. The cloned element needs to be attached to an element
  // manager before it can be used.
  static fml::RefPtr<FiberElement> CloneElementRecursively(
      const FiberElement* element, bool clone_resolved_props);

  static void AttachRootToElementManager(
      fml::RefPtr<FiberElement>& root, ElementManager* element_manager,
      const std::shared_ptr<CSSStyleSheetManager>& style_manager,
      bool keep_element_id);

  static void AttachToElementManagerRecursively(
      FiberElement& element, ElementManager* manager,
      const std::shared_ptr<CSSStyleSheetManager>& style_manager,
      bool keep_element_id);

 protected:
  static void GetPartsRecursively(const fml::RefPtr<FiberElement>& root,
                                  fml::RefPtr<lepus::Dictionary>& parts_map);

  // Construct element according to the element info.
  static fml::RefPtr<FiberElement> FromElementInfo(int64_t parent_component_id,
                                                   const ElementInfo& info);
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_TREE_RESOLVER_H_
