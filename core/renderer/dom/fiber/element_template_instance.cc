// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/element_template_instance.h"

#include <algorithm>
#include <functional>
#include <future>
#include <limits>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/value/array.h"
#include "base/include/value/base_value.h"
#include "base/trace/native/trace_event.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/list_element.h"
#include "core/renderer/dom/fiber/tree_resolver.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/template_entry.h"
#include "core/renderer/trace/renderer_trace_event_def.h"
#include "core/renderer/utils/base/tasm_constants.h"

namespace lynx {
namespace tasm {
namespace {

static constexpr const char kDefaultTemplateBundleUrl[] = "__Card__";
static constexpr const char kTemplateKey[] = "templateKey";
static constexpr const char kTemplateTypedTag[] = "tag";
static constexpr const char kTemplateAttributes[] = "attributes";
static constexpr const char kTemplateBundleUrl[] = "bundleUrl";
static constexpr const char kTemplateAttributeSlots[] = "attributeSlots";
static constexpr const char kTemplateChildSlots[] = "childSlots";
static constexpr const char kTemplateOptions[] = "options";
static constexpr const char kTemplateUid[] = "uid";
static constexpr const char kDefaultPageComponentId[] = "0";
static constexpr const char kListItemPlatformInfo[] = "__listItemPlatformInfo";
static constexpr const char kListItemKey[] = "item-key";
static constexpr int32_t kDefaultPageCSSId = 0;
static constexpr uint32_t kTypedTemplateAttributeSlotIndex = 0;
static constexpr uint32_t kTypedTemplateRootChildSlotIndex = 0;
constexpr char kGestureType[] = "type";

void AddPendingSlot(base::Vector<uint32_t>& pending_slots,
                    uint32_t slot_index) {
  for (auto pending_slot : pending_slots) {
    if (pending_slot == slot_index) {
      return;
    }
  }
  pending_slots.push_back(slot_index);
}

void DetachMaterializedElementFromCurrentParent(
    ElementManager* manager, const fml::RefPtr<Element>& node) {
  auto* current_parent = static_cast<Element*>(node->parent());
  if (current_parent == nullptr) {
    return;
  }
  EXEC_EXPR_FOR_INSPECTOR(
      manager->OnElementNodeRemovedForInspector(node.get()));
  current_parent->RemoveNode(node);
}

void PrepareMaterializedElementTreeForInspector(ElementManager* manager,
                                                Element* node) {
  if (node->inspector_attribute() == nullptr) {
    // Compiled nodes receive object styles before attaching to a manager,
    // when SetStyle cannot populate the inspector's inline-style mirror.
    const auto& styles = node->GetCurrentRawInlineStyles();
    if (styles.has_value()) {
      for (const auto& style : *styles) {
        node->data_model()->SetInlineStyle(style.first, style.second,
                                           manager->GetCSSParserConfigs());
      }
    }
    manager->PrepareNodeForInspector(node);
  }
  for (const auto& child : node->children()) {
    PrepareMaterializedElementTreeForInspector(
        manager, static_cast<Element*>(child.get()));
  }
}

void NotifyTemplateAttributesChangedForInspector(ElementManager* manager,
                                                 Element* target) {
  EXEC_EXPR_FOR_INSPECTOR({
    // Initial materialization is captured by PrepareNodeForInspector. Raw
    // string styles notify after parsing during the existing style flush.
    if (target->inspector_attribute() != nullptr &&
        target->GetRawInlineStyles().empty()) {
      manager->OnElementNodeSetForInspector(target);
    }
  });
}

void InitCompiledElementCSS(
    Element* element,
    const std::shared_ptr<CSSStyleSheetManager>& style_manager,
    int32_t inherited_css_id) {
  auto css_id = element->GetCSSID();
  if (css_id == kInvalidCssId) {
    css_id = inherited_css_id;
  }
  element->set_style_sheet_manager(style_manager);
  element->SetCSSID(css_id);
  for (const auto& child : element->children()) {
    InitCompiledElementCSS(child.get(), style_manager, css_id);
  }
}

bool IsLiveTemplateEventValue(const lepus::Value& value) {
  if (value.IsCallable()) {
    return true;
  }
  if (!ElementTemplateInstance::IsTemplateMapValue(value)) {
    return false;
  }
  const auto type = value.GetProperty(BASE_STATIC_STRING(kGestureType));
  return type.IsString() && type.String().IsEqual("worklet");
}

bool IsLiveTemplateGestureValue(const lepus::Value& value) {
  if (!ElementTemplateInstance::IsTemplateMapValue(value)) {
    return false;
  }
  const auto type = value.GetProperty(BASE_STATIC_STRING(kGestureType));
  return type.IsString() && type.String().IsEqual("gesture");
}

bool IsLiveSpreadCallbackEntry(const base::String& key,
                               const lepus::Value& item) {
  if (TreeResolver::IsTemplateEventAttribute(key)) {
    return IsLiveTemplateEventValue(item);
  }
  if (TreeResolver::IsTemplateGestureAttribute(key)) {
    return IsLiveTemplateGestureValue(item);
  }
  return false;
}

bool HasLiveSpreadDescriptorEntry(const lepus::Value& value) {
  if (!ElementTemplateInstance::IsTemplateMapValue(value)) {
    return false;
  }
  bool found = false;
  lepus::Value::ForEachLepusValue(
      value, [&found](const lepus::Value& key, const lepus::Value& item) {
        if (!found && key.IsString() && !item.IsCallable() &&
            IsLiveSpreadCallbackEntry(key.String(), item)) {
          found = true;
        }
      });
  return found;
}

lepus::Value ConvertTemplateMapForStorageInternal(
    const lepus::Value& value, bool preserve_spread_callbacks) {
  auto object = lepus::Dictionary::Create();
  lepus::Value::ForEachLepusValue(value, [&object, preserve_spread_callbacks](
                                             const lepus::Value& key,
                                             const lepus::Value& item) {
    if (key.IsString()) {
      const auto key_str = key.String();
      object->SetValue(
          key_str,
          preserve_spread_callbacks && IsLiveSpreadCallbackEntry(key_str, item)
              ? item
              : ElementTemplateInstance::ConvertTemplateValueForStorage(item));
    }
  });
  return lepus::Value(std::move(object));
}
lepus::Value CopySlotForWorkerPreparation(const lepus::Value& slot,
                                          bool is_spread_slot, bool* modified) {
  if (!slot.IsTable()) {
    return slot;
  }
  const auto& table = *slot.Table();
  auto should_strip = [is_spread_slot](const base::String& key,
                                       const lepus::Value& item) {
    return item.IsCallable() ||
           (is_spread_slot && IsLiveSpreadCallbackEntry(key, item));
  };
  bool has_live_callback = false;
  for (const auto& [key, item] : table) {
    if (should_strip(key, item)) {
      has_live_callback = true;
      break;
    }
  }
  if (!has_live_callback) {
    return slot;
  }
  *modified = true;
  auto filtered = lepus::Dictionary::Create();
  for (const auto& [key, item] : table) {
    if (!should_strip(key, item)) {
      filtered->SetValue(key, item);
    }
  }
  return lepus::Value(std::move(filtered));
}

const SharedTemplateAttributes& TypedTemplateAttributes() {
  static const auto attributes =
      std::make_shared<const TemplateAttributes>(TemplateAttributes{
          Attribute{ATTRIBUTE_BINDING_TYPE_SPREAD,
                    BASE_STATIC_STRING(kTemplateAttributes), lepus::Value(),
                    kTypedTemplateAttributeSlotIndex}});
  return attributes;
}

fml::RefPtr<Element> CreateTypedRootElement(ElementManager* manager,
                                            TemplateAssembler* tasm,
                                            const base::String& tag) {
  if (tag.IsEqual(kElementPageTag)) {
    auto page = manager->CreateFiberPage(
        BASE_STATIC_STRING(kDefaultPageComponentId), kDefaultPageCSSId);
    if (tasm != nullptr) {
      page->set_style_sheet_manager(
          tasm->style_sheet_manager(tasm::DEFAULT_ENTRY_NAME));
    }
    return page;
  }
  if (tag.IsEqual(kElementListTag)) {
    return manager->CreateFiberList(tasm, tag, lepus::Value(), lepus::Value(),
                                    lepus::Value());
  }
  return manager->CreateFiberElement(tag);
}

template <typename Apply>
void ApplyInitialAttributeSlots(
    const base::Vector<fml::RefPtr<Element>>& targets,
    const lepus::Value& attribute_slots, Apply apply) {
  Element* previous_element = nullptr;
  for (const auto& target : targets) {
    auto* element = target.get();
    if (element == nullptr || element == previous_element) {
      continue;
    }
    apply(element, attribute_slots);
    previous_element = element;
  }
}

void ApplyInitialNonEventAttributeSlots(
    const base::Vector<fml::RefPtr<Element>>& targets,
    const lepus::Value& attribute_slots) {
  ApplyInitialAttributeSlots(
      targets, attribute_slots,
      [](Element* element, const lepus::Value& slots) {
        TreeResolver::ApplyTemplateNonEventAttributesToElement(element, slots);
      });
}

void ApplyInitialEventAttributeSlots(
    const base::Vector<fml::RefPtr<Element>>& targets,
    const lepus::Value& attribute_slots) {
  ApplyInitialAttributeSlots(
      targets, attribute_slots,
      [](Element* element, const lepus::Value& slots) {
        TreeResolver::ApplyTemplateEventAttributesToElement(element, slots);
      });
}

void ApplyStaticEventAttributes(
    const base::Vector<fml::RefPtr<Element>>& targets) {
  for (const auto& target : targets) {
    TreeResolver::ApplyStaticTemplateEventAttributesToElement(target.get());
  }
}

GeneratedElementsResult GeneratePreparedElementsResult(
    TemplateEntry* entry, const base::String& template_key,
    const lepus::Value& attribute_slots, uint32_t attribute_slots_generation) {
  GeneratedElementsResult generated;
  if (entry != nullptr) {
    auto& info = entry->GetElementTemplateInfo(template_key.str());
    generated = TreeResolver::GenerateElementsFromTemplateInfo(info);
  }
  ApplyInitialNonEventAttributeSlots(generated.attribute_slot_targets_,
                                     attribute_slots);
  generated.prepared_attribute_slots_ = attribute_slots;
  generated.attribute_slots_generation_ = attribute_slots_generation;
  return generated;
}

struct SlotChildrenDiffResult {
  static constexpr size_t kNoMove = std::numeric_limits<size_t>::max();
  // New index -> ET instance to insert, or nullptr when no insertion is needed.
  base::Vector<fml::RefPtr<ElementTemplateInstance>> inserts_;
  // Old indices of instances to remove.
  base::Vector<size_t> removes_;
  // Old index -> new index, or kNoMove when the old child does not move.
  base::Vector<size_t> moves_;
  size_t insert_count_{0};
  size_t move_count_{0};
};

/**
 * Matches a slot's old and new logical children and records insertions,
 * removals, and moves.
 *
 * With has_item_key enabled, candidates are grouped by string item-key. Missing
 * and non-string keys share a bucket, with no fallback to template types. An
 * empty string is still a string key. With has_item_key disabled, candidates
 * are grouped by GetTemplateType(). Each bucket preserves the old order, and
 * only its first unconsumed candidate is checked. A false is_same_type result
 * records an insertion without consuming the candidate or searching later ones.
 *
 * A type match consumes the old candidate and calls
 * on_diff_children(old, new, old_index, new_index). The callback processes the
 * pair but returns no reuse result. A match does not guarantee physical reuse.
 * After the callback, compare the old index with last_placed_index, the
 * largest old index among previous matches. Record a move if it is smaller;
 * otherwise update last_placed_index. This does not guarantee the fewest moves.
 *
 * Null entries are skipped without renumbering input indices. inserts_ maps new
 * indices to instances to insert; removes_ holds old indices; moves_ maps old
 * indices to new indices. Unconsumed old candidates are recorded as removals.
 * Recording these operations does not insert, detach, or move physical nodes.
 *
 * Example (has_item_key = true, unique string keys, matching template types):
 *   before = [A, B, C, D]
 *   after  = [C, X, A, D]
 *
 *   {
 *     i: {1: X},  // Record inserting X at new index 1.
 *     r: [1],     // Record removing B at old index 1.
 *     m: {0: 2}   // Record moving A from old index 0 to new index 2.
 *   }
 *
 * Here i, r, and m represent inserts_, removes_, and moves_, respectively.
 * Insertion and move examples list only active entries.
 * C and D match without recorded moves; A matches with a recorded move.
 */
SlotChildrenDiffResult DiffSlotChildren(
    const base::Vector<fml::RefPtr<ElementTemplateInstance>>& before_instances,
    const base::Vector<fml::RefPtr<ElementTemplateInstance>>& after_instances,
    const std::function<bool(const fml::RefPtr<ElementTemplateInstance>&,
                             const fml::RefPtr<ElementTemplateInstance>&)>&
        is_same_type,
    const std::function<void(const fml::RefPtr<ElementTemplateInstance>&,
                             const fml::RefPtr<ElementTemplateInstance>&,
                             size_t, size_t)>& on_diff_children,
    bool has_item_key) {
  struct Candidates {
    base::Vector<size_t> indices;
    size_t cursor{0};
  };
  std::unordered_map<base::String, Candidates> candidates_by_key;
  // With item-key matching enabled, old instances with missing or non-string
  // item-key values share this candidate queue.
  Candidates missing_item_key_candidates;
  auto get_candidates =
      [&](const fml::RefPtr<ElementTemplateInstance>& instance) -> Candidates& {
    if (!has_item_key) {
      return candidates_by_key[instance->GetTemplateType()];
    }
    const auto list_item_platform_info =
        instance->GetListItemPlatformInfoFromOptions();
    const auto item_key = list_item_platform_info.IsObject()
                              ? list_item_platform_info.GetProperty(
                                    BASE_STATIC_STRING(kListItemKey))
                              : lepus::Value();
    if (item_key.IsNil() || item_key.IsUndefined() || item_key.IsEmpty() ||
        !item_key.IsString()) {
      return missing_item_key_candidates;
    }
    return candidates_by_key[item_key.String()];
  };

  // Collect candidate indices in old order. Matching checks only the first
  // unconsumed candidate in each bucket. A type mismatch neither advances the
  // cursor nor searches later candidates in that bucket.
  for (size_t old_index = 0; old_index < before_instances.size(); ++old_index) {
    if (before_instances[old_index]) {
      get_candidates(before_instances[old_index]).indices.push_back(old_index);
    }
  }

  SlotChildrenDiffResult result;
  result.inserts_ = base::Vector<fml::RefPtr<ElementTemplateInstance>>(
      after_instances.size());
  result.moves_ = base::Vector<size_t>(before_instances.size(),
                                       SlotChildrenDiffResult::kNoMove);
  size_t last_placed_index = 0;
  for (size_t after_index = 0; after_index < after_instances.size();
       ++after_index) {
    const auto& after_instance = after_instances[after_index];
    if (!after_instance) {
      continue;
    }
    auto& candidates = get_candidates(after_instance);
    if (candidates.cursor < candidates.indices.size()) {
      const auto before_index = candidates.indices[candidates.cursor];
      const auto& before_instance = before_instances[before_index];
      if (is_same_type(before_instance, after_instance)) {
        // Consume the logically matched old candidate and process the pair in
        // the callback. It returns no physical reuse result; move detection
        // still runs afterward using the old index.
        ++candidates.cursor;
        on_diff_children(before_instance, after_instance, before_index,
                         after_index);
        if (before_index < last_placed_index) {
          result.moves_[before_index] = after_index;
          ++result.move_count_;
        } else {
          last_placed_index = before_index;
        }
        continue;
      }
    }
    result.inserts_[after_index] = after_instance;
    ++result.insert_count_;
  }

  // Candidates before cursor have been matched. Record the remaining candidates
  // as removals without detaching their physical nodes here.
  auto collect_removes = [&result](const Candidates& candidates) {
    for (size_t index = candidates.cursor; index < candidates.indices.size();
         ++index) {
      result.removes_.push_back(candidates.indices[index]);
    }
  };
  for (const auto& entry : candidates_by_key) {
    collect_removes(entry.second);
  }
  collect_removes(missing_item_key_candidates);
  return result;
}

/**
 * Applies a slot's structural diff through callbacks and returns the resulting
 * instance sequence in the new order. before_instances must contain no null
 * entries. Removals and move sources use old indices; insertions and move
 * destinations use result indices. Removal sources must not overlap move
 * sources, and insertion destinations must not overlap move destinations.
 * Moves must follow DiffSlotChildren's encoding: each moved instance must be
 * scanned and cached before its destination is emitted. Valid indices alone
 * do not make an arbitrary move mapping executable.
 *
 * Calls on_remove(old_child) for removals. For insertions, calls
 * on_insert(new_child, reference) and stores its return value in the result.
 * For moves, calls on_move(old_child, reference). Retained and moved entries
 * keep their old ET references from before_instances. Physical updates are
 * delegated to the callbacks.
 *
 * The current unconsumed old instance serves as a right-hand logical reference,
 * which may differ from the final immediate next sibling. A null reference
 * means the old sequence is exhausted; static nodes or nodes from other slots
 * may still exist under the physical parent. The callbacks resolve physical
 * anchors. Emitting an insertion or cached move leaves the old cursor unchanged
 * so the same instance can anchor later operations. An empty diff returns the
 * old sequence without invoking any of this function's three callbacks.
 *
 * Example (i = inserts_, r = removes_, m = moves_):
 *   before = [A, B, C, D]
 *   diff   = {i: {1: X}, r: [1], m: {0: 2}}
 *   result = [C, X, A, D]  // Assuming on_insert(X, D) returns X.
 *
 *   Callback order:
 *     on_remove(B)
 *     on_insert(X, D) -> X
 *     on_move(A, D)
 *
 * Cache A, remove B, then emit C. Inserting X and emitting A both use D as the
 * reference; D is emitted last. X's final next sibling is A, but its insertion
 * callback still receives D as the reference.
 */
base::Vector<fml::RefPtr<ElementTemplateInstance>> ApplySlotChildrenDiffResult(
    const base::Vector<fml::RefPtr<ElementTemplateInstance>>& before_instances,
    const SlotChildrenDiffResult& diff_result,
    const std::function<fml::RefPtr<ElementTemplateInstance>(
        const fml::RefPtr<ElementTemplateInstance>&,
        const fml::RefPtr<ElementTemplateInstance>&)>& on_insert,
    const std::function<void(const fml::RefPtr<ElementTemplateInstance>&)>&
        on_remove,
    const std::function<void(const fml::RefPtr<ElementTemplateInstance>&,
                             const fml::RefPtr<ElementTemplateInstance>&)>&
        on_move) {
  // Return the old sequence without invoking structural callbacks for an empty
  // diff.
  if (diff_result.insert_count_ == 0 && diff_result.removes_.empty() &&
      diff_result.move_count_ == 0) {
    return before_instances;
  }

  base::Vector<bool> delete_flags(before_instances.size(), false);
  for (const auto before_index : diff_result.removes_) {
    delete_flags[before_index] = true;
  }

  // Cache scanned moves by destination index, e.g. {2: A} for A moving from 0
  // to 2. Only store references here; call on_move when emitting that position.
  std::unordered_map<size_t, fml::RefPtr<ElementTemplateInstance>>
      move_temp_map;
  base::Vector<fml::RefPtr<ElementTemplateInstance>> after_instances;
  after_instances.reserve(before_instances.size() + diff_result.insert_count_);

  // Count insertions and cached moves that have not yet been emitted. Moves not
  // yet scanned are excluded, so the initial count is just insert_count_.
  size_t remain_size = diff_result.insert_count_;
  size_t i = 0;  // Next output index.
  size_t j = 0;  // Current index in before_instances.
  fml::RefPtr<ElementTemplateInstance> before_instance =
      before_instances.empty() ? nullptr : before_instances[j];
  while (before_instance || remain_size > 0) {
    // Whether to keep the old cursor; false advances j after this iteration.
    bool keep = false;
    if (before_instance && delete_flags[j]) {
      // Case 1: Remove the old entry at j without appending to after_instances.
      // i stays unchanged and keep remains false; advance j afterward.
      on_remove(before_instance);
    } else if (before_instance &&
               diff_result.moves_[j] != SlotChildrenDiffResult::kNoMove) {
      // Case 2: Cache the old instance for a move, e.g. move_temp_map[2] = A
      // when A moves from old index 0 to new index 2.
      move_temp_map.insert_or_assign(diff_result.moves_[j], before_instance);
      // Advance j after this iteration without appending to after_instances,
      // leaving i unchanged. Increment remain_size for the pending cached move.
      ++remain_size;
    } else {
      // Case 3: Fill the new position using this priority:
      // 1. Emit a cached move at this index and call on_move.
      // 2. Insert a new entry at this index and call on_insert.
      // 3. Otherwise, retain the current old instance.
      fml::RefPtr<ElementTemplateInstance> after_instance = before_instance;
      const auto move_it = move_temp_map.find(i);
      if (move_it != move_temp_map.end()) {
        after_instance = move_it->second;
        on_move(after_instance, before_instance);
        keep = true;
        --remain_size;
      } else if (i < diff_result.inserts_.size() && diff_result.inserts_[i]) {
        after_instance = on_insert(diff_result.inserts_[i], before_instance);
        keep = true;
        --remain_size;
      }
      // Append one result entry and advance i.
      after_instances.push_back(after_instance);
      ++i;
    }
    // Insertions and cached moves preserve the old cursor. Otherwise, advance j
    // to read the next old instance.
    if (before_instance && !keep) {
      ++j;
      before_instance =
          j < before_instances.size() ? before_instances[j] : nullptr;
    }
  }
  return after_instances;
}

base::Vector<fml::RefPtr<ElementTemplateInstance>> CollectSlotChildren(
    const lepus::Value& child_slots, int32_t slot_index) {
  base::Vector<fml::RefPtr<ElementTemplateInstance>> instances;
  if (slot_index < 0 || !child_slots.IsArrayOrJSArray() ||
      slot_index >= child_slots.GetLength()) {
    return instances;
  }
  const auto children =
      child_slots.GetProperty(static_cast<uint32_t>(slot_index));
  if (!children.IsArrayOrJSArray()) {
    return instances;
  }
  instances.reserve(children.GetLength());
  for (size_t index = 0; index < static_cast<size_t>(children.GetLength());
       ++index) {
    auto child = ElementTemplateInstance::ResolveElementTemplateInstanceValue(
        children.GetProperty(static_cast<uint32_t>(index)));
    if (child) {
      instances.push_back(std::move(child));
    }
  }
  return instances;
}

}  // namespace

class ElementTemplateInstanceSerializer {
 public:
  lepus::Value Serialize(const ElementTemplateInstance& instance) {
    return instance.IsTypedTemplate() ? SerializeTypedTemplate(instance)
                                      : SerializeCompiledTemplate(instance);
  }

 private:
  lepus::Value SerializeChildSlots(const ElementTemplateInstance& instance) {
    if (!instance.child_slots_.IsArrayOrJSArray()) {
      return instance.child_slots_;
    }
    auto serialized_slots = lepus::CArray::Create();
    const int slot_count = instance.child_slots_.GetLength();
    serialized_slots->reserve(static_cast<size_t>(slot_count));
    for (int slot_index = 0; slot_index < slot_count; ++slot_index) {
      auto slot_children =
          instance.child_slots_.GetProperty(static_cast<uint32_t>(slot_index));
      if (!slot_children.IsArrayOrJSArray()) {
        serialized_slots->emplace_back(std::move(slot_children));
        continue;
      }
      auto serialized_children = lepus::CArray::Create();
      const int child_count = slot_children.GetLength();
      serialized_children->reserve(static_cast<size_t>(child_count));
      for (int child_index = 0; child_index < child_count; ++child_index) {
        auto child =
            ElementTemplateInstance::ResolveElementTemplateInstanceValue(
                slot_children.GetProperty(static_cast<uint32_t>(child_index)));
        if (child != nullptr) {
          serialized_children->emplace_back(Serialize(*child));
        }
      }
      serialized_slots->emplace_back(std::move(serialized_children));
    }
    return lepus::Value(std::move(serialized_slots));
  }

  void SetSerializedOptions(const ElementTemplateInstance& instance,
                            const lepus::DictionaryPtr& serialized) {
    if (instance.options_.IsObject() && instance.options_.GetLength() > 0) {
      serialized->SetValue(BASE_STATIC_STRING(kTemplateOptions),
                           instance.options_);
    }
  }

  void SetSerializedUid(const ElementTemplateInstance& instance,
                        const lepus::DictionaryPtr& serialized) {
    serialized->SetValue(BASE_STATIC_STRING(kTemplateUid), instance.uid_);
  }

  lepus::Value SerializeTypedTemplate(const ElementTemplateInstance& instance) {
    auto serialized = lepus::Dictionary::Create();
    serialized->SetValue(BASE_STATIC_STRING(kTemplateTypedTag),
                         instance.typed_tag_);
    auto attributes =
        instance.attribute_slots_.GetProperty(kTypedTemplateAttributeSlotIndex);
    if (attributes.IsObject() && attributes.GetLength() > 0) {
      serialized->SetValue(BASE_STATIC_STRING(kTemplateAttributes), attributes);
    }
    serialized->SetValue(BASE_STATIC_STRING(kTemplateChildSlots),
                         SerializeChildSlots(instance));
    SetSerializedOptions(instance, serialized);
    SetSerializedUid(instance, serialized);
    return lepus::Value(std::move(serialized));
  }

  lepus::Value SerializeCompiledTemplate(
      const ElementTemplateInstance& instance) {
    auto serialized = lepus::Dictionary::Create();
    serialized->SetValue(BASE_STATIC_STRING(kTemplateKey),
                         instance.template_key_);
    serialized->SetValue(BASE_STATIC_STRING(kTemplateBundleUrl),
                         instance.bundle_url_);
    serialized->SetValue(BASE_STATIC_STRING(kTemplateAttributeSlots),
                         instance.attribute_slots_);
    serialized->SetValue(BASE_STATIC_STRING(kTemplateChildSlots),
                         SerializeChildSlots(instance));
    SetSerializedOptions(instance, serialized);
    SetSerializedUid(instance, serialized);
    return lepus::Value(std::move(serialized));
  }
};

fml::RefPtr<ElementTemplateInstance>
ElementTemplateInstance::ResolveElementTemplateInstanceValue(
    const lepus::Value& value) {
  if (!value.IsRefCounted()) {
    return nullptr;
  }
  auto ref = value.RefCounted();
  if (ref->GetRefType() != lepus::RefType::kElementTemplate) {
    return nullptr;
  }
  return fml::static_ref_ptr_cast<ElementTemplateInstance>(ref).strongify();
}

lepus::Value ElementTemplateInstance::ConvertTemplateValueForStorage(
    const lepus::Value& value) {
  return value.IsCallable() ? value : lepus::Value::Clone(value);
}

lepus::Value ElementTemplateInstance::ConvertTemplateMapForStorage(
    const lepus::Value& value) {
  return ConvertTemplateMapForStorageInternal(
      value, /*preserve_spread_callbacks=*/false);
}

bool ElementTemplateInstance::IsTemplateMapValue(const lepus::Value& value) {
  if (value.IsTable()) {
    return true;
  }
  return value.IsJSValue() && value.IsJSTable() && !value.IsJSArray() &&
         !value.IsJSFunction();
}

size_t ElementTemplateInstance::FindSlotChildIndex(
    const lepus::Value& slot_children, const lepus::Value& ref_node) {
  if (!ref_node.IsRefCounted()) {
    return static_cast<size_t>(slot_children.GetLength());
  }
  for (size_t i = 0; i < static_cast<size_t>(slot_children.GetLength()); ++i) {
    if (slot_children.GetProperty(static_cast<uint32_t>(i)).IsEqual(ref_node)) {
      return i;
    }
  }
  return static_cast<size_t>(slot_children.GetLength());
}

ElementTemplateInstance::ElementTemplateInstance(
    ElementManager* element_manager)
    : element_manager_(element_manager),
      bundle_url_(BASE_STATIC_STRING(kDefaultTemplateBundleUrl)) {}

ElementTemplateInstance::~ElementTemplateInstance() {
  ClearLogicalChildParentLinks();
}

void ElementTemplateInstance::SetTypedTag(const base::String& typed_tag) {
  typed_tag_ = typed_tag;
}

void ElementTemplateInstance::SetAttributes(const lepus::Value& attributes) {
  if (!IsTemplateMapValue(attributes) && !attributes.IsNil() &&
      !attributes.IsUndefined()) {
    return;
  }
  auto next_slots = lepus::CArray::Create();
  next_slots->emplace_back(IsTemplateMapValue(attributes)
                               ? ConvertTemplateMapForStorage(attributes)
                               : lepus::Value());
  auto previous_slots = std::move(attribute_slots_);
  attribute_slots_ = lepus::Value(std::move(next_slots));
  if (IsMaterialized()) {
    TreeResolver::ApplyTemplateAttributesToElement(
        result_.get(), previous_slots, attribute_slots_);
    NotifyTemplateAttributesChangedForInspector(element_manager_,
                                                result_.get());
  }
}

void ElementTemplateInstance::CollectCallbackAttributeSlots(
    const std::vector<ElementInfo>& elements) {
  for (const auto& element : elements) {
    if (element.attributes_ != nullptr) {
      for (const auto& attr : *element.attributes_) {
        if (attr.type_ == ATTRIBUTE_BINDING_TYPE_DYNAMIC &&
            (TreeResolver::IsTemplateGestureAttribute(attr.key_) ||
             TreeResolver::IsTemplateEventAttribute(attr.key_))) {
          callback_attribute_slots_->push_back(attr.slot_index_);
        } else if (attr.type_ == ATTRIBUTE_BINDING_TYPE_SPREAD) {
          spread_attribute_slots_.push_back(attr.slot_index_);
        }
      }
    }
    CollectCallbackAttributeSlots(element.children_);
  }
}

lepus::Value ElementTemplateInstance::CopyAttributeSlotForStorage(
    uint32_t slot_index, const lepus::Value& value) {
  if (!callback_attribute_slots_.has_value() && !IsTypedTemplate() &&
      tasm_ != nullptr) {
    // Only live callback candidates need metadata. Ordinary writes must not
    // force cold template decoding.
    if (IsLiveTemplateEventValue(value) || IsLiveTemplateGestureValue(value) ||
        HasLiveSpreadDescriptorEntry(value)) {
      const auto entry = tasm_->FindTemplateEntry(bundle_url_.str());
      if (entry != nullptr) {
        callback_attribute_slots_.emplace();
        const auto& info = entry->GetElementTemplateInfo(template_key_.str());
        CollectCallbackAttributeSlots(info.elements_);
      }
    }
  }
  if (!value.IsEmpty() && callback_attribute_slots_.has_value() &&
      std::find(callback_attribute_slots_->begin(),
                callback_attribute_slots_->end(),
                slot_index) != callback_attribute_slots_->end() &&
      (value.IsCallable() || IsTemplateMapValue(value))) {
    return value;
  }
  const bool is_spread_slot =
      std::find(spread_attribute_slots_.begin(), spread_attribute_slots_.end(),
                slot_index) != spread_attribute_slots_.end();
  return IsTemplateMapValue(value)
             ? ConvertTemplateMapForStorageInternal(value, is_spread_slot)
             : lepus::Value::Clone(value);
}

void ElementTemplateInstance::SetAttributeSlots(
    const lepus::Value& attribute_slots) {
  lepus::Value copied_attribute_slots;
  if (attribute_slots.IsArrayOrJSArray()) {
    auto copied_slots = lepus::CArray::Create();
    const auto length = static_cast<uint32_t>(attribute_slots.GetLength());
    copied_slots->reserve(length);
    for (uint32_t index = 0; index < length; ++index) {
      copied_slots->emplace_back(CopyAttributeSlotForStorage(
          index, attribute_slots.GetProperty(index)));
    }
    copied_attribute_slots = lepus::Value(std::move(copied_slots));
  }
  attribute_slots_ = std::move(copied_attribute_slots);
  ++attribute_slots_generation_;
}

void ElementTemplateInstance::InitializeChildSlots(
    const lepus::Value& child_slots) {
  if (!child_slots.IsArrayOrJSArray()) {
    child_slots_ = child_slots;
    return;
  }

  // Rebuild both array levels so later logical mutations do not alias the
  // caller's arrays. Sequential inserts also establish single-parent ownership.
  const size_t slot_count = static_cast<size_t>(child_slots.GetLength());
  auto stored_slots = lepus::CArray::Create();
  stored_slots->reserve(slot_count);
  child_slots_ = lepus::Value(stored_slots);
  for (size_t slot_index = 0; slot_index < slot_count; ++slot_index) {
    auto slot_children =
        child_slots.GetProperty(static_cast<uint32_t>(slot_index));
    if (!slot_children.IsArrayOrJSArray()) {
      /*
       * Preserve non-array values to keep subsequent slot indices unchanged.
       * For example, undefined is not a children array
       * but still occupies slot 0:
       *
       *   child_slots = [
       *     undefined,     // No children provided for slot 0.
       *     [itemA, itemB] // Slot 1 has two children.
       *   ];
       */
      stored_slots->emplace_back(std::move(slot_children));
      continue;
    }
    auto stored_children = lepus::CArray::Create();
    stored_slots->emplace_back(stored_children);
    const size_t child_count = static_cast<size_t>(slot_children.GetLength());
    for (size_t child_index = 0; child_index < child_count; ++child_index) {
      auto child_value =
          slot_children.GetProperty(static_cast<uint32_t>(child_index));
      if (child_value.IsNil() || child_value.IsUndefined() ||
          child_value.IsEmpty()) {
        stored_children->emplace_back(std::move(child_value));
        continue;
      }
      InsertNodeIntoChildSlot(static_cast<uint32_t>(slot_index), child_value,
                              lepus::Value());
    }
  }
}

void ElementTemplateInstance::SetOptions(const lepus::Value& options) {
  // The creation entry point has converted JS options to a native Lepus value.
  // Store __listItemPlatformInfo with the options for later List diff
  // collection.
  options_ = options;
}

void ElementTemplateInstance::SetUid(const lepus::Value& uid) { uid_ = uid; }

void ElementTemplateInstance::RequestMaterializationRecursively() {
  if (materialization_requested_) {
    return;
  }
  materialization_requested_ = true;
  EnsureCreateElementTreeTaskScheduled();
  // This ET's materialization request is already registered above. Decide only
  // whether to recurse into child ETs. The virtual
  // ShouldMaterializeChildSlots() returns true for regular ETs and false for
  // List ETs, so materializing a parent tree does not request every List item
  // at once. ComponentAtIndex materializes each item through GetRoot() when
  // requested by index.
  if (!child_slots_.IsArrayOrJSArray() || !ShouldMaterializeChildSlots()) {
    return;
  }
  for (size_t slot_index = 0;
       slot_index < static_cast<size_t>(child_slots_.GetLength());
       ++slot_index) {
    auto slot_children =
        child_slots_.GetProperty(static_cast<uint32_t>(slot_index));
    if (!slot_children.IsArrayOrJSArray()) {
      continue;
    }
    for (size_t child_index = 0;
         child_index < static_cast<size_t>(slot_children.GetLength());
         ++child_index) {
      auto child = ResolveElementTemplateInstanceValue(
          slot_children.GetProperty(static_cast<uint32_t>(child_index)));
      if (child != nullptr) {
        child->RequestMaterializationRecursively();
      }
    }
  }
}

void ElementTemplateInstance::EnsureCreateElementTreeTaskScheduled() {
  if (IsTypedTemplate() || result_ != nullptr ||
      create_element_tree_task_ != nullptr) {
    return;
  }
  if (entry_ == nullptr && tasm_ != nullptr) {
    entry_ = tasm_->FindEntry(bundle_url_.str()).get();
  }

  create_element_tree_task_ = CreateElementTreeTask(entry_);
  element_manager_->EnqueuePostMTSRenderTask(
      base::closure([task = create_element_tree_task_]() { task->Run(); }));
}

base::OnceTaskRefptr<GeneratedElementsResult>
ElementTemplateInstance::CreateElementTreeTask(TemplateEntry* entry) {
  std::promise<GeneratedElementsResult> promise;
  auto future = promise.get_future();
  auto template_key = template_key_;
  auto attribute_slots = attribute_slots_;
  if (attribute_slots_.IsArray()) {
    // Detached preparation never consumes callbacks. Keep their live worklet
    // contexts and functions on MTS, including when a worker outlives this
    // logical instance.
    const auto length = static_cast<uint32_t>(attribute_slots_.GetLength());
    auto is_callback_slot = [this](uint32_t index) {
      return callback_attribute_slots_.has_value() &&
             std::find(callback_attribute_slots_->begin(),
                       callback_attribute_slots_->end(),
                       index) != callback_attribute_slots_->end();
    };
    auto is_spread_slot = [this](uint32_t index) {
      return std::find(spread_attribute_slots_.begin(),
                       spread_attribute_slots_.end(),
                       index) != spread_attribute_slots_.end();
    };
    auto preparation_slots = lepus::CArray::Create();
    preparation_slots->reserve(length);
    bool modified = false;
    for (uint32_t index = 0; index < length; ++index) {
      if (is_callback_slot(index)) {
        preparation_slots->emplace_back();
        modified = true;
        continue;
      }
      preparation_slots->emplace_back(
          CopySlotForWorkerPreparation(attribute_slots_.GetProperty(index),
                                       is_spread_slot(index), &modified));
    }
    if (modified) {
      attribute_slots = lepus::Value(std::move(preparation_slots));
    }
  }
  auto attribute_slots_generation = attribute_slots_generation_;
  return fml::MakeRefCounted<base::OnceTask<GeneratedElementsResult>>(
      [entry, template_key = std::move(template_key),
       attribute_slots = std::move(attribute_slots), attribute_slots_generation,
       promise = std::move(promise)]() mutable {
        promise.set_value(GeneratePreparedElementsResult(
            entry, template_key, attribute_slots, attribute_slots_generation));
      },
      std::move(future));
}

void ElementTemplateInstance::MaterializeRoot() {
  if (IsMaterialized()) {
    return;
  }

  if (IsTypedTemplate()) {
    InitTypedRoot();
    if (result_ == nullptr) {
      return;
    }
    TreeResolver::ApplyTemplateAttributesToElement(result_.get(),
                                                   attribute_slots_);
    MountInitialChildSlots();
    return;
  }

  if (create_element_tree_task_ == nullptr) {
    EnsureCreateElementTreeTaskScheduled();
    if (create_element_tree_task_ == nullptr) {
      return;
    }
  }

  create_element_tree_task_->Run();
  auto generated = create_element_tree_task_->GetFuture().get();
  create_element_tree_task_ = nullptr;
  if (generated.result_ == nullptr) {
    return;
  }
  result_ = std::move(generated.result_);
  attribute_slot_targets_ = std::move(generated.attribute_slot_targets_);
  event_attribute_slot_targets_ =
      std::move(generated.event_attribute_slot_targets_);
  static_event_targets_ = std::move(generated.static_event_targets_);
  element_slot_targets_ = std::move(generated.element_slot_targets_);

  InitGeneratedElementTree(generated.prepared_attribute_slots_,
                           generated.attribute_slots_generation_);
  MountInitialChildSlots();
}

void ElementTemplateInstance::InitGeneratedElementTree(
    const lepus::Value& prepared_attribute_slots,
    uint32_t prepared_attribute_slots_generation) {
  if (result_ == nullptr || entry_ == nullptr) {
    return;
  }
  if (prepared_attribute_slots_generation != attribute_slots_generation_) {
    ApplyInitialAttributeSlots(
        attribute_slot_targets_, attribute_slots_,
        [&prepared_attribute_slots](Element* element,
                                    const lepus::Value& slots) {
          TreeResolver::ApplyTemplateNonEventAttributesToElement(
              element, prepared_attribute_slots, slots);
        });
  }
  // Resolve root scope while detached, before page CSS can become the fallback.
  // Child-slot instances are mounted later and retain their own entry/scope.
  InitCompiledElementCSS(result_.get(), entry_->GetStyleSheetManager(),
                         kDefaultPageCSSId);
  auto* root = element_manager_->root();
  TreeResolver::InitElementTree(result_, root != nullptr ? root->impl_id() : -1,
                                element_manager_,
                                entry_->GetStyleSheetManager());
  ApplyStaticEventAttributes(static_event_targets_);
  ApplyInitialEventAttributeSlots(event_attribute_slot_targets_,
                                  attribute_slots_);
}

void ElementTemplateInstance::InitTypedRoot() {
  if (!IsTypedTemplate() || result_ != nullptr) {
    return;
  }

  result_ = CreateTypedRootElement(element_manager_, tasm_, typed_tag_);
  if (result_ == nullptr) {
    return;
  }
  result_->MarkTemplateElement();
  result_->SetTemplateAttributes(TypedTemplateAttributes());
  auto* root = element_manager_->root();
  if (root != nullptr) {
    result_->SetParentComponentUniqueIdRecursively(root->impl_id());
  }

  element_slot_targets_.clear();
  element_slot_targets_.push_back(ElementSlotMountPoint{result_, nullptr});
}

fml::RefPtr<Element> ElementTemplateInstance::GetRoot() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_ELEMENT_GET_ROOT,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  RequestMaterializationRecursively();
  MaterializeRoot();

  EXEC_EXPR_FOR_INSPECTOR(auto* manager = element_manager_;
                          if (result_ != nullptr && manager->GetDevToolFlag() &&
                              manager->IsDomTreeEnabled()) {
                            PrepareMaterializedElementTreeForInspector(
                                manager, result_.get());
                          });
  return result_;
}

fml::RefPtr<Element> ElementTemplateInstance::PeekMaterializedRoot() const {
  return result_;
}

fml::RefPtr<ElementTemplateInstance>
ElementTemplateInstance::GetChildInstanceAt(uint32_t slot_index,
                                            uint32_t child_index) const {
  if (!child_slots_.IsArrayOrJSArray() ||
      slot_index >= static_cast<uint32_t>(child_slots_.GetLength())) {
    return nullptr;
  }
  auto slot_children = child_slots_.GetProperty(slot_index);
  if (!slot_children.IsArrayOrJSArray() ||
      child_index >= static_cast<uint32_t>(slot_children.GetLength())) {
    return nullptr;
  }
  return ResolveElementTemplateInstanceValue(
      slot_children.GetProperty(child_index));
}

fml::RefPtr<Element> ElementTemplateInstance::GetElementByAttrSlot(
    uint32_t slot_index) {
  if (!IsMaterialized()) {
    GetRoot();
  }
  if (IsTypedTemplate()) {
    return slot_index == kTypedTemplateAttributeSlotIndex ? result_ : nullptr;
  }
  if (slot_index >= attribute_slot_targets_.size()) {
    return nullptr;
  }
  return attribute_slot_targets_[slot_index];
}

void ElementTemplateInstance::ApplyAttributeSlotToTarget(
    uint32_t slot_index, const lepus::Value& previous_attribute_slots) {
  if (slot_index >= attribute_slot_targets_.size()) {
    return;
  }
  const auto& target = attribute_slot_targets_[slot_index];
  if (target == nullptr) {
    return;
  }
  TreeResolver::ApplyTemplateAttributesToElement(
      target.get(), previous_attribute_slots, attribute_slots_, slot_index);
  NotifyTemplateAttributesChangedForInspector(element_manager_, target.get());
}

lepus::Value ElementTemplateInstance::GetOrCreateMutableChildSlot(
    uint32_t slot_index) {
  if (!child_slots_.IsArray()) {
    child_slots_ = lepus::Value(lepus::CArray::Create());
  }
  auto slot_children = child_slots_.GetProperty(slot_index);
  if (!slot_children.IsArray()) {
    slot_children = lepus::Value(lepus::CArray::Create());
    child_slots_.SetProperty(slot_index, slot_children);
  }
  return slot_children;
}

bool ElementTemplateInstance::EraseChildFromSlotStorage(
    uint32_t slot_index, const lepus::Value& child) {
  if (!child_slots_.IsArray() ||
      slot_index >= static_cast<uint32_t>(child_slots_.GetLength())) {
    return false;
  }
  auto slot_children = child_slots_.GetProperty(slot_index);
  if (!slot_children.IsArray()) {
    return false;
  }
  auto array = slot_children.Array();
  for (size_t i = 0; i < array->size(); ++i) {
    if (array->get(i).IsEqual(child)) {
      array->Erase(static_cast<uint32_t>(i));
      return true;
    }
  }
  return false;
}

bool ElementTemplateInstance::HasPendingChildMounts() const {
  return !pending_child_mount_slots_.empty();
}

ElementTemplateInstance::FlushPendingChildMountsResult
ElementTemplateInstance::FlushPendingChildMounts(Element* flush_root) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              ELEMENT_TEMPLATE_INSTANCE_FLUSH_PENDING_CHILD_MOUNTS,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  if (!IsMaterializedRootInFlushScope(flush_root)) {
    // A nested ET in a detached subtree still has a physical parent. Keep it
    // queued so reattaching the subtree can bring it back into flush scope.
    const bool has_physical_parent_or_is_root =
        result_->parent() != nullptr ||
        element_manager_->root() == result_.get();
    return has_physical_parent_or_is_root
               ? FlushPendingChildMountsResult::kOutOfScope
               : FlushPendingChildMountsResult::kDoNotRequeue;
  }

  auto pending_slots = std::move(pending_child_mount_slots_);
  pending_child_mount_slots_.clear();
  for (auto slot_index : pending_slots) {
    if (!MountChildSlot(slot_index, true)) {
      AddPendingSlot(pending_child_mount_slots_, slot_index);
    }
  }
  return FlushPendingChildMountsResult::kDoNotRequeue;
}

void ElementTemplateInstance::SchedulePendingChildMounts() {
  if (!HasPendingChildMounts() || result_ == nullptr) {
    return;
  }
  element_manager_->EnqueuePendingElementTemplateChildMounts(*this);
  result_->MarkDirty(Element::kDirtyTree);
}

bool ElementTemplateInstance::IsMaterializedRootInFlushScope(
    Element* flush_root) const {
  auto* current = result_.get();
  while (current != nullptr) {
    if (current == flush_root) {
      return true;
    }
    current = static_cast<Element*>(current->parent());
  }
  return false;
}

void ElementTemplateInstance::MountInitialChildSlots() {
  // Child slots with deferred materialization skip regular mounting and do not
  // register pending mount tasks.
  if (!ShouldMaterializeChildSlots()) {
    return;
  }
  if (!child_slots_.IsArrayOrJSArray()) {
    return;
  }
  const size_t slot_count =
      std::min(static_cast<size_t>(child_slots_.GetLength()),
               element_slot_targets_.size());
  for (size_t slot_index = 0; slot_index < slot_count; ++slot_index) {
    if (!MountChildSlot(static_cast<uint32_t>(slot_index), false)) {
      AddPendingSlot(pending_child_mount_slots_,
                     static_cast<uint32_t>(slot_index));
    }
  }
  SchedulePendingChildMounts();
}

bool ElementTemplateInstance::MountChildSlot(uint32_t slot_index,
                                             bool resolve_compiled_children) {
  if (slot_index >= element_slot_targets_.size() ||
      !child_slots_.IsArrayOrJSArray() ||
      slot_index >= static_cast<uint32_t>(child_slots_.GetLength())) {
    return true;
  }
  const auto& mount_point = element_slot_targets_[slot_index];
  auto slot_children = child_slots_.GetProperty(slot_index);
  if (mount_point.parent_ == nullptr || !slot_children.IsArrayOrJSArray()) {
    return true;
  }

  bool mounted_all = true;
  // Initial slots mount in producer order, so later slot children are not yet
  // attached to this mount point. Pending mounts must instead locate their
  // current logical successor.
  auto insertion_reference =
      resolve_compiled_children
          ? FindChildInsertionReference(
                slot_index, static_cast<size_t>(slot_children.GetLength()))
          : mount_point.ref_node_;
  for (size_t index = static_cast<size_t>(slot_children.GetLength()); index > 0;
       --index) {
    auto child = ResolveElementTemplateInstanceValue(
        slot_children.GetProperty(static_cast<uint32_t>(index - 1)));
    if (child == nullptr) {
      continue;
    }
    const bool is_typed_child = child->IsTypedTemplate();
    auto child_root = child->PeekMaterializedRoot();
    if (child_root == nullptr &&
        (is_typed_child || resolve_compiled_children)) {
      child_root = child->GetRoot();
    }
    if (child_root == nullptr) {
      if (!is_typed_child) {
        mounted_all = false;
      }
      continue;
    }
    MountMaterializedChildBefore(mount_point, child, child_root,
                                 insertion_reference);
    insertion_reference = std::move(child_root);
  }
  return mounted_all;
}

void ElementTemplateInstance::MountMaterializedChildBefore(
    const ElementSlotMountPoint& mount_point,
    const fml::RefPtr<ElementTemplateInstance>& child,
    const fml::RefPtr<Element>& child_root,
    const fml::RefPtr<Element>& insertion_reference) {
  if (child_root.get() == insertion_reference.get()) {
    return;
  }
  child->SchedulePendingChildMounts();
  if (child_root->parent() == mount_point.parent_.get() &&
      child_root->next_sibling() == insertion_reference.get()) {
    return;
  }

  DetachMaterializedElementFromCurrentParent(element_manager_, child_root);
  if (insertion_reference != nullptr) {
    mount_point.parent_->InsertNodeBefore(child_root, insertion_reference);
  } else {
    mount_point.parent_->InsertNode(child_root);
  }
  EXEC_EXPR_FOR_INSPECTOR({
    element_manager_->CheckAndProcessSlotForInspector(child_root.get());
    element_manager_->OnElementNodeAddedForInspector(child_root.get());
  });
}

void ElementTemplateInstance::UnmountMaterializedChild(
    uint32_t slot_index, const fml::RefPtr<ElementTemplateInstance>& child) {
  if (slot_index >= element_slot_targets_.size() || child == nullptr) {
    return;
  }
  auto child_root = child->PeekMaterializedRoot();
  const auto& mount_point = element_slot_targets_[slot_index];
  if (child_root != nullptr && mount_point.parent_ != nullptr &&
      child_root->parent() == mount_point.parent_.get()) {
    DetachMaterializedElementFromCurrentParent(element_manager_, child_root);
  }
}

fml::RefPtr<Element> ElementTemplateInstance::FindChildInsertionReference(
    uint32_t slot_index, size_t first_sibling_index) const {
  // Find a physical insert-before anchor without materializing pending
  // siblings. Single-child inserts search from the next sibling; whole-slot
  // mounts pass the slot length and build their within-slot order from back to
  // front.
  const auto& mount_point = element_slot_targets_[slot_index];
  auto slot_children = child_slots_.GetProperty(slot_index);
  for (size_t index = first_sibling_index;
       index < static_cast<size_t>(slot_children.GetLength()); ++index) {
    auto sibling = ResolveElementTemplateInstanceValue(
        slot_children.GetProperty(static_cast<uint32_t>(index)));
    if (sibling == nullptr) {
      continue;
    }
    // A logical successor may still be pending or attached to another parent.
    // Only an Element already under this mount point can serve as an anchor.
    auto sibling_root = sibling->PeekMaterializedRoot();
    if (sibling_root != nullptr &&
        sibling_root->parent() == mount_point.parent_.get()) {
      return sibling_root;
    }
  }

  // Adjacent slots can share the same parent and static reference. Their
  // mounted children must stay after this slot: [slot0: A, slot1: B, static S]
  // needs B, not S, as the anchor when appending a child to slot0.
  const size_t logical_slot_count =
      static_cast<size_t>(child_slots_.GetLength());
  for (size_t next_slot_index = slot_index + 1;
       next_slot_index < element_slot_targets_.size() &&
       next_slot_index < logical_slot_count;
       ++next_slot_index) {
    const auto& next_mount_point = element_slot_targets_[next_slot_index];
    if (next_mount_point.parent_.get() != mount_point.parent_.get() ||
        next_mount_point.ref_node_.get() != mount_point.ref_node_.get()) {
      continue;
    }
    auto next_slot_children =
        child_slots_.GetProperty(static_cast<uint32_t>(next_slot_index));
    if (!next_slot_children.IsArrayOrJSArray()) {
      continue;
    }
    for (size_t index = 0;
         index < static_cast<size_t>(next_slot_children.GetLength()); ++index) {
      auto sibling = ResolveElementTemplateInstanceValue(
          next_slot_children.GetProperty(static_cast<uint32_t>(index)));
      if (sibling == nullptr) {
        continue;
      }
      auto sibling_root = sibling->PeekMaterializedRoot();
      if (sibling_root != nullptr &&
          sibling_root->parent() == mount_point.parent_.get()) {
        return sibling_root;
      }
    }
  }
  // No mounted logical successor: insert before the static reference, or append
  // to the parent when the reference is null.
  return mount_point.ref_node_;
}

void ElementTemplateInstance::ClearLogicalChildParentLinks() {
  if (!child_slots_.IsArrayOrJSArray()) {
    return;
  }
  for (size_t slot_index = 0;
       slot_index < static_cast<size_t>(child_slots_.GetLength());
       ++slot_index) {
    auto slot_children =
        child_slots_.GetProperty(static_cast<uint32_t>(slot_index));
    if (!slot_children.IsArrayOrJSArray()) {
      continue;
    }
    for (size_t child_index = 0;
         child_index < static_cast<size_t>(slot_children.GetLength());
         ++child_index) {
      auto child = ResolveElementTemplateInstanceValue(
          slot_children.GetProperty(static_cast<uint32_t>(child_index)));
      if (child != nullptr && child->logical_parent_ == this) {
        child->logical_parent_ = nullptr;
        child->logical_parent_slot_index_ = 0;
      }
    }
  }
}

lepus::Value ElementTemplateInstance::Serialize() const {
  return ElementTemplateInstanceSerializer().Serialize(*this);
}

void ElementTemplateInstance::SetAttributeSlot(uint32_t slot_index,
                                               const lepus::Value& value) {
  if (IsTypedTemplate()) {
    if (slot_index == kTypedTemplateAttributeSlotIndex) {
      SetAttributes(value);
    }
    return;
  }

  auto previous_attribute_slots = std::move(attribute_slots_);
  auto next_slots = lepus::CArray::Create();
  if (previous_attribute_slots.IsArrayOrJSArray()) {
    next_slots->reserve(previous_attribute_slots.GetLength());
    for (size_t index = 0;
         index < static_cast<size_t>(previous_attribute_slots.GetLength());
         ++index) {
      next_slots->emplace_back(
          previous_attribute_slots.GetProperty(static_cast<uint32_t>(index)));
    }
  }
  attribute_slots_ = lepus::Value(std::move(next_slots));
  attribute_slots_.SetProperty(slot_index,
                               CopyAttributeSlotForStorage(slot_index, value));
  ++attribute_slots_generation_;
  if (IsMaterialized()) {
    ApplyAttributeSlotToTarget(slot_index, previous_attribute_slots);
  }
}

void ElementTemplateInstance::InsertNodeIntoChildSlot(
    uint32_t slot_index, const lepus::Value& child,
    const lepus::Value& ref_node) {
  auto child_instance = ResolveElementTemplateInstanceValue(child);
  if (child_instance == nullptr || child.IsEqual(ref_node)) {
    return;
  }
  if (IsTypedTemplate() && slot_index != kTypedTemplateRootChildSlotIndex) {
    // Typed templates, such as "page" and "list", only accept children in slot
    // 0.
    return;
  }

  auto* previous_parent = child_instance->logical_parent_;
  auto previous_slot_index = child_instance->logical_parent_slot_index_;
  auto insert_index =
      InsertLogicalChildIntoSlot(slot_index, child, ref_node, *child_instance);
  if (materialization_requested_) {
    child_instance->RequestMaterializationRecursively();
  }

  if (IsMaterialized()) {
    const bool is_typed_child = child_instance->IsTypedTemplate();
    auto child_root = child_instance->PeekMaterializedRoot();
    if (child_root == nullptr && is_typed_child) {
      child_root = child_instance->GetRoot();
    }
    if (child_root != nullptr) {
      if (slot_index < element_slot_targets_.size() &&
          element_slot_targets_[slot_index].parent_ != nullptr) {
        MountMaterializedChildBefore(
            element_slot_targets_[slot_index], child_instance, child_root,
            FindChildInsertionReference(slot_index, insert_index + 1));
      } else if (previous_parent != nullptr &&
                 previous_parent->IsMaterialized()) {
        previous_parent->UnmountMaterializedChild(previous_slot_index,
                                                  child_instance);
      }
    } else if (!is_typed_child) {
      AddPendingSlot(pending_child_mount_slots_, slot_index);
      SchedulePendingChildMounts();
    }
  } else if (previous_parent != nullptr && previous_parent->IsMaterialized()) {
    previous_parent->UnmountMaterializedChild(previous_slot_index,
                                              child_instance);
  }
}

size_t ElementTemplateInstance::InsertLogicalChildIntoSlot(
    uint32_t slot_index, const lepus::Value& child_node,
    const lepus::Value& ref_node, ElementTemplateInstance& child_instance) {
  auto* previous_parent = child_instance.logical_parent_;
  if (previous_parent != nullptr) {
    // Remove the child from its previous parent's slot before reinserting it,
    // even when the previous parent is this instance.
    previous_parent->EraseChildFromSlotStorage(
        child_instance.logical_parent_slot_index_, child_node);
  }

  // Get or create this parent's target slot array, then insert the child.
  auto slot_children = GetOrCreateMutableChildSlot(slot_index);
  auto insert_index = FindSlotChildIndex(slot_children, ref_node);
  slot_children.Array()->Insert(static_cast<uint32_t>(insert_index),
                                child_node);

  // Update the child's links to its new logical parent and slot.
  child_instance.logical_parent_ = this;
  child_instance.logical_parent_slot_index_ = slot_index;
  return insert_index;
}

void ElementTemplateInstance::RemoveNodeFromChildSlot(
    uint32_t slot_index, const lepus::Value& child) {
  auto child_instance = ResolveElementTemplateInstanceValue(child);
  if (child_instance == nullptr) {
    return;
  }
  if (IsTypedTemplate() && slot_index != kTypedTemplateRootChildSlotIndex) {
    return;
  }
  if (!EraseChildFromSlotStorage(slot_index, child)) {
    return;
  }
  child_instance->logical_parent_ = nullptr;
  child_instance->logical_parent_slot_index_ = 0;
  if (IsMaterialized()) {
    UnmountMaterializedChild(slot_index, child_instance);
  }
}

void ElementTemplateInstance::SetListItemPlatformInfo(
    const lepus::Value& platform_info) {
  if (!IsTemplateMapValue(platform_info) && !platform_info.IsNil() &&
      !platform_info.IsUndefined()) {
    return;
  }
  auto next_info = IsTemplateMapValue(platform_info)
                       ? ConvertTemplateMapForStorage(platform_info)
                       : lepus::Value(lepus::Dictionary::Create());
  if (GetListItemPlatformInfoFromOptions().IsEqual(next_info)) {
    return;
  }
  // Replace only the platform information, preserving other options without
  // modifying the shared object passed at creation.
  auto next_options = lepus::Dictionary::Create();
  if (options_.IsObject()) {
    lepus::Value::ForEachLepusValue(
        options_,
        [&next_options](const lepus::Value& key, const lepus::Value& value) {
          if (key.IsString()) {
            next_options->SetValue(key.String(), value);
          }
        });
  }
  next_options->SetValue(BASE_STATIC_STRING(kListItemPlatformInfo), next_info);
  options_ = lepus::Value(std::move(next_options));
  // Items not yet inserted into a List only store the information. A later
  // insertAction will read the latest value.
  if (logical_parent_ != nullptr &&
      logical_parent_slot_index_ == kTypedTemplateRootChildSlotIndex) {
    logical_parent_->OnListItemPlatformInfoChanged(
        lepus::Value(fml::RefPtr<ElementTemplateInstance>(this)), next_info);
  }
}

base::String ElementTemplateInstance::GetTemplateType() const {
  if (IsTypedTemplate()) {
    return typed_tag_;
  }
  if (bundle_url_.empty() || bundle_url_.IsEqual(kDefaultTemplateBundleUrl)) {
    return template_key_;
  }
  return base::String(bundle_url_.str() + ":" + template_key_.str());
}

// List metadata: a regular ElementTemplateInstance can also be a List item.
lepus::Value ElementTemplateInstance::GetListItemPlatformInfoFromOptions()
    const {
  // List items currently use regular ET instances, so read their platform
  // metadata from options_. This does not depend on attribute_slots_ or require
  // materializing the item's Element.
  if (!options_.IsObject()) {
    return lepus::Value();
  }
  auto list_item_platform_info =
      options_.GetProperty(BASE_STATIC_STRING(kListItemPlatformInfo));
  // Regular templates do not have this internal field. Preserve the distinction
  // between an empty object and missing metadata.
  return list_item_platform_info.IsObject() ? list_item_platform_info
                                            : lepus::Value();
}

bool ElementTemplateInstance::ReuseElementsFrom(
    ElementTemplateInstance& before_instance) {
  if (!HydrateImpl(before_instance, *this)) {
    return false;
  }
  before_instance.ResetMaterializedElements();
  return true;
}

bool ElementTemplateInstance::HydrateImpl(
    ElementTemplateInstance& before_instance,
    ElementTemplateInstance& after_instance) {
  // 1. Complete eligibility checks before changing the source's physical state.
  if (&before_instance == &after_instance) {
    // For the same instance, only report whether it has a physical root.
    return after_instance.result_ != nullptr;
  }

  const auto is_same_type = [](const ElementTemplateInstance& a,
                               const ElementTemplateInstance& b) {
    return a.IsTypedTemplate() == b.IsTypedTemplate() &&
           a.GetTemplateType() == b.GetTemplateType();
  };

  if (after_instance.result_ != nullptr || before_instance.result_ == nullptr ||
      !is_same_type(before_instance, after_instance)) {
    // External template types include bundleUrl:templateKey, so different
    // bundles are normally excluded.
    return false;
  }

  // TODO: Support nested List reuse in a follow-up change. Reject trees with
  // nested Lists until their reuse is supported.
  if (before_instance.ContainsListTemplate() ||
      after_instance.ContainsListTemplate()) {
    return false;
  }

  // 2. Temporarily share the physical root and transfer target tables to
  // after_instance. The old instance retains the same root until the outermost
  // caller recursively clears its physical bindings.
  after_instance.result_ = before_instance.result_;
  after_instance.create_element_tree_task_ = nullptr;
  after_instance.pending_child_mount_slots_.clear();
  after_instance.element_slot_targets_ =
      std::move(before_instance.element_slot_targets_);
  after_instance.attribute_slot_targets_ =
      std::move(before_instance.attribute_slot_targets_);
  after_instance.event_attribute_slot_targets_ =
      std::move(before_instance.event_attribute_slot_targets_);
  after_instance.static_event_targets_ =
      std::move(before_instance.static_event_targets_);
  if (after_instance.entry_ == nullptr) {
    // Logical ET creation does not immediately populate entry_. It is normally
    // resolved by bundle when physical creation is requested; see
    // EnsureCreateElementTreeTaskScheduled(). Hydration transfers the old
    // elements directly, bypassing physical creation and that entry lookup.
    after_instance.entry_ = before_instance.entry_;
  }
  after_instance.materialization_requested_ = true;

  // Pending mounts now follow the new instance's logical children; the old
  // instance no longer owns this work after the physical tree transfer.
  before_instance.pending_child_mount_slots_.clear();

  // 3. Update attributes and events from the old physical state. Remove fields
  // that disappeared from the old spread attributes.
  const auto previous_attribute_slots = before_instance.attribute_slots_;
  // TODO: Check for attribute changes and skip applying unchanged values.
  if (after_instance.IsTypedTemplate()) {
    TreeResolver::ApplyTemplateAttributesToElement(
        after_instance.result_.get(), previous_attribute_slots,
        after_instance.attribute_slots_);
    NotifyTemplateAttributesChangedForInspector(after_instance.element_manager_,
                                                after_instance.result_.get());
  } else {
    ApplyInitialAttributeSlots(
        after_instance.attribute_slot_targets_, after_instance.attribute_slots_,
        [&previous_attribute_slots, &after_instance](
            Element* element, const lepus::Value& slots) {
          TreeResolver::ApplyTemplateAttributesToElement(
              element, previous_attribute_slots, slots);
          NotifyTemplateAttributesChangedForInspector(
              after_instance.element_manager_, element);
        });
  }

  // 4. Recursively hydrate slot children.
  if (after_instance.ShouldMaterializeChildSlots()) {
    const int32_t before_slot_count =
        before_instance.child_slots_.IsArrayOrJSArray()
            ? before_instance.child_slots_.GetLength()
            : 0;
    const int32_t after_slot_count =
        after_instance.child_slots_.IsArrayOrJSArray()
            ? after_instance.child_slots_.GetLength()
            : 0;
    const int32_t slot_count = std::max(before_slot_count, after_slot_count);

    struct SlotDiff {
      int32_t slot_index{-1};
      const ElementSlotMountPoint* mount_point = nullptr;
      base::Vector<fml::RefPtr<ElementTemplateInstance>> before_children;
      SlotChildrenDiffResult result;
    };

    base::Vector<SlotDiff> slot_diffs;
    slot_diffs.reserve(slot_count);

    for (int32_t slot_index = 0; slot_index < slot_count; ++slot_index) {
      // CollectSlotChildren filters empty entries:
      // [null, oldA, undefined, oldB, oldC] -> [oldA, oldB, oldC].
      // DiffSlotChildren therefore uses indices in the filtered child arrays.
      auto before_children =
          CollectSlotChildren(before_instance.child_slots_, slot_index);
      auto after_children =
          CollectSlotChildren(after_instance.child_slots_, slot_index);
      const ElementSlotMountPoint* mount_point = nullptr;
      const Element* mount_parent = nullptr;
      if (slot_index <
          static_cast<int32_t>(after_instance.element_slot_targets_.size())) {
        mount_point = &(after_instance.element_slot_targets_[slot_index]);
        if (mount_point) {
          mount_parent = mount_point->parent_.get();
        }
      }
      const auto children_size = before_children.size();
      if (children_size == after_children.size()) {
        // Check whether this slot's old and new children can be reused pairwise
        // at matching indices.
        bool same_pairwise = true;
        for (size_t index = 0; index < children_size; ++index) {
          const auto& before_child = before_children[index];
          const auto& after_child = after_children[index];
          const auto before_root = before_child->PeekMaterializedRoot();
          const auto after_root = after_child->PeekMaterializedRoot();
          // The child counts already match. Each pair must also satisfy:
          // - The template types match.
          // - The old instance has a physical root.
          // - That root belongs to mount_parent when a parent is specified.
          // - The new instance has no physical root.
          // same_pairwise stays true only if every pair meets these conditions.
          if (!is_same_type(*before_child, *after_child) || !before_root ||
              (mount_parent && mount_parent != before_root->parent()) ||
              after_root) {
            // Fall back to DiffSlotChildren if any condition fails.
            same_pairwise = false;
            break;
          }
        }
        if (same_pairwise) {
          bool hydrated_pairwise = true;
          for (size_t index = 0; index < children_size; ++index) {
            if (!HydrateImpl(*before_children[index], *after_children[index])) {
              hydrated_pairwise = false;
              break;
            }
            after_children[index]->SchedulePendingChildMounts();
          }
          if (hydrated_pairwise) {
            // Skip this slot's diff once all children hydrate successfully.
            continue;
          }
        }
      }
      base::Vector<std::pair<size_t, size_t>> replacements;
      auto diff_result = DiffSlotChildren(
          before_children, after_children,
          [&](const fml::RefPtr<ElementTemplateInstance>& before_child,
              const fml::RefPtr<ElementTemplateInstance>& after_child) {
            return is_same_type(*before_child, *after_child);
          },
          [&](const fml::RefPtr<ElementTemplateInstance>& before_child,
              const fml::RefPtr<ElementTemplateInstance>& after_child,
              size_t before_index, size_t after_index) {
            // Attempt to transfer the old child's physical tree to the new
            // child; the return value indicates success.
            bool reused = HydrateImpl(*before_child, *after_child);
            const auto root = after_child->PeekMaterializedRoot();
            if (reused && root != nullptr &&
                (mount_parent == nullptr || root->parent() == mount_parent)) {
              return;
            }
            /*
             * If hydration fails, record a replacement for removal and
             * insertion. For example:
             *
             * Old logical tree:
             *   oldP: ElementP
             *     └─ oldB: ElementB
             * New logical tree:
             *   newP: not materialized
             *     └─ newB: already owns physical root NewElementB
             *
             * HydrateImpl(oldP, newP)
             *   ├─ newP takes over oldP's ElementP
             *   ├─ HydrateImpl(oldB, newB)
             *   │    → newB already has its own root
             *   │    → returns false before transferring B's physical tree
             *   ├─ parent records replacements = {(0, 0)}
             *   ├─ apply removal and insertion
             *   │    → detach oldB's physical root
             *   │    → insert newB's existing NewElementB
             *   └─ the oldP → newP call returns true
             *
             * Result:
             * {removes_ = {0}; inserts_ = {{0, newB}}; moves_ = {}}
             */
            replacements.emplace_back(before_index, after_index);
          },
          false);
      for (const auto& replacement : replacements) {
        diff_result.removes_.push_back(replacement.first);
        diff_result.inserts_[replacement.second] =
            after_children[replacement.second];
        ++diff_result.insert_count_;
        if (diff_result.moves_[replacement.first] !=
            SlotChildrenDiffResult::kNoMove) {
          diff_result.moves_[replacement.first] =
              SlotChildrenDiffResult::kNoMove;
          --diff_result.move_count_;
        }
      }
      slot_diffs.push_back(
          SlotDiff{.slot_index = slot_index,
                   .mount_point = mount_point,
                   .before_children = std::move(before_children),
                   .result = std::move(diff_result)});
    }
    // 5. Apply the insertions, removals, and moves from DiffSlotChildren.
    // Process slots in reverse so later slots establish physical anchors for
    // appending children to the current slot.
    for (size_t slot_diff_index = slot_diffs.size(); slot_diff_index > 0;
         --slot_diff_index) {
      const auto& slot_diff = slot_diffs[slot_diff_index - 1];
      const auto slot_index = static_cast<uint32_t>(slot_diff.slot_index);
      const auto* mount_point = slot_diff.mount_point;
      // Read the original slot children again.
      auto slot_children = lepus::Value();
      int32_t child_count = 0;
      if (after_instance.child_slots_.IsArrayOrJSArray()) {
        slot_children = after_instance.child_slots_.GetProperty(slot_index);
        if (slot_children.IsArrayOrJSArray()) {
          child_count = slot_children.GetLength();
        }
      }
      ApplySlotChildrenDiffResult(
          slot_diff.before_children, slot_diff.result,
          [mount_point, slot_index, child_count, &after_instance](
              const fml::RefPtr<ElementTemplateInstance>& child,
              const fml::RefPtr<ElementTemplateInstance>& target)
              -> fml::RefPtr<ElementTemplateInstance> {
            if (mount_point == nullptr || mount_point->parent_ == nullptr) {
              return child;
            }
            // An inserted child may already own a root or need materialization.
            auto root = child->PeekMaterializedRoot();
            if (root == nullptr) {
              root = child->GetRoot();
            }
            if (root == nullptr) {
              // This child has no physical root yet and cannot be inserted.
              // Mark its slot as pending so a later flush can retry the mount.
              AddPendingSlot(after_instance.pending_child_mount_slots_,
                             slot_index);
              return child;
            }
            // Find the physical insertion reference.
            fml::RefPtr<Element> reference_element = nullptr;
            if (target) {
              reference_element = target->PeekMaterializedRoot();
            }
            // Find a fallback reference if the intended target has no physical
            // root or its root is not attached to the destination parent.
            if (reference_element == nullptr ||
                reference_element->parent() != mount_point->parent_.get()) {
              // A null target denotes a slot-end insertion and needs no pending
              // mount. Retry only when a non-null target is unusable.
              if (target != nullptr) {
                AddPendingSlot(after_instance.pending_child_mount_slots_,
                               slot_index);
              }
              // Passing the full child_count skips this slot's children and
              // searches for the boundary to its right.
              reference_element = after_instance.FindChildInsertionReference(
                  slot_index, static_cast<size_t>(child_count));
            }
            after_instance.MountMaterializedChildBefore(
                *mount_point, child, root, reference_element);
            return child;
          },
          [&after_instance](const fml::RefPtr<ElementTemplateInstance>& child) {
            const auto root = child->PeekMaterializedRoot();
            if (root != nullptr) {
              DetachMaterializedElementFromCurrentParent(
                  after_instance.element_manager_, root);
            }
          },
          [mount_point, slot_index, child_count, &after_instance](
              const fml::RefPtr<ElementTemplateInstance>& child,
              const fml::RefPtr<ElementTemplateInstance>& target) {
            if (mount_point == nullptr || mount_point->parent_ == nullptr) {
              return;
            }
            const auto root = child->PeekMaterializedRoot();
            if (root == nullptr) {
              AddPendingSlot(after_instance.pending_child_mount_slots_,
                             slot_index);
              return;
            }
            fml::RefPtr<Element> reference_element = nullptr;
            if (target) {
              reference_element = target->PeekMaterializedRoot();
            }
            if (reference_element == nullptr ||
                reference_element->parent() != mount_point->parent_.get()) {
              // Find a fallback if the target has no physical root or its root
              // is not attached to the destination parent.
              // A null target denotes a slot-end insertion and needs no pending
              // mount.
              if (target != nullptr) {
                AddPendingSlot(after_instance.pending_child_mount_slots_,
                               slot_index);
              }
              // Passing the full child_count skips this slot's children and
              // searches for the boundary to its right.
              reference_element = after_instance.FindChildInsertionReference(
                  slot_index, static_cast<size_t>(child_count));
            }
            if (root.get() == reference_element.get() ||
                (root->parent() == mount_point->parent_.get() &&
                 root->next_sibling() == reference_element.get())) {
              // The root is the reference itself, or is already under the
              // destination parent immediately before the reference.
              return;
            }
            // Detach the same physical root and reinsert it at the destination.
            DetachMaterializedElementFromCurrentParent(
                after_instance.element_manager_, root);
            if (reference_element != nullptr) {
              mount_point->parent_->InsertNodeBefore(root, reference_element);
            } else {
              mount_point->parent_->InsertNode(root);
            }
            EXEC_EXPR_FOR_INSPECTOR({
              after_instance.element_manager_->CheckAndProcessSlotForInspector(
                  root.get());
              after_instance.element_manager_->OnElementNodeAddedForInspector(
                  root.get());
            });
          });
    }
  }
  // 6. Queue any remaining child mounts for a later flush.
  after_instance.SchedulePendingChildMounts();
  return true;
}

bool ElementTemplateInstance::ContainsListTemplate() const {
  if (IsTypedTemplate() && typed_tag_.IsEqual(kElementListTag)) {
    return true;
  }
  if (!child_slots_.IsArrayOrJSArray()) {
    return false;
  }
  for (size_t slot_index = 0;
       slot_index < static_cast<size_t>(child_slots_.GetLength());
       ++slot_index) {
    auto children = child_slots_.GetProperty(static_cast<uint32_t>(slot_index));
    if (!children.IsArrayOrJSArray()) {
      continue;
    }
    for (size_t index = 0; index < static_cast<size_t>(children.GetLength());
         ++index) {
      auto child = ResolveElementTemplateInstanceValue(
          children.GetProperty(static_cast<uint32_t>(index)));
      if (child && child->ContainsListTemplate()) {
        return true;
      }
    }
  }
  return false;
}

void ElementTemplateInstance::ResetOwnMaterializedElements() {
  result_ = nullptr;
  attribute_slot_targets_.clear();
  event_attribute_slot_targets_.clear();
  static_event_targets_.clear();
  element_slot_targets_.clear();
  pending_child_mount_slots_.clear();
  create_element_tree_task_ = nullptr;
  materialization_requested_ = false;
}

void ElementTemplateInstance::ResetMaterializedElements() {
  if (ShouldMaterializeChildSlots() && child_slots_.IsArrayOrJSArray()) {
    for (size_t slot_index = 0;
         slot_index < static_cast<size_t>(child_slots_.GetLength());
         ++slot_index) {
      auto children =
          child_slots_.GetProperty(static_cast<uint32_t>(slot_index));
      if (!children.IsArrayOrJSArray()) {
        continue;
      }
      for (size_t index = 0; index < static_cast<size_t>(children.GetLength());
           ++index) {
        auto child = ResolveElementTemplateInstanceValue(
            children.GetProperty(static_cast<uint32_t>(index)));
        if (child) {
          child->ResetMaterializedElements();
        }
      }
    }
  }
  ResetOwnMaterializedElements();
}

#if ENABLE_TRACE_PERFETTO
void ElementTemplateInstance::UpdateTraceDebugInfo(TraceEvent* event) const {
  auto* bundle_url_info = event->add_debug_annotations();
  bundle_url_info->set_name("bundle_url");
  bundle_url_info->set_string_value(bundle_url_.str());

  auto* template_key_info = event->add_debug_annotations();
  template_key_info->set_name("template_key");
  template_key_info->set_string_value(
      template_key_.empty() ? "null" : template_key_.str());

  auto* typed_tag_info = event->add_debug_annotations();
  typed_tag_info->set_name("typed_tag");
  typed_tag_info->set_string_value(typed_tag_.empty() ? "null"
                                                      : typed_tag_.str());

  if (uid_.IsNumber()) {
    auto* uid_info = event->add_debug_annotations();
    uid_info->set_name("uid");
    uid_info->set_double_value(uid_.Number());
  }

  auto* materialized_info = event->add_debug_annotations();
  materialized_info->set_name("materialized");
  materialized_info->set_bool_value(IsMaterialized());

  auto* pending_slots_info = event->add_debug_annotations();
  pending_slots_info->set_name("pending_child_mount_slot_count");
  pending_slots_info->set_uint_value(pending_child_mount_slots_.size());

  if (result_ != nullptr) {
    result_->UpdateTraceDebugInfo(event);
  }
}
#endif

}  // namespace tasm
}  // namespace lynx
