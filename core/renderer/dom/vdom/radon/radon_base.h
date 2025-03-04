// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_VDOM_RADON_RADON_BASE_H_
#define CORE_RENDERER_DOM_VDOM_RADON_RADON_BASE_H_

#include <functional>
#include <memory>
#include <unordered_set>

#include "base/include/value/base_string.h"
#include "base/include/vector.h"
#include "base/trace/native/trace_event.h"
#include "core/renderer/dom/selector/selector_item.h"
#include "core/renderer/dom/vdom/radon/radon_dispatch_option.h"
#include "core/renderer/dom/vdom/radon/radon_element.h"
#include "core/renderer/dom/vdom/radon/radon_factory.h"
#include "core/renderer/dom/vdom/radon/radon_types.h"
#include "core/renderer/utils/base/base_def.h"

namespace lynx {
namespace tasm {

class Element;
class RadonPage;
class RadonComponent;
class RadonPlug;

using RadonNodeIndexType = uint32_t;
using RadonBaseVector =
    base::InlineVector<std::unique_ptr<RadonBase>, kChildrenInlineVectorSize>;
constexpr RadonNodeIndexType kRadonInvalidNodeIndex = 0;

class RadonBase : public SelectorItem {
 public:
  RadonBase(RadonNodeType node_type, const base::String& tag_name,
            RadonNodeIndexType node_index);
  RadonBase(const RadonBase& node, PtrLookupMap& map);
  virtual ~RadonBase() = default;
  virtual void SetComponent(RadonComponent* component);
  void NeedModifySubTreeComponent(RadonComponent* const target);
  virtual void ModifySubTreeComponent(RadonComponent* const target);
  RadonComponent* component() const { return radon_component_; }
  RadonComponent* radon_component_ = nullptr;

  virtual void Dispatch(const DispatchOption&);
  virtual void DispatchSelf(const DispatchOption&);
  void DispatchSubTree(const DispatchOption&);
  virtual void DispatchChildren(const DispatchOption&);

  virtual void DispatchForDiff(const DispatchOption&);
  virtual void DispatchChildrenForDiff(const DispatchOption&);
  virtual void RadonDiffChildren(
      const std::unique_ptr<RadonBase>& old_radon_child,
      const DispatchOption& option);

  /* Radon Element Struct */
  virtual bool NeedsElement() const { return false; }

  virtual Element* element() const { return nullptr; }

  virtual RadonElement* radon_element() const;

  virtual const fml::RefPtr<Element>& GetElementRef() const;

  virtual Element* LastNoFixedElement() const;

  Element* ParentElement();

  Element* PreviousSiblingElement();

  // WillRemoveNode is used to handle some special logic before
  // RemoveElementFromParent or radon's structure dtor
  virtual void WillRemoveNode();
  virtual void RemoveElementFromParent();
  virtual int ImplId() const { return kInvalidImplId; }

  /*devtool notify element added*/
  virtual bool GetDevToolFlag();

  virtual void NotifyElementNodeAdded() {}
  virtual RadonPlug* GetRadonPlug() { return nullptr; }

  RadonPage* root_node();

  // To find the current node's parent lazyComponent or radonPage.
  RadonComponent* GetRootEntryNode();

  // To check whether this node is connected with root node.
  // After this function is called, the root_node_ will be set to a correct
  // value.
  bool IsConnectedWithRootNode();

  // used to get page's root element
  // taking page element feature in consideration
  Element* GetRootElement();

  RadonBase* Parent() { return radon_parent_; }
  const RadonBase* Parent() const { return radon_parent_; }

  int32_t IndexInSiblings() const;

  /* getter and setters */
  RadonNodeType NodeType() const { return node_type_; }
  RadonNodeIndexType NodeIndex() const { return node_index_; }
  const base::String& TagName() const { return tag_name_; }

  virtual bool IsRadonNode() const { return false; }

  bool dispatched() { return dispatched_; }

  // Used to clear sub-node's element tree structure,
  // but remain Radon Tree structure
  // Should call RemoveElementFromParent before
  // calling ResetElementRecursively.
  virtual void ResetElementRecursively();

  // return true if lynx-key is setted successfully
  bool SetLynxKey(const base::String& key, const lepus::Value& value);

  /* Radon Tree Struct */
  virtual void AddChild(std::unique_ptr<RadonBase> child);
  void AddChildWithoutSetComponent(std::unique_ptr<RadonBase> child);
  virtual void AddSubTree(std::unique_ptr<RadonBase> child);
  /* Be careful:
   * If you want to destruct one radon node, please use
   * ClearChildrenRecursivelyInPostOrder before RemoveChild.
   * See ClearChildrenRecursivelyInPostOrder comment for more info.
   */
  std::unique_ptr<RadonBase> RemoveChild(RadonBase* child);
  RadonBase* LastChild();
  void Visit(bool including_self,
             const base::MoveOnlyClosure<bool, RadonBase*>& visitor);
  RadonBase* radon_parent_ = nullptr;
  RadonBase* radon_previous_ = nullptr;
  RadonBase* radon_next_ = nullptr;

  /* Recursively clear children.
   * We need to call this function before one radon node is about to destruct.
   * In this case the radon node will destruct in the order of its children to
   * itself.
   */
  /* Reason: Sometimes the radon node may call its parent's function
   * when destruct, so we need to retain this node while its children is
   * destructing.
   */
  /* Example: When one radon component is destructing, it may call
   * component->GetParentComponent() in FireComponentLifecycleEvent. If its
   * parent component has been destructed yet, the program will crash. So we
   * need to destruct the child component before destructing the parent
   * component.
   */
  void ClearChildrenRecursivelyInPostOrder();

  virtual void MarkChildStyleDirtyRecursively(bool is_root){};

  /* Recursively call Component removed lifecycle in post order.
   * But save the original radon tree structure.
   */
  virtual void OnComponentRemovedInPostOrder();

  RadonBaseVector radon_children_ = {};

  // component item_key_ in list new arch;
  base::String list_item_key_;
  void SetListItemKey(const base::String& list_item_key) {
    list_item_key_ = list_item_key;
  }
  const base::String& GetListItemKey() const { return list_item_key_; }

  bool IsRadonComponent() const {
    return kRadonComponent == node_type_ || kRadonLazyComponent == node_type_;
  }

  bool IsRadonLazyComponent() const {
    return kRadonLazyComponent == node_type_;
  }

  bool IsRadonPage() const { return kRadonPage == node_type_; }

  virtual bool CanBeReusedBy(const RadonBase* const radon_base) const;

#if ENABLE_TRACE_PERFETTO
  virtual void UpdateTraceDebugInfo(TraceEvent* event) {
    auto* tagInfo = event->add_debug_annotations();
    tagInfo->set_name("tagName");
    tagInfo->set_string_value(tag_name_.str());
  }
#endif

 protected:
  friend class DispatchOptionObserverForInspector;

  /* node_index_ is generated by radon_parser.cc. Each <tag> has a different
   * node_index_. Two RadonNode in by RadonForNode will has same node_index_. In
   * other case, every RadonNode has different node_index_.
   */
  RadonNodeType node_type_ = kRadonUnknown;
  const RadonNodeIndexType node_index_ = kRadonInvalidNodeIndex;
  const base::String tag_name_;

  void RadonMyersDiff(RadonBaseVector& old_radon_children,
                      const DispatchOption& option);

  virtual void SwapElement(const std::unique_ptr<RadonBase>& old_radon_base,
                           const DispatchOption& option){};

  virtual void triggerNewLifecycle(const DispatchOption& option);

  bool will_remove_node_has_been_called_{false};
  bool dispatched_ = false;

  constexpr static const char* const kLynxKey = "lynx-key";
  lepus::Value lynx_key_;
  RadonPage* root_node_{nullptr};
  RadonComponent* root_entry_node_{nullptr};
  Element* root_element_{nullptr};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_VDOM_RADON_RADON_BASE_H_
