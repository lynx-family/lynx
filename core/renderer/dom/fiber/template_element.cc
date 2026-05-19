// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/template_element.h"

#include <functional>
#include <future>
#include <utility>

#include "base/include/log/logging.h"
#include "base/include/value/array.h"
#include "base/include/value/base_value.h"
#include "base/trace/native/trace_event.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/tree_resolver.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/template_entry.h"
#include "core/renderer/trace/renderer_trace_event_def.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/renderer/utils/value_utils.h"

namespace lynx {
namespace tasm {
namespace {

// These keys define the serialized Element Template payload consumed by
// FiberSerializeElementTemplate / FiberCreateElementTemplate.
static constexpr const char kTemplateTag[] = "template";
static constexpr const char kDefaultTemplateBundleUrl[] = "__Card__";
static constexpr const char kTemplateKey[] = "templateKey";
static constexpr const char kTemplateTypedTag[] = "tag";
static constexpr const char kTemplateAttributes[] = "attributes";
static constexpr const char kTemplateBundleUrl[] = "bundleUrl";
static constexpr const char kTemplateAttributeSlots[] = "attributeSlots";
static constexpr const char kTemplateElementSlots[] = "elementSlots";
static constexpr const char kTemplateUid[] = "uid";
static constexpr const char kTemplateRootAttributeSpread[] = "rootAttributes";
static constexpr uint32_t kTypedTemplateRootSlotIndex = 0;

fml::RefPtr<FiberElement> ResolveInitialElementSlotChild(
    const lepus::Value& child) {
  if (!child.IsRefCounted()) {
    return nullptr;
  }

  auto ref_counted = child.RefCounted();
  if (ref_counted->GetRefType() != lepus::RefType::kElement) {
    return nullptr;
  }

  return fml::static_ref_ptr_cast<FiberElement>(ref_counted);
}

void RemoveElementFromSlotChildren(lepus::Value* slot_children,
                                   FiberElement* child) {
  if (slot_children == nullptr || child == nullptr ||
      !slot_children->IsArray()) {
    return;
  }
  auto array = slot_children->Array();
  for (size_t i = 0; i < array->size();) {
    auto slot_child = ResolveInitialElementSlotChild(array->get(i));
    if (slot_child != nullptr && slot_child.get() == child) {
      array->Erase(static_cast<uint32_t>(i));
      continue;
    }
    ++i;
  }
}

size_t FindSlotChildIndex(const lepus::Value& slot_children,
                          FiberElement* child) {
  if (child == nullptr || !slot_children.IsArray()) {
    return static_cast<size_t>(slot_children.GetLength());
  }
  for (size_t i = 0; i < static_cast<size_t>(slot_children.GetLength()); ++i) {
    auto slot_child = ResolveInitialElementSlotChild(
        slot_children.GetProperty(static_cast<uint32_t>(i)));
    if (slot_child != nullptr && slot_child.get() == child) {
      return i;
    }
  }
  return static_cast<size_t>(slot_children.GetLength());
}

lepus::Value CreateRootAttributeSlots(const lepus::Value& root_attributes) {
  auto attribute_slots = lepus::CArray::Create();
  attribute_slots->emplace_back(root_attributes.IsObject()
                                    ? lepus::Value::Clone(root_attributes)
                                    : lepus::Value());
  return lepus::Value(std::move(attribute_slots));
}

SharedTemplateAttributes CreateRootSpreadTemplateAttributes() {
  return std::make_shared<const TemplateAttributes>(TemplateAttributes{
      Attribute{ATTRIBUTE_BINDING_TYPE_SPREAD,
                BASE_STATIC_STRING(kTemplateRootAttributeSpread),
                lepus::Value(), kTypedTemplateRootSlotIndex}});
}

void ApplyInitialAttributeSlots(
    const base::Vector<fml::RefPtr<FiberElement>>& targets,
    const lepus::Value& attribute_slots) {
  FiberElement* previous_element = nullptr;
  for (const auto& target : targets) {
    auto* element = target.get();
    if (element == nullptr || element == previous_element) {
      continue;
    }
    TreeResolver::ApplyTemplateAttributesToElement(element, attribute_slots);
    previous_element = element;
  }
}

void ApplyStaticEventAttributes(
    const base::Vector<fml::RefPtr<FiberElement>>& targets) {
  for (const auto& target : targets) {
    TreeResolver::ApplyStaticTemplateEventAttributesToElement(target.get());
  }
}

void PrepareGeneratedElementsResult(GeneratedElementsResult* generated,
                                    const lepus::Value& element_slots) {
  if (generated == nullptr) {
    return;
  }

  if (!element_slots.IsArrayOrJSArray()) {
    return;
  }

  // Resolve slot children early, but defer insertion until GetRoot consumes the
  // prepared tree on the main render path.
  for (size_t slot_index = 0;
       slot_index < static_cast<size_t>(element_slots.GetLength());
       ++slot_index) {
    auto slot_children =
        element_slots.GetProperty(static_cast<uint32_t>(slot_index));
    if (!slot_children.IsArrayOrJSArray()) {
      continue;
    }

    for (size_t child_index = 0;
         child_index < static_cast<size_t>(slot_children.GetLength());
         ++child_index) {
      auto child = ResolveInitialElementSlotChild(
          slot_children.GetProperty(static_cast<uint32_t>(child_index)));
      if (child == nullptr) {
        continue;
      }
      PreparedElementSlotInsertion insertion;
      insertion.slot_index_ = static_cast<uint32_t>(slot_index);
      insertion.child_ = std::move(child);
      generated->prepared_element_slot_insertions_.push_back(
          std::move(insertion));
    }
  }
}

GeneratedElementsResult GeneratePreparedElementsResult(
    TemplateEntry* entry, const base::String& template_key,
    const lepus::Value& element_slots) {
  GeneratedElementsResult generated;
  if (entry != nullptr) {
    auto& info = entry->GetElementTemplateInfo(template_key.str());
    generated = TreeResolver::GenerateElementsFromTemplateInfo(info);
  }
  PrepareGeneratedElementsResult(&generated, element_slots);
  return generated;
}

}  // namespace

TemplateElement::TemplateElement(ElementManager* element_manager)
    : FiberElement(element_manager, BASE_STATIC_STRING(kTemplateTag)),
      bundle_url_(BASE_STATIC_STRING(kDefaultTemplateBundleUrl)) {
  MarkTemplateElement();
}

TemplateElement::~TemplateElement() = default;

void TemplateElement::SetTypedTag(const base::String& typed_tag) {
  typed_tag_ = typed_tag;
  if (IsPageTemplate()) {
    MarkInTemplateTreeAndPrepare();
  }
  if (is_in_template_tree_) {
    MarkTemplateChildrenInElementSlotsInTree();
  }
}

void TemplateElement::SetRootAttributes(const lepus::Value& attributes) {
  if (!attributes.IsObject() && !attributes.IsNil() &&
      !attributes.IsUndefined()) {
    return;
  }
  auto previous_root_attributes = root_attributes_;
  root_attributes_ = attributes.IsObject()
                         ? lepus::Value::Clone(attributes.ToLepusValue())
                         : lepus::Value();
  ApplyRootAttributes(previous_root_attributes);
}

void TemplateElement::SetElementSlots(const lepus::Value& element_slots) {
  element_slots_ = element_slots;
  if (IsPageTemplate()) {
    MarkInTemplateTreeAndPrepare();
  }
  if (is_in_template_tree_) {
    MarkTemplateChildrenInElementSlotsInTree();
  }
}

void TemplateElement::PrepareAsyncCreateElementTree() {
  if (IsTypedTemplate()) {
    return;
  }
  if (result_ != nullptr || async_create_task_ != nullptr ||
      prepared_cached_tree_.has_value()) {
    return;
  }
  if (TryPrepareFromCache()) {
    return;
  }
  auto* manager = element_manager();
  if (manager == nullptr) {
    return;
  }

  if (entry_ == nullptr && tasm_ != nullptr) {
    entry_ = tasm_->FindEntry(bundle_url_.str()).get();
  }

  async_create_task_ = CreateAsyncCreateElementTreeTask(entry_);
  manager->EnqueuePostMTSRenderTask(
      base::closure([task = async_create_task_]() { task->Run(); }));
}

base::OnceTaskRefptr<GeneratedElementsResult>
TemplateElement::CreateAsyncCreateElementTreeTask(TemplateEntry* entry) {
  std::promise<GeneratedElementsResult> promise;
  auto future = promise.get_future();
  auto template_key = template_key_;
  auto element_slots = element_slots_;
  return fml::MakeRefCounted<base::OnceTask<GeneratedElementsResult>>(
      [entry, template_key = std::move(template_key),
       element_slots = std::move(element_slots),
       promise = std::move(promise)]() mutable {
        promise.set_value(
            GeneratePreparedElementsResult(entry, template_key, element_slots));
      },
      std::move(future));
}

void TemplateElement::ResolveGeneratedElements() {
  if (result_ != nullptr) {
    return;
  }

  if (IsTypedTemplate()) {
    InitTypedRoot();
    if (result_ == nullptr) {
      return;
    }
    GeneratedElementsResult generated;
    PrepareGeneratedElementsResult(&generated, element_slots_);
    prepared_element_slot_insertions_ =
        std::move(generated.prepared_element_slot_insertions_);
    ApplyRootAttributes(lepus::Value());
    ApplyInitialElementSlots();
    ApplyPendingOperations();
    return;
  }

  if (prepared_cached_tree_.has_value() || TryPrepareFromCache()) {
    ConsumePreparedCachedTree();
    return;
  }

  if (async_create_task_ == nullptr) {
    PrepareAsyncCreateElementTree();
    if (async_create_task_ == nullptr) {
      return;
    }
  }

  async_create_task_->Run();
  auto generated = async_create_task_->GetFuture().get();
  async_create_task_ = nullptr;
  result_ = std::move(generated.result_);
  attribute_slot_targets_ = std::move(generated.attribute_slot_targets_);
  static_event_targets_ = std::move(generated.static_event_targets_);
  element_slot_targets_ = std::move(generated.element_slot_targets_);
  prepared_element_slot_insertions_ =
      std::move(generated.prepared_element_slot_insertions_);

  // Attach generated elements and mount slot children only when the template is
  // actually materialized into the Fiber tree.
  InitGeneratedElementTree();
  ApplyRootAttributes(lepus::Value());
  ApplyInitialElementSlots();
  ApplyPendingOperations();
}

void TemplateElement::InitGeneratedElementTree() {
  auto* manager = element_manager();
  if (result_ == nullptr || manager == nullptr || entry_ == nullptr) {
    return;
  }
  auto* root = manager->root();
  TreeResolver::InitElementTree(result_, root != nullptr ? root->impl_id() : -1,
                                manager, entry_->GetStyleSheetManager());
  // Event attributes must be applied after the generated tree is attached so
  // FiberAddEvent can sync EventListenerMap when event-refactor is enabled.
  ApplyStaticEventAttributes(static_event_targets_);
  ApplyInitialAttributeSlots(attribute_slot_targets_, attribute_slots_);
}

void TemplateElement::InitTypedRoot() {
  if (!IsTypedTemplate() || result_ != nullptr) {
    return;
  }

  auto* manager = element_manager();
  if (manager == nullptr) {
    return;
  }

  result_ = manager->CreateFiberElement(typed_tag_);
  if (result_ == nullptr) {
    return;
  }
  result_->MarkTemplateElement();

  element_slot_targets_.clear();
  element_slot_targets_.push_back(ElementSlotMountPoint{result_, nullptr});
}

bool TemplateElement::IsPageTemplate() const {
  return IsTypedTemplate() && typed_tag_.IsEqual(kElementPageTag);
}

bool TemplateElement::TryPrepareFromCache() {
  if (prepared_cached_tree_.has_value()) {
    return true;
  }
  auto* manager = element_manager();
  if (manager == nullptr) {
    return false;
  }
  CachedTemplateElementTree cached_tree;
  if (!manager->TakeCachedTemplateElementTree(bundle_url_, template_key_,
                                              &cached_tree)) {
    return false;
  }

  prepared_cached_tree_.emplace(PreparedCachedTemplateElementTree{
      std::move(cached_tree.generated_),
      std::move(cached_tree.applied_attribute_slots_)});
  return true;
}

void TemplateElement::ConsumePreparedCachedTree() {
  auto prepared_tree = std::move(*prepared_cached_tree_);
  prepared_cached_tree_.reset();

  result_ = std::move(prepared_tree.generated_.result_);
  attribute_slot_targets_ =
      std::move(prepared_tree.generated_.attribute_slot_targets_);
  static_event_targets_ =
      std::move(prepared_tree.generated_.static_event_targets_);
  element_slot_targets_ =
      std::move(prepared_tree.generated_.element_slot_targets_);

  GeneratedElementsResult generated;
  PrepareGeneratedElementsResult(&generated, element_slots_);
  prepared_element_slot_insertions_ =
      std::move(generated.prepared_element_slot_insertions_);

  ApplyAttributeSlotsFromPrevious(prepared_tree.applied_attribute_slots_);
  ApplyRootAttributes(lepus::Value());
  ApplyInitialElementSlots();
  ApplyPendingOperations();
}

bool TemplateElement::MoveElementTreeToCacheIfNeeded() {
  if (IsTypedTemplate()) {
    return false;
  }
  auto* manager = element_manager();
  if (manager == nullptr) {
    return false;
  }

  if (result_ == nullptr) {
    if (!prepared_cached_tree_.has_value()) {
      return false;
    }
    CachedTemplateElementTree cached_tree;
    cached_tree.generated_ = std::move(prepared_cached_tree_->generated_);
    cached_tree.applied_attribute_slots_ =
        std::move(prepared_cached_tree_->applied_attribute_slots_);
    prepared_cached_tree_.reset();
    async_create_task_ = nullptr;
    manager->PutCachedTemplateElementTree(bundle_url_, template_key_,
                                          std::move(cached_tree));
    return true;
  }

  if (result_->parent() != nullptr) {
    static_cast<FiberElement*>(result_->parent())->RemoveNode(result_);
  }

  CachedTemplateElementTree cached_tree;
  cached_tree.generated_.result_ = std::move(result_);
  cached_tree.generated_.attribute_slot_targets_ =
      std::move(attribute_slot_targets_);
  cached_tree.generated_.static_event_targets_ =
      std::move(static_event_targets_);
  cached_tree.generated_.element_slot_targets_ =
      std::move(element_slot_targets_);
  cached_tree.applied_attribute_slots_ =
      attribute_slots_.IsEmpty() ? lepus::Value()
                                 : lepus::Value::Clone(attribute_slots_);

  result_ = nullptr;
  attribute_slot_targets_.clear();
  static_event_targets_.clear();
  element_slot_targets_.clear();
  prepared_element_slot_insertions_.clear();
  prepared_cached_tree_.reset();
  async_create_task_ = nullptr;

  manager->PutCachedTemplateElementTree(bundle_url_, template_key_,
                                        std::move(cached_tree));
  return true;
}

void TemplateElement::MarkInTemplateTreeAndPrepare() {
  if (is_in_template_tree_) {
    return;
  }
  is_in_template_tree_ = true;
  PrepareAsyncCreateElementTree();
}

void TemplateElement::MarkInTemplateTreeAndPrepareRecursively() {
  if (is_in_template_tree_) {
    return;
  }
  MarkInTemplateTreeAndPrepare();
  MarkTemplateChildrenInElementSlotsInTree();
}

void TemplateElement::MarkTemplateChildrenInElementSlotsInTree() {
  if (!element_slots_.IsArrayOrJSArray()) {
    return;
  }

  for (size_t slot_index = 0;
       slot_index < static_cast<size_t>(element_slots_.GetLength());
       ++slot_index) {
    auto slot_children =
        element_slots_.GetProperty(static_cast<uint32_t>(slot_index));
    if (!slot_children.IsArrayOrJSArray()) {
      continue;
    }

    for (size_t child_index = 0;
         child_index < static_cast<size_t>(slot_children.GetLength());
         ++child_index) {
      auto child = ResolveInitialElementSlotChild(
          slot_children.GetProperty(static_cast<uint32_t>(child_index)));
      if (child == nullptr || !child->is_template()) {
        continue;
      }
      static_cast<TemplateElement*>(child.get())
          ->MarkInTemplateTreeAndPrepareRecursively();
    }
  }
}

void TemplateElement::DetachElementSlotChildrenForCacheRecursively() {
  if (!element_slots_.IsArrayOrJSArray()) {
    return;
  }

  for (size_t slot_index = 0;
       slot_index < static_cast<size_t>(element_slots_.GetLength());
       ++slot_index) {
    auto slot_children =
        element_slots_.GetProperty(static_cast<uint32_t>(slot_index));
    if (!slot_children.IsArrayOrJSArray()) {
      continue;
    }

    ElementSlotMountPoint mount_point;
    if (slot_index < element_slot_targets_.size()) {
      mount_point = element_slot_targets_[slot_index];
    }
    for (size_t child_index = 0;
         child_index < static_cast<size_t>(slot_children.GetLength());
         ++child_index) {
      auto child = ResolveInitialElementSlotChild(
          slot_children.GetProperty(static_cast<uint32_t>(child_index)));
      DetachAndMaybeCacheElementSlotChild(mount_point, child);
    }
  }
}

void TemplateElement::DetachAndMaybeCacheElementSlotChild(
    const ElementSlotMountPoint& mount_point,
    const fml::RefPtr<FiberElement>& child) {
  if (child == nullptr) {
    return;
  }

  TemplateElement* template_child = nullptr;
  if (child->is_template()) {
    template_child = static_cast<TemplateElement*>(child.get());
    template_child->DetachElementSlotChildrenForCacheRecursively();
  }

  UnmountElementSlotChild(mount_point, child);

  if (template_child != nullptr) {
    template_child->MoveElementTreeToCacheIfNeeded();
  }
}

void TemplateElement::ApplyAttributeSlotsFromPrevious(
    const lepus::Value& previous_attribute_slots) {
  FiberElement* previous_element = nullptr;
  for (const auto& target : attribute_slot_targets_) {
    auto* element = target.get();
    if (element == nullptr || element == previous_element) {
      continue;
    }
    TreeResolver::ApplyTemplateAttributesToElement(
        element, previous_attribute_slots, attribute_slots_);
    previous_element = element;
  }
}

void TemplateElement::ApplyRootAttributes(
    const lepus::Value& previous_root_attributes) {
  if (result_ == nullptr ||
      (!previous_root_attributes.IsObject() && !root_attributes_.IsObject())) {
    return;
  }

  result_->SetTemplateAttributes(CreateRootSpreadTemplateAttributes());
  if (previous_root_attributes.IsObject()) {
    TreeResolver::ApplyTemplateAttributesToElement(
        result_.get(), CreateRootAttributeSlots(previous_root_attributes),
        CreateRootAttributeSlots(root_attributes_));
    return;
  }
  TreeResolver::ApplyTemplateAttributesToElement(
      result_.get(), CreateRootAttributeSlots(root_attributes_));
}

void TemplateElement::ApplyAttributeSlotToTarget(
    uint32_t slot_index, const lepus::Value& previous_attribute_slots) {
  if (result_ == nullptr || slot_index >= attribute_slot_targets_.size()) {
    return;
  }
  auto target = attribute_slot_targets_[slot_index];
  if (target == nullptr) {
    return;
  }
  TreeResolver::ApplyTemplateAttributesToElement(
      target.get(), previous_attribute_slots, attribute_slots_);
}

void TemplateElement::ApplyPendingOperations() {
  auto operations = std::move(pending_operations_);
  pending_operations_.clear();

  for (const auto& operation : operations) {
    if (operation.type_ == PendingOperation::Type::kSetAttributeSlot) {
      SetAttributeSlot(operation.slot_index_, operation.value_);
      continue;
    }
    if (operation.type_ == PendingOperation::Type::kInsertElementSlotChild) {
      InsertElementSlotChild(operation.slot_index_, operation.child_,
                             operation.ref_node_);
      continue;
    }
    if (operation.type_ == PendingOperation::Type::kRemoveElementSlotChild) {
      RemoveElementSlotChild(operation.slot_index_, operation.child_);
      continue;
    }
  }
}

void TemplateElement::ApplyInitialElementSlots() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_ELEMENT_APPLY_INITIAL_ELEMENT_SLOTS,
              "template_key", template_key_.str(), "bundle_url",
              bundle_url_.str());
  for (const auto& insertion : prepared_element_slot_insertions_) {
    auto slot_index = static_cast<size_t>(insertion.slot_index_);
    if (slot_index >= element_slot_targets_.size()) {
      continue;
    }
    const auto& mount_point = element_slot_targets_[slot_index];
    if (mount_point.parent_ == nullptr || insertion.child_ == nullptr) {
      continue;
    }
    InsertInitialElementSlotChild(mount_point, insertion.child_);
  }
}

void TemplateElement::InsertInitialElementSlotChild(
    const ElementSlotMountPoint& mount_point,
    const fml::RefPtr<FiberElement>& child) {
  if (mount_point.parent_ == nullptr || child == nullptr) {
    return;
  }
  if (mount_point.ref_node_ != nullptr) {
    mount_point.parent_->InsertNodeBefore(child, mount_point.ref_node_);
  } else {
    mount_point.parent_->InsertNode(child);
  }
}

void TemplateElement::MountElementSlotChild(
    const ElementSlotMountPoint& mount_point,
    const fml::RefPtr<FiberElement>& child,
    const fml::RefPtr<FiberElement>& ref_node) {
  if (mount_point.parent_ == nullptr || child == nullptr) {
    return;
  }

  auto mounted_child = child;
  if (child->is_template()) {
    auto* template_child = static_cast<TemplateElement*>(child.get());
    if (template_child->result_ != nullptr) {
      mounted_child = template_child->result_;
    }
  }

  auto mounted_ref_node = ref_node;
  if (mounted_ref_node != nullptr && mounted_ref_node->is_template()) {
    auto* template_ref = static_cast<TemplateElement*>(mounted_ref_node.get());
    if (template_ref->result_ != nullptr) {
      mounted_ref_node = template_ref->result_;
    }
  }

  if (mounted_ref_node != nullptr) {
    mount_point.parent_->InsertNodeBefore(mounted_child, mounted_ref_node);
  } else if (mount_point.ref_node_ != nullptr) {
    mount_point.parent_->InsertNodeBefore(mounted_child, mount_point.ref_node_);
  } else {
    mount_point.parent_->InsertNode(mounted_child);
  }
}

void TemplateElement::UnmountElementSlotChild(
    const ElementSlotMountPoint& mount_point,
    const fml::RefPtr<FiberElement>& child) {
  if (mount_point.parent_ == nullptr || child == nullptr) {
    return;
  }

  auto mounted_child = child;
  if (child->is_template()) {
    auto* template_child = static_cast<TemplateElement*>(child.get());
    if (template_child->result_ != nullptr) {
      mounted_child = template_child->result_;
    }
  }

  if (mounted_child->parent() == mount_point.parent_.get()) {
    mount_point.parent_->RemoveNode(mounted_child);
  }
}

lepus::Value TemplateElement::GetOrCreateElementSlotChildren(
    uint32_t slot_index) {
  if (element_slots_.IsEmpty()) {
    element_slots_ = lepus::Value(lepus::CArray::Create());
  }
  auto slot_children = element_slots_.GetProperty(slot_index);
  if (slot_children.IsEmpty()) {
    slot_children = lepus::Value(lepus::CArray::Create());
  }
  element_slots_.SetProperty(slot_index, slot_children);
  return slot_children;
}

void TemplateElement::RemoveElementSlotChildFromSlot(uint32_t slot_index,
                                                     FiberElement* child) {
  if (child == nullptr || !element_slots_.IsArray() ||
      slot_index >= static_cast<uint32_t>(element_slots_.GetLength())) {
    return;
  }
  auto slot_children = element_slots_.GetProperty(slot_index);
  if (!slot_children.IsArray()) {
    return;
  }
  RemoveElementFromSlotChildren(&slot_children, child);
  element_slots_.SetProperty(slot_index, slot_children);
}

lepus::Value TemplateElement::Serialize() const {
  if (IsTypedTemplate()) {
    return SerializeTypedTemplate();
  }
  return SerializeCompiledTemplate();
}

lepus::Value TemplateElement::SerializeTypedTemplate() const {
  auto serialized = lepus::Dictionary::Create();
  serialized->SetValue(BASE_STATIC_STRING(kTemplateTypedTag), typed_tag_);
  auto attributes = SerializeRootAttributes();
  if (!attributes.IsEmpty()) {
    serialized->SetValue(BASE_STATIC_STRING(kTemplateAttributes),
                         std::move(attributes));
  }
  serialized->SetValue(BASE_STATIC_STRING(kTemplateElementSlots),
                       SerializeElementSlots());
  serialized->SetValue(BASE_STATIC_STRING(kTemplateUid), uid_);
  return lepus::Value(std::move(serialized));
}

lepus::Value TemplateElement::SerializeRootAttributes() const {
  if (!root_attributes_.IsObject() || root_attributes_.GetLength() == 0) {
    return lepus::Value();
  }
  return root_attributes_;
}

lepus::Value TemplateElement::SerializeCompiledTemplate() const {
  auto serialized = lepus::Dictionary::Create();
  serialized->SetValue(BASE_STATIC_STRING(kTemplateKey), template_key_);
  serialized->SetValue(BASE_STATIC_STRING(kTemplateBundleUrl), bundle_url_);
  serialized->SetValue(BASE_STATIC_STRING(kTemplateAttributeSlots),
                       attribute_slots_);
  serialized->SetValue(BASE_STATIC_STRING(kTemplateElementSlots),
                       SerializeElementSlots());
  serialized->SetValue(BASE_STATIC_STRING(kTemplateUid), uid_);
  return lepus::Value(std::move(serialized));
}

lepus::Value TemplateElement::SerializeElementSlots() const {
  if (!element_slots_.IsArrayOrJSArray()) {
    return element_slots_;
  }

  auto serialized_slots = lepus::CArray::Create();
  serialized_slots->reserve(element_slots_.GetLength());
  for (size_t slot_index = 0;
       slot_index < static_cast<size_t>(element_slots_.GetLength());
       ++slot_index) {
    serialized_slots->emplace_back(SerializeElementSlotChildren(
        element_slots_.GetProperty(static_cast<uint32_t>(slot_index))));
  }
  return lepus::Value(std::move(serialized_slots));
}

lepus::Value TemplateElement::SerializeElementSlotChildren(
    const lepus::Value& slot_children) const {
  if (!slot_children.IsArrayOrJSArray()) {
    return slot_children;
  }

  auto serialized_children = lepus::CArray::Create();
  serialized_children->reserve(slot_children.GetLength());
  for (size_t child_index = 0;
       child_index < static_cast<size_t>(slot_children.GetLength());
       ++child_index) {
    auto serialized_child = SerializeElementSlotChild(
        slot_children.GetProperty(static_cast<uint32_t>(child_index)));
    if (!serialized_child.IsEmpty() && !serialized_child.IsUndefined()) {
      serialized_children->emplace_back(std::move(serialized_child));
    }
  }
  return lepus::Value(std::move(serialized_children));
}

lepus::Value TemplateElement::SerializeElementSlotChild(
    const lepus::Value& child) const {
  if (!child.IsRefCounted()) {
    LOGE(
        "SerializeElementTemplate only supports TemplateElement children in "
        "elementSlots, but got non-refcounted child.");
    return lepus::Value();
  }

  auto ref_counted = child.RefCounted();
  if (ref_counted->GetRefType() != lepus::RefType::kElement) {
    LOGE(
        "SerializeElementTemplate only supports TemplateElement children in "
        "elementSlots.");
    return lepus::Value();
  }

  auto element =
      fml::static_ref_ptr_cast<FiberElement>(ref_counted).strongify();
  if (element == nullptr || !element->is_template()) {
    LOGE(
        "SerializeElementTemplate only supports TemplateElement children in "
        "elementSlots.");
    return lepus::Value();
  }
  auto template_element = fml::static_ref_ptr_cast<TemplateElement>(element);
  return template_element->Serialize();
}

fml::RefPtr<FiberElement> TemplateElement::GetRoot() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_ELEMENT_GET_ROOT, "template_key",
              template_key_.str(), "bundle_url", bundle_url_.str());
  ResolveGeneratedElements();

  EXEC_EXPR_FOR_INSPECTOR(
      auto* manager = element_manager();
      if (result_ != nullptr && manager != nullptr &&
          manager->GetDevToolFlag() && manager->IsDomTreeEnabled()) {
        std::function<void(FiberElement*)> prepare_node_f =
            [manager, &prepare_node_f](FiberElement* element) {
              manager->PrepareNodeForInspector(element);
              for (const auto& child : element->children()) {
                prepare_node_f(static_cast<FiberElement*>(child.get()));
              }
            };
        prepare_node_f(result_.get());
      });
  return result_;
}

void TemplateElement::SetAttributeSlot(uint32_t slot_index,
                                       const lepus::Value& value) {
  if (IsTypedTemplate()) {
    if (slot_index == kTypedTemplateRootSlotIndex) {
      SetRootAttributes(value);
    }
    return;
  }
  const bool should_apply_to_result = result_ != nullptr;
  if (!should_apply_to_result) {
    pending_operations_.emplace_back(PendingOperation::Type::kSetAttributeSlot,
                                     slot_index, value);
    return;
  }

  auto previous_attribute_slots = attribute_slots_;
  if (attribute_slots_.IsEmpty()) {
    attribute_slots_ = lepus::Value(lepus::CArray::Create());
  }
  attribute_slots_.SetProperty(slot_index, value);

  ApplyAttributeSlotToTarget(slot_index, previous_attribute_slots);
}

void TemplateElement::InsertElementSlotChild(
    uint32_t slot_index, const fml::RefPtr<FiberElement>& child,
    const fml::RefPtr<FiberElement>& ref_node) {
  if (child == nullptr || child.get() == ref_node.get()) {
    return;
  }
  if (IsTypedTemplate() && slot_index != kTypedTemplateRootSlotIndex) {
    return;
  }

  if (is_in_template_tree_ && child->is_template()) {
    static_cast<TemplateElement*>(child.get())
        ->MarkInTemplateTreeAndPrepareRecursively();
  }

  const bool should_apply_to_result = result_ != nullptr;
  if (!should_apply_to_result) {
    pending_operations_.emplace_back(
        PendingOperation::Type::kInsertElementSlotChild, slot_index, child,
        ref_node);
    return;
  }

  // TODO(songshourui.null): Restore insert-or-move semantics by removing this
  // child from existing element slot records before inserting it here, so
  // Serialize() does not keep stale same-slot or cross-slot entries.
  auto slot_children = GetOrCreateElementSlotChildren(slot_index);
  auto insert_index = FindSlotChildIndex(slot_children, ref_node.get());
  slot_children.Array()->Insert(static_cast<uint32_t>(insert_index),
                                lepus::Value(child));
  element_slots_.SetProperty(slot_index, slot_children);

  if (slot_index < element_slot_targets_.size()) {
    MountElementSlotChild(element_slot_targets_[slot_index], child, ref_node);
  }
}

void TemplateElement::RemoveElementSlotChild(
    uint32_t slot_index, const fml::RefPtr<FiberElement>& child) {
  if (child == nullptr) {
    return;
  }
  if (IsTypedTemplate() && slot_index != kTypedTemplateRootSlotIndex) {
    return;
  }

  const bool should_apply_to_result = result_ != nullptr;
  if (!should_apply_to_result) {
    pending_operations_.emplace_back(
        PendingOperation::Type::kRemoveElementSlotChild, slot_index, child);
    return;
  }

  RemoveElementSlotChildFromSlot(slot_index, child.get());
  if (slot_index < element_slot_targets_.size()) {
    DetachAndMaybeCacheElementSlotChild(element_slot_targets_[slot_index],
                                        child);
  } else if (child->is_template()) {
    auto* template_child = static_cast<TemplateElement*>(child.get());
    template_child->DetachElementSlotChildrenForCacheRecursively();
    template_child->MoveElementTreeToCacheIfNeeded();
  }
}

}  // namespace tasm
}  // namespace lynx
