// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/dom/fiber/list_element_template_instance.h"

#include <array>
#include <initializer_list>
#include <memory>
#include <string>
#include <utility>

#include "base/include/value/array.h"
#include "base/include/value/table.h"
#include "core/public/pipeline_option.h"
#include "core/renderer/dom/attribute_holder.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/list_element.h"
#include "core/renderer/dom/fiber/page_element.h"
#include "core/renderer/dom/fiber/raw_text_element.h"
#include "core/renderer/dom/testing/element_template_instance_test.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/template_entry.h"
#include "core/renderer/utils/base/element_template_info.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/runtime/lepus/bindings/renderer.h"
#include "core/runtime/lepus/bindings/renderer_functions.h"
#include "core/runtime/lepus/bytecode_generator.h"
#include "core/runtime/lepusng/quick_context.h"
#include "core/shell/runtime/mts/mts_runtime.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

#undef protected
#undef private

namespace lynx {
namespace tasm {
namespace testing {

class ListElementTemplateInstanceTest : public ElementTemplateInstanceTest {
 protected:
  using InstanceRefPtr = fml::RefPtr<ElementTemplateInstance>;

  struct InitialListTestData {
    static constexpr uint32_t kItemCount = 20;

    // Declare runtime first so it outlives the JS/ET Values stored below.
    std::shared_ptr<runtime::MTSRuntime> runtime;
    lepus::Value page_value;
    lepus::Value list_value;
  };

  /**
   * Create a compiled header ET referencing et_header_template via native PAPI.
   * The corresponding JSX root is shown below; its static styles and subtree
   * come from the template definition:
   *
   * <list-item key="header" item-key="header" full-span={true}
   *   estimated-main-axis-size-px={150}>...</list-item>
   *
   * This creates only a logical instance. It neither registers a template
   * definition nor materializes an Element.
   * @param ctx Execution context for the native PAPI call.
   * @param uid Numeric native ET UID, separate from React key and List
   * item-key.
   * @return Value wrapping the ET reference, which SetGlobalData can expose to
   * JS.
   */
  lepus::Value CreateHeaderInstanceValue(runtime::MTSContext* ctx, int uid) {
    auto header_attribute_slots = lepus::CArray::Create();
    header_attribute_slots->emplace_back(lepus::Value(150));

    auto list_item_platform_info = lepus::Dictionary::Create();
    list_item_platform_info->SetValue("item-key", lepus::Value("header"));
    list_item_platform_info->SetValue("full-span", lepus::Value(true));
    list_item_platform_info->SetValue("estimated-main-axis-size-px",
                                      lepus::Value(150));
    auto header_options = lepus::Dictionary::Create();
    header_options->SetValue("__listItemPlatformInfo",
                             lepus::Value(list_item_platform_info));
    // PAPI arguments: [templateKey, bundleUrl, attributeSlots, childSlots, uid,
    // options].
    lepus::Value header_args[] = {lepus::Value("et_header_template"),
                                  lepus::Value(DEFAULT_ENTRY_NAME),
                                  lepus::Value(header_attribute_slots),
                                  lepus::Value(lepus::CArray::Create()),
                                  lepus::Value(uid),
                                  lepus::Value(header_options)};
    return RendererFunctions::FiberCreateElementTemplate(ctx, header_args, 6);
  }

  /**
   * Prepare template definitions, a PAPI runtime, and logical ET instances for
   * the initial render of a vertical List.
   *
   * Prepare a Page ET and a detached List ET whose subtree contains 20 compiled
   * item ETs, each with its own raw-text ET. Item keys and text range from
   * list-item-0 to list-item-19. Only the Page root is materialized on return.
   * The caller can mount the List, submit initial updates, and request item
   * materialization by index.
   *
   * @param data Holds the runtime and Page/List Values for subsequent PAPI
   * calls.
   */
  void PrepareInitialList(InitialListTestData& data) {
    /**
     * Prepare the main bundle's compiled template table.
     *
     * At build time, elementTemplate in tasm.json is encoded into the bundle.
     * When the page loads the bundle, its templates are decoded into
     * ElementTemplateInfo/ElementInfo and stored in the TemplateEntry's table.
     * This test supplies that decoded data directly for lookup by templateKey.
     *
     * TemplateEntry holds a bundle's context; DEFAULT_ENTRY_NAME identifies the
     * main bundle. element_template_infos_ maps template keys to definitions.
     * The two definitions below are shared by 20 item ETs and 20 text ETs.
     * Subsequent PAPI calls supply each instance's UID, attributeSlots, and
     * childSlots. List and Page use typed PAPI, with their root tags supplied
     * directly.
     */
    auto entry = std::make_shared<TemplateEntry>();
    entry->SetName(DEFAULT_ENTRY_NAME);
    tasm->template_entries_[DEFAULT_ENTRY_NAME] = entry;

    /**
     * The first definition describes a list-item.
     *
     * "_et_bb36f9d80e45": {
     *   "kind": "element",
     *   "type": "list-item",
     *   "attributesArray": [
     *     { "kind": "slot", "key": "item-key", "attrSlotIndex": 0 },
     *     { "kind": "static", "key": "full-span", "value": true },
     *     {
     *       "kind": "static",
     *       "key": "estimated-main-axis-size-px",
     *       "value": 100
     *     },
     *     { "kind": "static", "key": "class", "value": "ListItem" }
     *   ],
     *   "children": [
     *     {
     *       "kind": "element",
     *       "type": "text",
     *       "attributesArray": [
     *         { "kind": "static", "key": "class", "value": "ItemText" }
     *       ],
     *       "children": [
     *         { "kind": "childSlot", "type": "slot", "elementSlotIndex": 0 }
     *       ]
     *     }
     *   ]
     * }
     *
     * The corresponding node structure is:
     * list-item Element
     *   item-key = the current item ET's attributeSlots[0]
     *   full-span = true, estimated-main-axis-size-px = 100
     *   class = ListItem
     *   └─ text Element
     *      class = ItemText
     *      └─ ElementSlot[0]
     */
    auto item_element_template_info = std::make_shared<ElementTemplateInfo>();
    // exist_ marks the definition as valid; key_ matches the template key
    // passed to __CreateElementTemplate below.
    item_element_template_info->exist_ = true;
    item_element_template_info->key_ = "_et_bb36f9d80e45";

    // Describe the list-item root of the template.
    ElementInfo item_root_info;
    item_root_info.tag_ = base::String("list-item");
    // class_selector_ stores static classes, matching the JSON class attribute.
    item_root_info.class_selector_ = {base::String("ListItem")};
    // Attribute fields: binding type, name, static value, and attribute slot
    // index. For DYNAMIC, index 0 corresponds to attrSlotIndex=0 and reads
    // attributeSlots[0]. STATIC reads value_; its trailing index 0 is an unused
    // placeholder.
    item_root_info.attributes_ =
        std::make_shared<const TemplateAttributes>(TemplateAttributes{
            Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC, base::String("item-key"),
                      lepus::Value(), 0},
            Attribute{ATTRIBUTE_BINDING_TYPE_STATIC, base::String("full-span"),
                      lepus::Value(true), 0},
            Attribute{ATTRIBUTE_BINDING_TYPE_STATIC,
                      base::String("estimated-main-axis-size-px"),
                      lepus::Value(100), 0}});

    ElementInfo text_info;
    // The built-in text tag_enum_ corresponds to type="text" in JSON.
    text_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_TEXT;
    text_info.class_selector_ = {base::String("ItemText")};

    ElementInfo text_slot;
    // ELEMENT_SLOT with slot_index_=0 corresponds to
    // childSlot/elementSlotIndex=0. This indexes childSlots independently of
    // attributeSlots[0] above.
    text_slot.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_SLOT;
    text_slot.slot_index_ = 0;

    // Build outward: add the slot to text, then text to list-item.
    text_info.children_.emplace_back(std::move(text_slot));
    item_root_info.children_.emplace_back(std::move(text_info));
    // elements_ holds the root descriptions; this template has one list-item
    // root.
    item_element_template_info->elements_.emplace_back(
        std::move(item_root_info));
    // Register in the main bundle so all item ETs can reference this
    // definition.
    entry->template_bundle_
        .element_template_infos_[item_element_template_info->key_] =
        item_element_template_info;

    /**
     * The second definition describes a raw-text leaf holding dynamic text.
     *
     * "_et_builtin_raw_text": {
     *   "kind": "element",
     *   "type": "raw-text",
     *   "attributesArray": [
     *     { "kind": "slot", "key": "text", "attrSlotIndex": 0 }
     *   ],
     *   "children": []
     * }
     */
    auto raw_text_element_template_info =
        std::make_shared<ElementTemplateInfo>();
    raw_text_element_template_info->exist_ = true;
    raw_text_element_template_info->key_ = "_et_builtin_raw_text";
    ElementInfo raw_text_root_info;
    // The built-in tag creates a RawTextElement to hold the dynamic text.
    raw_text_root_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_RAW_TEXT;
    raw_text_root_info.attributes_ = std::make_shared<const TemplateAttributes>(
        TemplateAttributes{Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC,
                                     base::String("text"), lepus::Value(), 0}});
    raw_text_element_template_info->elements_.emplace_back(
        std::move(raw_text_root_info));
    // Register a compiled template for PAPI to create separate text instances.
    entry->template_bundle_
        .element_template_infos_[raw_text_element_template_info->key_] =
        raw_text_element_template_info;

    // 2. Follow the initial renderer's creation order: text, item, then typed
    // List. Assign decreasing numeric native ET UIDs.
    data.runtime = CreatePAPIRuntime();
    auto& runtime = data.runtime;
    auto* ctx = runtime::MTSRuntime::ToQuickContext(runtime.get());
    ASSERT_NE(ctx, nullptr);
    static const runtime::RenderBindingFunction functions[] = {
        {kCFunctionCreateElementTemplate,
         &RendererFunctions::FiberCreateElementTemplate},
        {kCFunctionCreateTypedElementTemplate,
         &RendererFunctions::FiberCreateTypedElementTemplate}};
    runtime->RegisterGlobalFunction(functions, 2);

    // Page is the initial commit root; creating its ET materializes the root
    // node.
    lepus::Value page_args[] = {lepus::Value("page"), lepus::Value(),
                                lepus::Value(), lepus::Value(0)};
    auto page_value =
        RendererFunctions::FiberCreateTypedElementTemplate(ctx, page_args, 4);
    auto page_instance =
        ElementTemplateInstance::ResolveElementTemplateInstanceValue(
            page_value);
    ASSERT_NE(page_instance, nullptr);
    auto page_root = page_instance->PeekMaterializedRoot();
    ASSERT_NE(page_root, nullptr);
    ASSERT_TRUE(page_root->is_page());

    runtime->SetGlobalData("itemCount",
                           lepus::Value(InitialListTestData::kItemCount));
    lepus::BytecodeGenerator::GenerateBytecode(runtime->GetMTSContext(), R"JS(
          let nextUid = -1;
          const items = Array.from(
            { length: itemCount },
            (_, index) => `list-item-${index}`);
          // Create text before each item, with UIDs starting at -1.
          let itemInstances = items.map(item => {
            const textET = __CreateElementTemplate(
              '_et_builtin_raw_text', null, [item], [], nextUid--);
            return __CreateElementTemplate(
              '_et_bb36f9d80e45', null, [item], [[textET]], nextUid--,
              {
                __listItemPlatformInfo: {
                  'item-key': item,
                  'full-span': true,
                  'estimated-main-axis-size-px': 100,
                },
              });
          });
          let onScroll = e => e;
          let onLayoutComplete = () => 'layout complete';
          let listAttributes = {
            class: 'List',
            'scroll-orientation': 'vertical',
            'span-count': 1,
            'list-type': 'single',
            'custom-list-name': 'list-container',
            bindscroll: onScroll,
            bindlayoutcomplete: onLayoutComplete,
            style: { listMainAxisGap: '10px', background: 'lightgrey' },
          };
          let listInstance = __CreateTypedElementTemplate(
            'list', listAttributes, [itemInstances], nextUid--, {});
        )JS",
                                               runtime->GetSdkVersion(), "");
    ASSERT_TRUE(runtime->Execute(nullptr));
    data.page_value = page_value;
    data.list_value = runtime->GetGlobalData("listInstance");
  }

  void CreateAdjacentSlotsTemplate() {
    auto entry = std::make_shared<TemplateEntry>();
    entry->SetName(DEFAULT_ENTRY_NAME);
    tasm->template_entries_[DEFAULT_ENTRY_NAME] = entry;

    auto info = std::make_shared<ElementTemplateInfo>();
    info->exist_ = true;
    info->key_ = "adjacent_slots";
    ElementInfo root;
    root.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
    for (uint32_t slot_index : {0u, 1u}) {
      ElementInfo slot;
      slot.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_SLOT;
      slot.slot_index_ = slot_index;
      root.children_.emplace_back(std::move(slot));
    }
    ElementInfo sentinel;
    sentinel.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
    root.children_.emplace_back(std::move(sentinel));
    info->elements_.emplace_back(std::move(root));
    entry->template_bundle_.element_template_infos_["adjacent_slots"] =
        std::move(info);
  }

  InstanceRefPtr CreateChild(const char* uid, const char* tag = "view") {
    auto child = fml::AdoptRef(new ElementTemplateInstance(manager));
    child->SetTASM(tasm.get());
    child->SetTypedTag(base::String(tag));
    child->SetUid(lepus::Value(uid));
    return child;
  }

  void SetChildSlots(
      const InstanceRefPtr& parent,
      std::initializer_list<std::initializer_list<InstanceRefPtr>> slots) {
    auto child_slots = lepus::CArray::Create();
    for (const auto& children : slots) {
      auto slot_children = lepus::CArray::Create();
      for (const auto& child : children) {
        slot_children->emplace_back(lepus::Value(child));
      }
      child_slots->emplace_back(lepus::Value(std::move(slot_children)));
    }
    parent->InitializeChildSlots(lepus::Value(std::move(child_slots)));
  }

  InstanceRefPtr CreateItem(const char* uid,
                            std::initializer_list<InstanceRefPtr> first_slot,
                            std::initializer_list<InstanceRefPtr> second_slot) {
    auto item = fml::AdoptRef(new ElementTemplateInstance(manager));
    item->SetTASM(tasm.get());
    item->SetBundleUrl(base::String(DEFAULT_ENTRY_NAME));
    item->SetTemplateKey(base::String("adjacent_slots"));
    item->SetUid(lepus::Value(uid));
    SetChildSlots(item, {first_slot, second_slot});
    return item;
  }

  void ExpectSlotChildren(const InstanceRefPtr& parent, uint32_t slot_index,
                          std::initializer_list<InstanceRefPtr> expected) {
    auto children = parent->GetChildSlots().GetProperty(slot_index);
    ASSERT_TRUE(children.IsArray());
    ASSERT_EQ(static_cast<size_t>(children.GetLength()), expected.size());
    uint32_t index = 0;
    for (const auto& child : expected) {
      EXPECT_EQ(ElementTemplateInstance::ResolveElementTemplateInstanceValue(
                    children.GetProperty(index++)),
                child);
      EXPECT_EQ(child->GetLogicalParent(), parent.get());
    }
  }
};

TEST_P(ListElementTemplateInstanceTest,
       PAPICreatesListElementTemplateInstance) {
  /**
   * Purpose
   * Verify PAPI creation, materialization, and serialization of a typed List ET
   * with compiled child ETs.
   *
   * Input
   * C++ creates a compiled header and exposes it to JS. JS creates four
   * compiled item ETs, each with a text ET. List child slot 0 contains
   * [header,item-1,item-2,item-3,item-4]. The List has attributes, styles, and
   * scroll/layoutcomplete callbacks. Each item's estimated main-axis size is
   * 150.
   *
   * Expected
   * 1. The List type, UID, attributes, callbacks, and childSlots are correct.
   *    Its materialized root is a List with scroll/layoutcomplete events.
   * 2. Header/item/text templates, UIDs, slots, and logical parents are
   * correct. Header/item platform metadata is complete; all items and text ETs
   * are not yet materialized.
   * 3. Serialization preserves the List, five items, and four text child ETs.
   */
  auto runtime = CreatePAPIRuntime();
  auto* ctx = runtime::MTSRuntime::ToQuickContext(runtime.get());
  ASSERT_NE(ctx, nullptr);

  // JS creates text and regular items through compiled PAPI.
  // C++ calls typed PAPI for the List directly, so JS needs no typed binding.
  static const runtime::RenderBindingFunction functions[] = {
      {kCFunctionCreateElementTemplate,
       &RendererFunctions::FiberCreateElementTemplate}};
  runtime->RegisterGlobalFunction(functions, 1);

  // Create the header ET in C++ and expose it through SetGlobalData.
  // Execute JS below to create the remaining List children.
  auto et_header_instance_value = CreateHeaderInstanceValue(ctx, 1);
  ASSERT_TRUE(et_header_instance_value.IsRefCounted());
  ASSERT_EQ(et_header_instance_value.RefCounted()->GetRefType(),
            lepus::RefType::kElementTemplate);
  auto et_header_instance =
      ElementTemplateInstance::ResolveElementTemplateInstanceValue(
          et_header_instance_value);
  ASSERT_NE(et_header_instance, nullptr);
  EXPECT_EQ(et_header_instance->PeekMaterializedRoot(), nullptr);
  EXPECT_EQ(et_header_instance->GetLogicalParent(), nullptr);
  runtime->SetGlobalData("etHeaderInstance", et_header_instance_value);

  // Compile JS for Execute to construct the initial List inputs.
  lepus::BytecodeGenerator::GenerateBytecode(runtime->GetMTSContext(),
                                             R"JS(
        // PAPI requires numeric UIDs: List=0, C++ header=1, remaining nodes start at 2.
        let nextUid = 2;
        const mainAxisSize = 150;

        const getItemOptions = itemKey => ({
          __listItemPlatformInfo: {
            'item-key': itemKey,
            'full-span': true,
            'estimated-main-axis-size-px': mainAxisSize,
          },
        });

        let onScroll = e => e;
        let onLayoutComplete = () => 'layout complete';
        let listAttributes = {
          class: 'List',
          'scroll-orientation': 'vertical',
          'span-count': 1,
          'list-type': 'single',
          'custom-list-name': 'list-container',
          bindscroll: onScroll,
          bindlayoutcomplete: onLayoutComplete,
          style: {
            height: 600,
            listMainAxisGap: 10,
            background: 'lightgrey',
            padding: '5px',
          },
        };

        let items = ['item-1', 'item-2', 'item-3', 'item-4'].map(item => {
          // Follow the dynamic text path using the built-in compiled raw-text template.
          // Attribute slot 0 holds text; childSlots is empty; null selects the main bundle.
          const textET = __CreateElementTemplate(
            '_et_builtin_raw_text', null, [item], [], nextUid++);
          return __CreateElementTemplate(
            '_et_ed2a1fed883d',
            undefined,
            [item, mainAxisSize], // attribute slot 0: item-key; slot 1: estimated size
            [[textET]],          // child slot 0: text ET reference
            nextUid++,
            getItemOptions(item)
          );
        });

        // The outer array contains slots; slot 0 holds the header and four items.
        // These are inputs for the List ET created by C++ PAPI below.
        // JS places the compiled etHeaderInstance reference from C++ into the array.
        let listSlots = [[etHeaderInstance, ...items]];
      )JS",
                                             runtime->GetSdkVersion(), "");

  // Execute JS before reading back the attributes and arrays it creates.
  ASSERT_TRUE(runtime->Execute(nullptr));
  auto list_attributes = runtime->GetGlobalData("listAttributes");
  auto list_slots = runtime->GetGlobalData("listSlots");
  ASSERT_TRUE(list_attributes.IsJSValue());
  ASSERT_TRUE(list_slots.IsJSArray());
  ASSERT_EQ(list_slots.GetLength(), 1);
  auto js_items = list_slots.GetProperty(0);
  ASSERT_EQ(js_items.GetLength(), 5);

  // Create the List ET: [tag="list", attributes, childSlots, uid=0].
  // PAPI selects ListElementTemplateInstance and establishes logical parents.
  lepus::Value list_args[] = {lepus::Value("list"), list_attributes, list_slots,
                              lepus::Value(0)};
  auto list_value =
      RendererFunctions::FiberCreateTypedElementTemplate(ctx, list_args, 4);

  // 1. Check List ET creation by reading the instance state.
  ASSERT_TRUE(list_value.IsRefCounted());
  ASSERT_EQ(list_value.RefCounted()->GetRefType(),
            lepus::RefType::kElementTemplate);
  auto list_instance =
      ElementTemplateInstance::ResolveElementTemplateInstanceValue(list_value);
  ASSERT_NE(list_instance, nullptr);
  ASSERT_EQ(list_instance->GetChildSlots().GetLength(), 1);
  ASSERT_EQ(list_instance->GetChildSlots().GetProperty(0).GetLength(), 5);
  EXPECT_TRUE(list_instance->IsTypedTemplate());
  EXPECT_EQ(list_instance->GetTypedTag(), base::String("list"));
  EXPECT_EQ(list_instance->GetUid(), lepus::Value(0));
  EXPECT_EQ(list_instance->GetLogicalParent(), nullptr);
  EXPECT_EQ(list_instance->PeekMaterializedRoot(), nullptr);
  // Typed ETs store the full attributes in attribute slot 0; inspect them here.
  ASSERT_TRUE(list_instance->attribute_slots_.IsArray());
  ASSERT_EQ(list_instance->attribute_slots_.GetLength(), 1);
  auto instance_attributes = list_instance->attribute_slots_.GetProperty(0);
  EXPECT_EQ(instance_attributes.GetProperty("class"), lepus::Value("List"));
  EXPECT_EQ(instance_attributes.GetProperty("scroll-orientation"),
            lepus::Value("vertical"));
  EXPECT_EQ(instance_attributes.GetProperty("span-count").Number(), 1);
  EXPECT_EQ(instance_attributes.GetProperty("list-type"),
            lepus::Value("single"));
  EXPECT_EQ(instance_attributes.GetProperty("custom-list-name"),
            lepus::Value("list-container"));
  for (const auto& event :
       {std::make_pair("bindscroll", "onScroll"),
        std::make_pair("bindlayoutcomplete", "onLayoutComplete")}) {
    auto callback = instance_attributes.GetProperty(event.first);
    EXPECT_TRUE(callback.IsCallable());
    EXPECT_TRUE(callback.IsEqual(runtime->GetGlobalData(event.second)));
  }
  auto instance_style = instance_attributes.GetProperty("style");
  EXPECT_EQ(instance_style.GetProperty("height").Number(), 600);
  EXPECT_EQ(instance_style.GetProperty("listMainAxisGap").Number(), 10);
  EXPECT_EQ(instance_style.GetProperty("background"),
            lepus::Value("lightgrey"));
  EXPECT_EQ(instance_style.GetProperty("padding"), lepus::Value("5px"));

  // GetRoot materializes only the List; no item has been materialized yet.
  auto list_root = list_instance->GetRoot();
  ASSERT_NE(list_root, nullptr);
  EXPECT_TRUE(list_root->is_list());
  EXPECT_TRUE(list_root->children().empty());
  // Check event registration.
  EXPECT_EQ(list_root->event_map().count("scroll"), 1u);
  EXPECT_EQ(list_root->event_map().count("layoutcomplete"), 1u);

  // 2. Check header, regular item, and text ET creation.
  const char* item_keys[] = {"header", "item-1", "item-2", "item-3", "item-4"};
  const char* item_labels[] = {"item-1", "item-2", "item-3", "item-4"};
  for (uint32_t index = 0; index < 5; ++index) {
    SCOPED_TRACE(index);
    auto item_instance = list_instance->GetChildInstanceAt(0, index);
    ASSERT_NE(item_instance, nullptr);
    // The List must hold the original ET instances from the JS array.
    EXPECT_EQ(item_instance,
              ElementTemplateInstance::ResolveElementTemplateInstanceValue(
                  js_items.GetProperty(index)));
    EXPECT_FALSE(item_instance->IsTypedTemplate());
    EXPECT_EQ(item_instance->GetBundleUrl(), base::String(DEFAULT_ENTRY_NAME));
    EXPECT_EQ(item_instance->GetLogicalParent(), list_instance.get());
    EXPECT_EQ(item_instance->PeekMaterializedRoot(), nullptr);
    // Header UID=1; regular item UIDs=3/5/7/9; text UIDs=2/4/6/8.
    EXPECT_EQ(item_instance->GetUid().Number(), index * 2 + 1);
    auto metadata = item_instance->GetListItemPlatformInfoFromOptions();
    EXPECT_EQ(metadata.GetProperty("item-key"), lepus::Value(item_keys[index]));
    EXPECT_TRUE(metadata.GetProperty("full-span").IsTrue());
    EXPECT_EQ(metadata.GetProperty("estimated-main-axis-size-px").Number(),
              150);

    const auto& attribute_slots = item_instance->attribute_slots_;
    ASSERT_TRUE(attribute_slots.IsArray());
    if (index == 0) {
      // Header text is static template content, so it has no text child ET.
      EXPECT_EQ(item_instance, et_header_instance);
      EXPECT_EQ(item_instance->GetTemplateKey(),
                base::String("et_header_template"));
      ASSERT_EQ(attribute_slots.GetLength(), 1);
      EXPECT_EQ(attribute_slots.GetProperty(0).Number(), 150);
      ASSERT_TRUE(item_instance->GetChildSlots().IsArray());
      EXPECT_EQ(item_instance->GetChildSlots().GetLength(), 0);
      continue;
    }

    EXPECT_EQ(item_instance->GetTemplateKey(),
              base::String("_et_ed2a1fed883d"));
    ASSERT_EQ(attribute_slots.GetLength(), 2);
    EXPECT_EQ(attribute_slots.GetProperty(0), lepus::Value(item_keys[index]));
    EXPECT_EQ(attribute_slots.GetProperty(1).Number(), 150);
    ASSERT_EQ(item_instance->GetChildSlots().GetLength(), 1);
    ASSERT_EQ(item_instance->GetChildSlots().GetProperty(0).GetLength(), 1);
    auto text_instance = item_instance->GetChildInstanceAt(0, 0);
    ASSERT_NE(text_instance, nullptr);
    EXPECT_FALSE(text_instance->IsTypedTemplate());
    EXPECT_EQ(text_instance->GetTemplateKey(),
              base::String("_et_builtin_raw_text"));
    EXPECT_EQ(text_instance->GetBundleUrl(), base::String(DEFAULT_ENTRY_NAME));
    EXPECT_EQ(text_instance->GetUid().Number(), index * 2);
    EXPECT_EQ(text_instance->GetLogicalParent(), item_instance.get());
    EXPECT_EQ(text_instance->PeekMaterializedRoot(), nullptr);
    ASSERT_TRUE(text_instance->GetChildSlots().IsArray());
    EXPECT_EQ(text_instance->GetChildSlots().GetLength(), 0);
    const auto& text_attribute_slots = text_instance->attribute_slots_;
    ASSERT_TRUE(text_attribute_slots.IsArray());
    ASSERT_EQ(text_attribute_slots.GetLength(), 1);
    EXPECT_EQ(text_attribute_slots.GetProperty(0),
              lepus::Value(item_labels[index - 1]));
  }

  // 3. Check serialized List data, including all items and text child ETs.
  lepus::Value serialize_args[] = {list_value};
  auto serialized =
      RendererFunctions::FiberSerializeElementTemplate(ctx, serialize_args, 1);
  EXPECT_EQ(serialized.GetProperty("tag"), lepus::Value("list"));
  EXPECT_EQ(serialized.GetProperty("uid"), lepus::Value(0));
  auto attributes = serialized.GetProperty("attributes");
  EXPECT_EQ(attributes.GetProperty("class"), lepus::Value("List"));
  EXPECT_EQ(attributes.GetProperty("scroll-orientation"),
            lepus::Value("vertical"));
  EXPECT_EQ(attributes.GetProperty("span-count").Number(), 1);
  EXPECT_EQ(attributes.GetProperty("list-type"), lepus::Value("single"));
  EXPECT_EQ(attributes.GetProperty("custom-list-name"),
            lepus::Value("list-container"));
  for (const auto& event :
       {std::make_pair("bindscroll", "onScroll"),
        std::make_pair("bindlayoutcomplete", "onLayoutComplete")}) {
    auto callback = attributes.GetProperty(event.first);
    EXPECT_TRUE(callback.IsCallable());
    EXPECT_TRUE(callback.IsEqual(runtime->GetGlobalData(event.second)));
  }
  auto style = attributes.GetProperty("style");
  EXPECT_EQ(style.GetProperty("height").Number(), 600);
  EXPECT_EQ(style.GetProperty("listMainAxisGap").Number(), 10);
  EXPECT_EQ(style.GetProperty("background"), lepus::Value("lightgrey"));
  EXPECT_EQ(style.GetProperty("padding"), lepus::Value("5px"));

  auto serialized_slots = serialized.GetProperty("childSlots");
  ASSERT_EQ(serialized_slots.GetLength(), 1);
  auto serialized_items = serialized_slots.GetProperty(0);
  ASSERT_EQ(serialized_items.GetLength(), 5);
  for (uint32_t index = 0; index < 5; ++index) {
    SCOPED_TRACE(index);
    auto serialized_item = serialized_items.GetProperty(index);
    EXPECT_EQ(
        serialized_item.GetProperty("templateKey"),
        lepus::Value(index == 0 ? "et_header_template" : "_et_ed2a1fed883d"));
    EXPECT_EQ(serialized_item.GetProperty("bundleUrl"),
              lepus::Value(DEFAULT_ENTRY_NAME));
    EXPECT_EQ(serialized_item.GetProperty("uid").Number(), index * 2 + 1);
    auto metadata = serialized_item.GetProperty("options").GetProperty(
        "__listItemPlatformInfo");
    EXPECT_EQ(metadata.GetProperty("item-key"), lepus::Value(item_keys[index]));
    EXPECT_TRUE(metadata.GetProperty("full-span").IsTrue());
    EXPECT_EQ(metadata.GetProperty("estimated-main-axis-size-px").Number(),
              150);

    auto attribute_slots = serialized_item.GetProperty("attributeSlots");
    ASSERT_TRUE(attribute_slots.IsArray());
    auto child_slots = serialized_item.GetProperty("childSlots");
    ASSERT_TRUE(child_slots.IsArray());
    if (index == 0) {
      ASSERT_EQ(attribute_slots.GetLength(), 1);
      EXPECT_EQ(attribute_slots.GetProperty(0).Number(), 150);
      EXPECT_EQ(child_slots.GetLength(), 0);
      continue;
    }
    ASSERT_EQ(attribute_slots.GetLength(), 2);
    EXPECT_EQ(attribute_slots.GetProperty(0), lepus::Value(item_keys[index]));
    EXPECT_EQ(attribute_slots.GetProperty(1).Number(), 150);
    ASSERT_EQ(child_slots.GetLength(), 1);
    ASSERT_EQ(child_slots.GetProperty(0).GetLength(), 1);
    auto text_snapshot = child_slots.GetProperty(0).GetProperty(0);
    // Compiled text uses a template key and attribute slot 0; childSlots is
    // empty.
    EXPECT_EQ(text_snapshot.GetProperty("templateKey"),
              lepus::Value("_et_builtin_raw_text"));
    EXPECT_EQ(text_snapshot.GetProperty("bundleUrl"),
              lepus::Value(DEFAULT_ENTRY_NAME));
    EXPECT_EQ(text_snapshot.GetProperty("uid").Number(), index * 2);
    auto text_attribute_slots = text_snapshot.GetProperty("attributeSlots");
    ASSERT_TRUE(text_attribute_slots.IsArray());
    ASSERT_EQ(text_attribute_slots.GetLength(), 1);
    EXPECT_EQ(text_attribute_slots.GetProperty(0),
              lepus::Value(item_labels[index - 1]));
    auto text_child_slots = text_snapshot.GetProperty("childSlots");
    ASSERT_TRUE(text_child_slots.IsArray());
    EXPECT_EQ(text_child_slots.GetLength(), 0);
  }
}

TEST_P(ListElementTemplateInstanceTest,
       PAPIInitialListFlushGeneratesUpdateListInfo) {
  /**
   * Purpose
   * Cover initial creation, List root materialization, and list update
   * submission for a vertical List.
   *
   * Input
   * Page ET -> List ET -> 20 compiled list-item ETs -> their raw-text ETs.
   * Item keys and text range from list-item-0 to list-item-19.
   * Each item's attribute slot 0 holds its key; child slot 0 holds its text ET.
   * Platform metadata includes full-span=true and estimated main-axis size 100.
   *
   * Expected
   * 1. Templates, UIDs, slots, metadata, and logical parents are correct.
   * 2. Mounting on Page materializes only the List; all items and text ETs are
   *    not yet materialized.
   * 3. The initial commit generates 20 insertAction entries at positions 0-19.
   *    removeAction/updateAction are empty; the recorder and pending state
   * clear.
   * 4. The List data model contains the complete update-list-info, and the
   *    platform mock receives a List node without physical item children.
   */
  InitialListTestData initial_list;
  ASSERT_NO_FATAL_FAILURE(PrepareInitialList(initial_list));
  auto& runtime = initial_list.runtime;
  auto* ctx = runtime::MTSRuntime::ToQuickContext(runtime.get());
  const auto& page_value = initial_list.page_value;
  auto page_instance =
      ElementTemplateInstance::ResolveElementTemplateInstanceValue(page_value);
  auto page_root = page_instance->PeekMaterializedRoot();
  const auto& list_value = initial_list.list_value;
  auto list_instance =
      ElementTemplateInstance::ResolveElementTemplateInstanceValue(list_value);

  ASSERT_NE(list_instance, nullptr);
  ASSERT_TRUE(list_instance->IsTypedTemplate());
  ASSERT_EQ(list_instance->GetTypedTag(), base::String("list"));
  EXPECT_EQ(list_instance->GetUid(), lepus::Value(-41));
  EXPECT_EQ(list_instance->GetLogicalParent(), nullptr);
  EXPECT_EQ(list_instance->PeekMaterializedRoot(), nullptr);
  ASSERT_TRUE(list_instance->GetChildSlots().IsArray());
  ASSERT_EQ(list_instance->GetChildSlots().GetLength(), 1);
  ASSERT_EQ(list_instance->GetChildSlots().GetProperty(0).GetLength(),
            InitialListTestData::kItemCount);
  auto* list_instance_ptr =
      static_cast<ListElementTemplateInstance*>(list_instance.get());
  /**
   * __CreateTypedElementTemplate("list", ..., [itemInstances], ...)
   *   -> InitializeChildSlots(...)
   *      -> ListElementTemplateInstance::InsertNodeIntoChildSlot(...) per item
   *         -> Record insertion: HasPendingListUpdates() returns true.
   *         -> Enqueue in ElementManager:
   *            IsPendingListUpdatesEnqueued() returns true.
   */
  EXPECT_TRUE(list_instance_ptr->HasPendingListUpdates());
  EXPECT_TRUE(list_instance_ptr->IsPendingListUpdatesEnqueued());
  ASSERT_EQ(manager->pending_element_template_list_updates_.size(), 1u);
  EXPECT_EQ(manager->pending_element_template_list_updates_.front().get(),
            list_instance.get());

  // Check zero-based inputs, the logical tree, and 20 expected insert actions.
  auto js_item_keys = runtime->GetGlobalData("items");
  ASSERT_TRUE(js_item_keys.IsJSArray());
  ASSERT_EQ(js_item_keys.GetLength(), InitialListTestData::kItemCount);
  auto js_items = runtime->GetGlobalData("itemInstances");
  ASSERT_TRUE(js_items.IsJSArray());
  ASSERT_EQ(js_items.GetLength(), InitialListTestData::kItemCount);
  auto expected_insert_actions = lepus::CArray::Create();
  for (uint32_t index = 0; index < InitialListTestData::kItemCount; ++index) {
    SCOPED_TRACE(index);
    const auto item_key = "list-item-" + std::to_string(index);
    EXPECT_EQ(js_item_keys.GetProperty(index), lepus::Value(item_key));
    // UIDs follow creation order: the first item is -2 and its text is -1.
    const auto item_uid = -2 * static_cast<int32_t>(index + 1);
    // Check item_instance.
    auto item_instance = list_instance->GetChildInstanceAt(0, index);
    ASSERT_NE(item_instance, nullptr);
    EXPECT_EQ(item_instance,
              ElementTemplateInstance::ResolveElementTemplateInstanceValue(
                  js_items.GetProperty(index)));
    EXPECT_FALSE(item_instance->IsTypedTemplate());
    EXPECT_EQ(item_instance->GetTemplateKey(),
              base::String("_et_bb36f9d80e45"));
    EXPECT_EQ(item_instance->GetBundleUrl(), base::String(DEFAULT_ENTRY_NAME));
    EXPECT_EQ(item_instance->GetUid(), lepus::Value(item_uid));
    EXPECT_EQ(item_instance->GetLogicalParent(), list_instance.get());
    EXPECT_EQ(item_instance->PeekMaterializedRoot(), nullptr);
    ASSERT_TRUE(item_instance->attribute_slots_.IsArray());
    ASSERT_EQ(item_instance->attribute_slots_.GetLength(), 1);
    EXPECT_EQ(item_instance->attribute_slots_.GetProperty(0),
              lepus::Value(item_key));
    ASSERT_EQ(item_instance->GetChildSlots().GetLength(), 1);
    ASSERT_EQ(item_instance->GetChildSlots().GetProperty(0).GetLength(), 1);
    // Check text_instance.
    auto text_instance = item_instance->GetChildInstanceAt(0, 0);
    ASSERT_NE(text_instance, nullptr);
    EXPECT_FALSE(text_instance->IsTypedTemplate());
    EXPECT_EQ(text_instance->GetTemplateKey(),
              base::String("_et_builtin_raw_text"));
    EXPECT_EQ(text_instance->GetBundleUrl(), base::String(DEFAULT_ENTRY_NAME));
    EXPECT_EQ(text_instance->GetUid(), lepus::Value(item_uid + 1));
    EXPECT_EQ(text_instance->GetLogicalParent(), item_instance.get());
    EXPECT_EQ(text_instance->PeekMaterializedRoot(), nullptr);
    ASSERT_TRUE(text_instance->attribute_slots_.IsArray());
    ASSERT_EQ(text_instance->attribute_slots_.GetLength(), 1);
    EXPECT_EQ(text_instance->attribute_slots_.GetProperty(0),
              lepus::Value(item_key));
    ASSERT_TRUE(text_instance->GetChildSlots().IsArray());
    EXPECT_EQ(text_instance->GetChildSlots().GetLength(), 0);

    auto expected_metadata = lepus::Dictionary::Create();
    expected_metadata->SetValue("item-key", lepus::Value(item_key));
    expected_metadata->SetValue("full-span", lepus::Value(true));
    expected_metadata->SetValue("estimated-main-axis-size-px",
                                lepus::Value(100));
    // Check __listItemPlatformInfo.
    EXPECT_EQ(item_instance->GetListItemPlatformInfoFromOptions(),
              lepus::Value(expected_metadata));
    // Add the insertion position and template type to the expected dictionary.
    expected_metadata->SetValue("position",
                                lepus::Value(static_cast<int32_t>(index)));
    expected_metadata->SetValue("type", lepus::Value("_et_bb36f9d80e45"));
    expected_insert_actions->emplace_back(std::move(expected_metadata));
  }
  auto expected_info = lepus::Dictionary::Create();
  expected_info->SetValue("insertAction",
                          lepus::Value(expected_insert_actions));
  expected_info->SetValue("removeAction",
                          lepus::Value(lepus::CArray::Create()));
  expected_info->SetValue("updateAction",
                          lepus::Value(lepus::CArray::Create()));
  const lepus::Value expected_update_info(std::move(expected_info));

  // 3. Mount the List on Page through PAPI, materializing its typed root.
  lepus::Value insert_args[] = {page_value, lepus::Value(0), list_value,
                                lepus::Value()};
  RendererFunctions::FiberInsertNodeToElementTemplate(ctx, insert_args, 4);
  EXPECT_EQ(page_instance->GetChildInstanceAt(0, 0), list_instance);
  EXPECT_EQ(list_instance->GetLogicalParent(), page_instance.get());
  auto list_root = list_instance->PeekMaterializedRoot();
  ASSERT_NE(list_root, nullptr);
  ASSERT_TRUE(list_root->is_list());
  EXPECT_EQ(list_root->parent(), page_root.get());
  ASSERT_EQ(page_root->children().size(), 1u);
  EXPECT_EQ(page_root->children()[0], list_root);
  EXPECT_TRUE(list_root->children().empty());
  EXPECT_EQ(list_root->event_map().count("scroll"), 1u);
  EXPECT_EQ(list_root->event_map().count("layoutcomplete"), 1u);
  EXPECT_EQ(list_root->data_model()->attributes().count("update-list-info"),
            0u);
  EXPECT_TRUE(list_instance_ptr->HasPendingListUpdates());

  // 4. Finish the patch through Page::FlushActionsAsRoot.
  // Page submits pending List updates; Element::FlushActions consumes
  // attributes. Flush the platform mock's operation queue and check the
  // submitted data.
  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page_root.get());
  platform_impl_->Flush();

  // Check update data on ListElement.
  const auto& attributes = list_root->data_model()->attributes();
  const auto update_info = attributes.find(base::String("update-list-info"));
  ASSERT_NE(update_info, attributes.end());
  EXPECT_EQ(update_info->second, expected_update_info);
  // Check that this batch has finished processing.
  EXPECT_FALSE(list_instance_ptr->HasPendingListUpdates());
  EXPECT_FALSE(list_instance_ptr->IsPendingListUpdatesEnqueued());
  EXPECT_TRUE(manager->pending_element_template_list_updates_.empty());
  EXPECT_TRUE(manager->pending_element_template_child_mounts_.empty());
  // Check the physical List references its ET.
  auto* list_element = static_cast<ListElement*>(list_root.get());
  EXPECT_TRUE(list_element->is_element_template_list_);
  EXPECT_EQ(list_element->element_template_instance_.get(),
            list_instance.get());

  // After the initial commit, all item/text ETs retain their logical identity.
  // None has been materialized yet.
  ASSERT_EQ(list_instance->GetChildSlots().GetProperty(0).GetLength(),
            InitialListTestData::kItemCount);
  for (uint32_t index = 0; index < InitialListTestData::kItemCount; ++index) {
    SCOPED_TRACE(index);
    auto item_instance = list_instance->GetChildInstanceAt(0, index);
    ASSERT_NE(item_instance, nullptr);
    EXPECT_EQ(item_instance->GetLogicalParent(), list_instance.get());
    EXPECT_EQ(item_instance->PeekMaterializedRoot(), nullptr);
    auto text_instance = item_instance->GetChildInstanceAt(0, 0);
    ASSERT_NE(text_instance, nullptr);
    EXPECT_EQ(text_instance->GetLogicalParent(), item_instance.get());
    EXPECT_EQ(text_instance->PeekMaterializedRoot(), nullptr);
  }
}

TEST_P(ListElementTemplateInstanceTest,
       PAPIComponentAtIndexMaterializesFirstFiveItems) {
  /**
   * Purpose
   * Materialize the first five items on demand through ComponentAtIndex after
   * the initial List commit.
   *
   * Input
   * PrepareInitialList creates Page, List, and 20 compiled item ETs.
   * Each item defines a static list-item -> text subtree, with text child slot
   * 0 referencing a compiled raw-text ET. Text ranges from list-item-0 to
   * list-item-19. After the initial commit, request indices 0-4 from the
   * physical List, using operation IDs 1-5.
   *
   * Expected
   * 1. The initial commit materializes only Page/List, leaving items/text lazy.
   * 2. Each request creates a list-item -> text -> raw-text physical subtree,
   *    returns the item root's sign, mounts it on List, and registers it for
   * reuse.
   * 3. Attributes, text, and parents of the first five items are correct, and
   *    node creation reaches the platform mock.
   * 4. The remaining 15 items/text ETs are not yet materialized; the logical
   *    list still has 20 items. No new list updates or pending child mounts
   * remain.
   */
  constexpr uint32_t kVisibleItemCount = 5;
  InitialListTestData initial_list;
  ASSERT_NO_FATAL_FAILURE(PrepareInitialList(initial_list));
  auto* ctx = runtime::MTSRuntime::ToQuickContext(initial_list.runtime.get());
  auto page_instance =
      ElementTemplateInstance::ResolveElementTemplateInstanceValue(
          initial_list.page_value);
  ASSERT_NE(page_instance, nullptr);
  auto page_root = page_instance->PeekMaterializedRoot();
  ASSERT_NE(page_root, nullptr);
  auto list_instance =
      ElementTemplateInstance::ResolveElementTemplateInstanceValue(
          initial_list.list_value);
  ASSERT_NE(list_instance, nullptr);

  /**
   * 1. Mount the List on Page and finish the initial commit.
   * This generates 20 insertions and associates the physical List with its ET,
   * allowing ListElement::ComponentAtIndex to use the ET materialization path.
   */
  lepus::Value insert_args[] = {initial_list.page_value, lepus::Value(0),
                                initial_list.list_value, lepus::Value()};
  RendererFunctions::FiberInsertNodeToElementTemplate(ctx, insert_args, 4);
  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page_root.get());
  platform_impl_->Flush();

  auto list_root = list_instance->PeekMaterializedRoot();
  ASSERT_NE(list_root, nullptr);
  ASSERT_TRUE(list_root->is_list());
  EXPECT_EQ(list_root->parent(), page_root.get());
  auto* list_element = static_cast<ListElement*>(list_root.get());
  ASSERT_TRUE(list_element->is_element_template_list_);
  ASSERT_EQ(list_element->element_template_instance_.get(),
            list_instance.get());
  auto* list_instance_ptr =
      static_cast<ListElementTemplateInstance*>(list_instance.get());
  EXPECT_FALSE(list_instance_ptr->HasPendingListUpdates());
  EXPECT_FALSE(list_instance_ptr->IsPendingListUpdatesEnqueued());
  ASSERT_TRUE(list_root->children().empty());
  ASSERT_EQ(list_instance->GetChildSlots().GetProperty(0).GetLength(),
            InitialListTestData::kItemCount);
  for (uint32_t index = 0; index < InitialListTestData::kItemCount; ++index) {
    SCOPED_TRACE(index);
    auto item_instance = list_instance->GetChildInstanceAt(0, index);
    ASSERT_NE(item_instance, nullptr);
    ASSERT_EQ(item_instance->PeekMaterializedRoot(), nullptr);
    auto text_instance = item_instance->GetChildInstanceAt(0, 0);
    ASSERT_NE(text_instance, nullptr);
    ASSERT_EQ(text_instance->PeekMaterializedRoot(), nullptr);
  }

  /**
   * 2. Request the first five visible items through ComponentAtIndex.
   * index is the logical position; operation_id identifies the render request;
   * the returned sign is the materialized item root's impl_id.
   * ComponentAtIndex commits the item subtree internally. Avoid calling GetRoot
   * directly so that only the index request can trigger materialization.
   */
  for (uint32_t index = 0; index < kVisibleItemCount; ++index) {
    SCOPED_TRACE(index);
    const auto item_key = "list-item-" + std::to_string(index);
    auto item_instance = list_instance->GetChildInstanceAt(0, index);
    const int64_t operation_id = static_cast<int64_t>(index) + 1;
    const auto sign =
        list_element->ComponentAtIndex(index, operation_id, false);
    auto item_root = item_instance->PeekMaterializedRoot();
    ASSERT_NE(item_root, nullptr);
    EXPECT_EQ(sign, item_root->impl_id());
    EXPECT_EQ(list_instance_ptr->reuse_pool_.GetInstance(sign), item_instance);
    EXPECT_EQ(item_root->GetTag(), base::String("list-item"));
    EXPECT_TRUE(item_root->is_list_item());
    EXPECT_EQ(item_root->parent(), list_root.get());
    ASSERT_EQ(list_root->children().size(), index + 1u);
    EXPECT_EQ(list_root->children()[index], item_root);

    // Both dynamic instance attributes and static template attributes reach the
    // item root.
    const auto& attributes = item_root->data_model()->attributes();
    ASSERT_EQ(attributes.count("item-key"), 1u);
    EXPECT_EQ(attributes.find("item-key")->second, lepus::Value(item_key));
    ASSERT_EQ(attributes.count("full-span"), 1u);
    EXPECT_EQ(attributes.find("full-span")->second, lepus::Value(true));
    ASSERT_EQ(attributes.count("estimated-main-axis-size-px"), 1u);
    EXPECT_EQ(attributes.find("estimated-main-axis-size-px")->second,
              lepus::Value(100));

    // text is static item structure; raw-text comes from a separate ET in child
    // slot 0.
    ASSERT_EQ(item_root->children().size(), 1u);
    auto text_root = item_root->children()[0];
    ASSERT_TRUE(text_root->is_text());
    EXPECT_EQ(text_root->parent(), item_root.get());
    auto text_instance = item_instance->GetChildInstanceAt(0, 0);
    auto raw_text_root = text_instance->PeekMaterializedRoot();
    ASSERT_NE(raw_text_root, nullptr);
    ASSERT_TRUE(raw_text_root->is_raw_text());
    EXPECT_EQ(raw_text_root->parent(), text_root.get());
    ASSERT_EQ(text_root->children().size(), 1u);
    EXPECT_EQ(text_root->children()[0], raw_text_root);
    // With platform layout, text is stored in the raw-text node's text
    // attribute.
    const auto& text_attributes = raw_text_root->data_model()->attributes();
    const auto content = text_attributes.find(RawTextElement::kTextAttr);
    ASSERT_NE(content, text_attributes.end());
    EXPECT_EQ(content->second, lepus::Value(item_key));
    EXPECT_TRUE(raw_text_root->children().empty());
  }

  /**
   * 3. Flush the platform mock's queue and check item/text node creation.
   * raw-text is a virtual node whose content was checked through C++ above.
   * Only five items have physical nodes; all 20 items and their text ETs retain
   * their original logical parents.
   */
  platform_impl_->Flush();
  ASSERT_EQ(list_root->children().size(), kVisibleItemCount);
  ASSERT_EQ(list_instance->GetChildSlots().GetProperty(0).GetLength(),
            InitialListTestData::kItemCount);
  for (uint32_t index = 0; index < InitialListTestData::kItemCount; ++index) {
    SCOPED_TRACE(index);
    auto item_instance = list_instance->GetChildInstanceAt(0, index);
    EXPECT_EQ(item_instance->GetLogicalParent(), list_instance.get());
    auto text_instance = item_instance->GetChildInstanceAt(0, 0);
    EXPECT_EQ(text_instance->GetLogicalParent(), item_instance.get());
    if (index < kVisibleItemCount) {
      auto item_root = item_instance->PeekMaterializedRoot();
      ASSERT_NE(item_root, nullptr);
      EXPECT_TRUE(HasCapturePlatformNodeTag(item_root->impl_id(), "list-item"));
      ASSERT_EQ(item_root->children().size(), 1u);
      EXPECT_TRUE(HasCapturePlatformNodeTag(item_root->children()[0]->impl_id(),
                                            "text"));
      EXPECT_NE(text_instance->PeekMaterializedRoot(), nullptr);
    } else {
      EXPECT_EQ(item_instance->PeekMaterializedRoot(), nullptr);
      EXPECT_EQ(text_instance->PeekMaterializedRoot(), nullptr);
    }
  }
  EXPECT_FALSE(list_instance_ptr->HasPendingListUpdates());
  EXPECT_FALSE(list_instance_ptr->IsPendingListUpdatesEnqueued());
  EXPECT_TRUE(manager->pending_element_template_list_updates_.empty());
  EXPECT_TRUE(manager->pending_element_template_child_mounts_.empty());
}

TEST_P(ListElementTemplateInstanceTest, PAPIEnqueueComponentMakesItemReusable) {
  /**
   * Purpose
   * Cover ListElement::EnqueueComponent marking an item as reusable.
   *
   * Input
   * PrepareInitialList supplies 20 items sharing one template. Commit the List,
   * get the first item's sign through ComponentAtIndex(0), enqueue it twice,
   * then request another item through ComponentAtIndex(1).
   *
   * Expected
   * 1. The template's reuse queue has one available sign, even after
   * re-enqueue.
   * 2. Enqueueing retains the original item's physical root and sign binding.
   * 3. The next request reuses the same root and sign for the item at index 1;
   *    the item at index 0 releases its binding and the reuse queue becomes
   * empty.
   */
  InitialListTestData initial_list;
  ASSERT_NO_FATAL_FAILURE(PrepareInitialList(initial_list));
  auto* ctx = runtime::MTSRuntime::ToQuickContext(initial_list.runtime.get());
  auto page_instance =
      ElementTemplateInstance::ResolveElementTemplateInstanceValue(
          initial_list.page_value);
  auto page_root = page_instance->PeekMaterializedRoot();
  auto list_instance =
      ElementTemplateInstance::ResolveElementTemplateInstanceValue(
          initial_list.list_value);
  // 1. Mount and commit the List to associate the physical List with its ET.
  lepus::Value insert_args[] = {initial_list.page_value, lepus::Value(0),
                                initial_list.list_value, lepus::Value()};
  RendererFunctions::FiberInsertNodeToElementTemplate(ctx, insert_args, 4);
  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page_root.get());
  platform_impl_->Flush();
  auto list_root = list_instance->PeekMaterializedRoot();
  ASSERT_NE(list_root, nullptr);
  ASSERT_TRUE(list_root->is_list());
  auto* list_element = static_cast<ListElement*>(list_root.get());
  ASSERT_TRUE(list_element->is_element_template_list_);
  auto* list_instance_ptr =
      static_cast<ListElementTemplateInstance*>(list_instance.get());
  auto& reuse_pool = list_instance_ptr->reuse_pool_;

  // 2. Materialize the first item and keep its sign and physical root for
  // reuse.
  auto item_instance = list_instance->GetChildInstanceAt(0, 0);
  auto next_item_instance = list_instance->GetChildInstanceAt(0, 1);
  ASSERT_NE(item_instance, nullptr);
  ASSERT_NE(next_item_instance, nullptr);
  const auto sign = list_element->ComponentAtIndex(0, 1, false);
  auto item_root = item_instance->PeekMaterializedRoot();
  ASSERT_NE(item_root, nullptr);
  ASSERT_EQ(sign, item_root->impl_id());
  ASSERT_TRUE(reuse_pool.recycled_pool_.empty());

  // 3. Recycle through ListElement; check the template group, sign, and
  // availability.
  list_element->EnqueueComponent(sign);
  ASSERT_EQ(reuse_pool.recycled_pool_.size(), 1u);
  const auto pool_it =
      reuse_pool.recycled_pool_.find(item_instance->GetTemplateType());
  ASSERT_NE(pool_it, reuse_pool.recycled_pool_.end());
  ASSERT_EQ(pool_it->second.size(), 1u);
  EXPECT_EQ(pool_it->second.begin()->first, sign);
  EXPECT_TRUE(pool_it->second.begin()->second);

  // Enqueueing the same sign again must leave exactly one available record.
  list_element->EnqueueComponent(sign);
  ASSERT_EQ(pool_it->second.size(), 1u);
  EXPECT_EQ(pool_it->second.begin()->first, sign);
  EXPECT_TRUE(pool_it->second.begin()->second);

  // 4. Request the next item of the same template and check the binding
  // transfer.
  EXPECT_EQ(list_element->ComponentAtIndex(1, 2, false), sign);
  EXPECT_EQ(next_item_instance->PeekMaterializedRoot(), item_root);
  EXPECT_EQ(item_instance->PeekMaterializedRoot(), nullptr);
  EXPECT_EQ(reuse_pool.GetInstance(sign), next_item_instance);
  EXPECT_TRUE(reuse_pool.recycled_pool_.empty());
  ASSERT_EQ(list_root->children().size(), 1u);
  EXPECT_EQ(list_root->children()[0], item_root);
  platform_impl_->Flush();
}

TEST_P(ListElementTemplateInstanceTest,
       PAPIReusesTwoRecycledItemsAfterInitialFive) {
  /**
   * Purpose
   * Materialize five initial items, recycle two, then reuse their physical
   * subtrees for two subsequent items.
   *
   * Input
   * PrepareInitialList supplies 20 items sharing one template, with zero-based
   * keys and text. Request indices 0-4, recycle signs for indices 0 and 1 in
   * order, then request indices 5 and 6, all through ListElement entry points.
   *
   * Expected
   * 1. Indices 5 and 6 receive the roots/signs from indices 0 and 1 in FIFO
   * order.
   * 2. raw-text ETs reuse their nodes; keys and text become list-item-5 and -6.
   * 3. Sign bindings transfer to new items; old item/text ETs release physical
   *    bindings. All ETs retain their original logical parents.
   * 4. Indices 2-4 keep their roots. The List has five physical item children
   * and 20 logical items; reuse creates no additional platform nodes.
   */
  constexpr uint32_t kVisibleItemCount = 5;
  constexpr uint32_t kRecycledItemCount = 2;
  InitialListTestData initial_list;
  ASSERT_NO_FATAL_FAILURE(PrepareInitialList(initial_list));
  auto* ctx = runtime::MTSRuntime::ToQuickContext(initial_list.runtime.get());
  auto page_instance =
      ElementTemplateInstance::ResolveElementTemplateInstanceValue(
          initial_list.page_value);
  auto page_root = page_instance->PeekMaterializedRoot();
  auto list_instance =
      ElementTemplateInstance::ResolveElementTemplateInstanceValue(
          initial_list.list_value);

  // 1. Mount and commit the List to establish its ET association and initial
  // data.
  lepus::Value insert_args[] = {initial_list.page_value, lepus::Value(0),
                                initial_list.list_value, lepus::Value()};
  RendererFunctions::FiberInsertNodeToElementTemplate(ctx, insert_args, 4);
  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page_root.get());
  platform_impl_->Flush();
  auto list_root = list_instance->PeekMaterializedRoot();
  ASSERT_NE(list_root, nullptr);
  ASSERT_TRUE(list_root->is_list());
  auto* list_element = static_cast<ListElement*>(list_root.get());
  ASSERT_TRUE(list_element->is_element_template_list_);
  auto* list_instance_ptr =
      static_cast<ListElementTemplateInstance*>(list_instance.get());
  auto& reuse_pool = list_instance_ptr->reuse_pool_;

  // 2. Materialize five items and retain their signs and roots for reuse
  // checks.
  std::array<int32_t, kVisibleItemCount> initial_signs{};
  std::array<fml::RefPtr<Element>, kVisibleItemCount> initial_item_roots;
  std::array<fml::RefPtr<Element>, kVisibleItemCount> initial_raw_text_roots;
  for (uint32_t index = 0; index < kVisibleItemCount; ++index) {
    SCOPED_TRACE(index);
    initial_signs[index] =
        list_element->ComponentAtIndex(index, index + 1, false);
    auto item_instance = list_instance->GetChildInstanceAt(0, index);
    ASSERT_NE(item_instance, nullptr);
    initial_item_roots[index] = item_instance->PeekMaterializedRoot();
    ASSERT_NE(initial_item_roots[index], nullptr);
    ASSERT_EQ(initial_signs[index], initial_item_roots[index]->impl_id());
    auto raw_text_instance = item_instance->GetChildInstanceAt(0, 0);
    ASSERT_NE(raw_text_instance, nullptr);
    initial_raw_text_roots[index] = raw_text_instance->PeekMaterializedRoot();
    ASSERT_NE(initial_raw_text_roots[index], nullptr);
  }
  platform_impl_->Flush();
  const auto initial_platform_node_count =
      platform_impl_->captured_create_tags_map_.size();
  ASSERT_EQ(list_root->children().size(), kVisibleItemCount);

  // 3. Recycle signs 0 and 1 in order for FIFO reuse by the same template.
  for (uint32_t index = 0; index < kRecycledItemCount; ++index) {
    list_element->EnqueueComponent(initial_signs[index]);
  }

  // 4. Request indices 5 and 6; check subtree transfer, attributes, and ET
  // bindings.
  for (uint32_t index = 0; index < kRecycledItemCount; ++index) {
    const uint32_t next_index = kVisibleItemCount + index;
    SCOPED_TRACE(next_index);
    const auto item_key = "list-item-" + std::to_string(next_index);
    auto old_item_instance = list_instance->GetChildInstanceAt(0, index);
    auto next_item_instance = list_instance->GetChildInstanceAt(0, next_index);
    ASSERT_NE(next_item_instance, nullptr);
    ASSERT_EQ(next_item_instance->PeekMaterializedRoot(), nullptr);

    // The root and sign must transfer to the new item in recycling order.
    const auto sign =
        list_element->ComponentAtIndex(next_index, next_index + 1, false);
    EXPECT_EQ(sign, initial_signs[index]);
    auto item_root = next_item_instance->PeekMaterializedRoot();
    ASSERT_EQ(item_root, initial_item_roots[index]);
    EXPECT_EQ(reuse_pool.GetInstance(sign), next_item_instance);
    EXPECT_EQ(old_item_instance->PeekMaterializedRoot(), nullptr);
    EXPECT_EQ(item_root->parent(), list_root.get());
    // Check the updated item-key attribute.
    const auto& attributes = item_root->data_model()->attributes();
    const auto item_key_attribute = attributes.find("item-key");
    ASSERT_NE(item_key_attribute, attributes.end());
    EXPECT_EQ(item_key_attribute->second, lepus::Value(item_key));

    // The raw-text ET also receives the reused node and updates its text.
    auto old_raw_text_instance = old_item_instance->GetChildInstanceAt(0, 0);
    auto next_raw_text_instance = next_item_instance->GetChildInstanceAt(0, 0);
    ASSERT_NE(old_raw_text_instance, nullptr);
    ASSERT_NE(next_raw_text_instance, nullptr);
    EXPECT_EQ(old_raw_text_instance->PeekMaterializedRoot(), nullptr);
    auto raw_text_root = next_raw_text_instance->PeekMaterializedRoot();
    ASSERT_EQ(raw_text_root, initial_raw_text_roots[index]);
    ASSERT_EQ(item_root->children().size(), 1u);
    auto text_root = item_root->children()[0];
    ASSERT_EQ(text_root->children().size(), 1u);
    EXPECT_EQ(text_root->children()[0], raw_text_root);
    EXPECT_EQ(raw_text_root->parent(), text_root.get());
    const auto& text_attributes = raw_text_root->data_model()->attributes();
    const auto content = text_attributes.find(RawTextElement::kTextAttr);
    ASSERT_NE(content, text_attributes.end());
    EXPECT_EQ(content->second, lepus::Value(item_key));

    // Physical transfer preserves the logical parent of both old and new ETs.
    EXPECT_EQ(old_item_instance->GetLogicalParent(), list_instance.get());
    EXPECT_EQ(next_item_instance->GetLogicalParent(), list_instance.get());
    EXPECT_EQ(old_raw_text_instance->GetLogicalParent(),
              old_item_instance.get());
    EXPECT_EQ(next_raw_text_instance->GetLogicalParent(),
              next_item_instance.get());
  }

  // 5. Flush platform operations; the List must still have only five item
  // subtrees.
  platform_impl_->Flush();
  EXPECT_EQ(platform_impl_->captured_create_tags_map_.size(),
            initial_platform_node_count);
  ASSERT_EQ(list_root->children().size(), kVisibleItemCount);
  for (uint32_t index = 0; index < kVisibleItemCount; ++index) {
    EXPECT_EQ(list_root->children()[index], initial_item_roots[index]);
  }
  for (uint32_t index = kRecycledItemCount; index < kVisibleItemCount;
       ++index) {
    SCOPED_TRACE(index);
    auto item_instance = list_instance->GetChildInstanceAt(0, index);
    EXPECT_EQ(item_instance->PeekMaterializedRoot(), initial_item_roots[index]);
    EXPECT_EQ(reuse_pool.GetInstance(initial_signs[index]), item_instance);
    auto raw_text_instance = item_instance->GetChildInstanceAt(0, 0);
    EXPECT_EQ(raw_text_instance->PeekMaterializedRoot(),
              initial_raw_text_roots[index]);
  }
  EXPECT_EQ(list_instance->GetChildSlots().GetProperty(0).GetLength(),
            InitialListTestData::kItemCount);
  EXPECT_TRUE(reuse_pool.recycled_pool_.empty());
  EXPECT_FALSE(list_instance_ptr->HasPendingListUpdates());
  EXPECT_TRUE(manager->pending_element_template_list_updates_.empty());
  EXPECT_TRUE(manager->pending_element_template_child_mounts_.empty());
}

namespace {

using ::testing::ElementsAre;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::UnorderedElementsAre;

/**
 * Directly test child-slot diff calculation and application.
 * Each ET only has a template type and item-key; ElementManager is nullptr.
 * Strict mocks check callback arguments, order, and unexpected calls.
 */
class ListElementTemplateSlotDiffTest : public ::testing::Test {
 protected:
  using InstanceRefPtr = fml::RefPtr<ElementTemplateInstance>;
  using DiffResult = ElementTemplateInstance::SlotChildrenDiffResult;
  static constexpr size_t kNoMove = DiffResult::kNoMove;
  using TypeCallback = ::testing::StrictMock<::testing::MockFunction<bool(
      const InstanceRefPtr&, const InstanceRefPtr&)>>;
  using MatchCallback = ::testing::StrictMock<::testing::MockFunction<void(
      const InstanceRefPtr&, const InstanceRefPtr&, size_t, size_t)>>;
  using InsertCallback =
      ::testing::StrictMock<::testing::MockFunction<InstanceRefPtr(
          const InstanceRefPtr&, const InstanceRefPtr&)>>;
  using RemoveCallback = ::testing::StrictMock<
      ::testing::MockFunction<void(const InstanceRefPtr&)>>;
  using MoveCallback = ::testing::StrictMock<::testing::MockFunction<void(
      const InstanceRefPtr&, const InstanceRefPtr&)>>;

  static void SetItemKey(const InstanceRefPtr& item, const lepus::Value& key) {
    auto info = lepus::Dictionary::Create();
    info->SetValue("item-key", key);
    item->SetListItemPlatformInfo(lepus::Value(std::move(info)));
  }

  static InstanceRefPtr CreateItem(const char* key,
                                   const char* template_key = "Card") {
    auto item = fml::AdoptRef(new ElementTemplateInstance(nullptr));
    item->SetTemplateKey(base::String(template_key));
    SetItemKey(item, lepus::Value(key));
    return item;
  }

  static bool IsSameType(const InstanceRefPtr& before,
                         const InstanceRefPtr& after) {
    return before->IsTypedTemplate() == after->IsTypedTemplate() &&
           before->GetTemplateType() == after->GetTemplateType();
  }
};

TEST_F(ListElementTemplateSlotDiffTest, DiffEmptyInputs) {
  /**
   * Purpose: Check the diff of empty inputs.
   * Input: before=[], after=[].
   * Expected: Empty operation arrays, zero counts, and no match callbacks.
   */
  MatchCallback on_match;
  auto diff = ElementTemplateInstance::DiffSlotChildren(
      {}, {}, IsSameType, on_match.AsStdFunction(), false);

  EXPECT_TRUE(diff.inserts_.empty());
  EXPECT_TRUE(diff.removes_.empty());
  EXPECT_TRUE(diff.moves_.empty());
  EXPECT_EQ(diff.insert_count_, 0u);
  EXPECT_EQ(diff.move_count_, 0u);
}

TEST_F(ListElementTemplateSlotDiffTest, DiffInsertionsPreserveNullPositions) {
  /**
   * Purpose: Check insertions and null placeholders in after.
   * Input: before=[], after=[null,a1,null,b1].
   * Expected: Insert at new indices 1 and 3, preserving null positions; no
   * matches.
   */
  auto a1 = CreateItem("a1");
  auto b1 = CreateItem("b1");
  MatchCallback on_match;
  auto diff = ElementTemplateInstance::DiffSlotChildren(
      {}, {nullptr, a1, nullptr, b1}, IsSameType, on_match.AsStdFunction(),
      true);

  EXPECT_THAT(diff.inserts_, ElementsAre(nullptr, a1, nullptr, b1));
  EXPECT_TRUE(diff.removes_.empty());
  EXPECT_TRUE(diff.moves_.empty());
  EXPECT_EQ(diff.insert_count_, 2u);
  EXPECT_EQ(diff.move_count_, 0u);
}

TEST_F(ListElementTemplateSlotDiffTest, DiffRemovalsPreserveNullIndices) {
  /**
   * Purpose: Check removals and null placeholders in before.
   * Input: before=[null,a0,null,b0], after=[].
   * Expected: Remove old indices 1 and 3; all moves are kNoMove; no matches.
   */
  auto a0 = CreateItem("a0");
  auto b0 = CreateItem("b0");
  MatchCallback on_match;
  auto diff = ElementTemplateInstance::DiffSlotChildren(
      {nullptr, a0, nullptr, b0}, {}, IsSameType, on_match.AsStdFunction(),
      true);

  EXPECT_TRUE(diff.inserts_.empty());
  // removes_ must contain exactly indices 1 and 3, in either order.
  EXPECT_THAT(diff.removes_, UnorderedElementsAre(1u, 3u));
  // moves_ must contain exactly four entries, all equal to kNoMove.
  EXPECT_THAT(diff.moves_, ElementsAre(kNoMove, kNoMove, kNoMove, kNoMove));
  EXPECT_EQ(diff.insert_count_, 0u);
  EXPECT_EQ(diff.move_count_, 0u);
}

TEST_F(ListElementTemplateSlotDiffTest,
       DiffByTypeMixesRepeatedTypesInsertionsRemovalsAndMoves) {
  /**
   * Purpose: Check mixed insert/remove/move operations with repeated templates.
   * Input: before=[A(a_0),B(b_0),A(a_1)], after=[B(b_0),C(c_0),A(a_1)],
   *        has_item_key=false.
   * Matches: old_b_0->new_b_0, old_a_0->new_a_1.
   * Expected: Insert new_c_0 at new index 1, remove old_a_1 at old index 2,
   *           and move old_a_0 from old index 0 to new index 2.
   */
  // Old list: [A(a_0), B(b_0), A(a_1)].
  auto old_a_0 = CreateItem("a_0", "_et_A");
  auto old_b_0 = CreateItem("b_0", "_et_B");
  auto old_a_1 = CreateItem("a_1", "_et_A");

  // New list: [B(b_0), C(c_0), A(a_1)].
  auto new_b_0 = CreateItem("b_0", "_et_B");
  auto new_c_0 = CreateItem("c_0", "_et_C");
  auto new_a_1 = CreateItem("a_1", "_et_A");

  MatchCallback on_match;
  InSequence sequence;
  EXPECT_CALL(on_match, Call(old_b_0, new_b_0, 1u, 0u));
  EXPECT_CALL(on_match, Call(old_a_0, new_a_1, 0u, 2u));
  auto diff = ElementTemplateInstance::DiffSlotChildren(
      {old_a_0, old_b_0, old_a_1}, {new_b_0, new_c_0, new_a_1}, IsSameType,
      on_match.AsStdFunction(), false);

  EXPECT_THAT(diff.inserts_, ElementsAre(nullptr, new_c_0, nullptr));
  EXPECT_THAT(diff.removes_, ElementsAre(2u));
  EXPECT_THAT(diff.moves_, ElementsAre(2u, kNoMove, kNoMove));
  EXPECT_EQ(diff.insert_count_, 1u);
  EXPECT_EQ(diff.move_count_, 1u);
}

TEST_F(ListElementTemplateSlotDiffTest, DiffByTypeUsesFifoAcrossReorderedKeys) {
  /**
   * Purpose: Check FIFO consumption of same-template candidates despite key
   * order. Input: before=[A(a_0),A(a_1),B(b_0)], after=[A(a_1),B(b_0),A(a_0)],
   *        has_item_key=false.
   * Matches: old_a_0->new_a_1, old_b_0->new_b_0, old_a_1->new_a_0.
   * Expected: Move old_a_1 from old index 1 to new index 2; no inserts/removes.
   */
  // Old list: [A(a_0), A(a_1), B(b_0)].
  auto old_a_0 = CreateItem("a_0", "_et_A");
  auto old_a_1 = CreateItem("a_1", "_et_A");
  auto old_b_0 = CreateItem("b_0", "_et_B");

  // New list: [A(a_1), B(b_0), A(a_0)].
  auto new_a_1 = CreateItem("a_1", "_et_A");
  auto new_b_0 = CreateItem("b_0", "_et_B");
  auto new_a_0 = CreateItem("a_0", "_et_A");

  MatchCallback on_match;
  InSequence sequence;
  EXPECT_CALL(on_match, Call(old_a_0, new_a_1, 0u, 0u));
  EXPECT_CALL(on_match, Call(old_b_0, new_b_0, 2u, 1u));
  EXPECT_CALL(on_match, Call(old_a_1, new_a_0, 1u, 2u));
  auto diff = ElementTemplateInstance::DiffSlotChildren(
      {old_a_0, old_a_1, old_b_0}, {new_a_1, new_b_0, new_a_0}, IsSameType,
      on_match.AsStdFunction(), false);

  EXPECT_THAT(diff.inserts_, ElementsAre(nullptr, nullptr, nullptr));
  EXPECT_TRUE(diff.removes_.empty());
  EXPECT_THAT(diff.moves_, ElementsAre(kNoMove, 2u, kNoMove));
  EXPECT_EQ(diff.insert_count_, 0u);
  EXPECT_EQ(diff.move_count_, 1u);
}

TEST_F(ListElementTemplateSlotDiffTest,
       DiffByTypeInsertsAfterCandidatesAreExhausted) {
  /**
   * Purpose: Insert excess new instances once same-template candidates run out.
   * Input: before=[A(a_0)], after=[A(a_1),A(a_0)], has_item_key=false.
   * Matches: old_a_0->new_a_1, exhausting the old _et_A candidates.
   * Expected: Insert new_a_0 at new index 1; no removes or moves.
   */
  auto old_a_0 = CreateItem("a_0", "_et_A");
  auto new_a_1 = CreateItem("a_1", "_et_A");
  auto new_a_0 = CreateItem("a_0", "_et_A");
  MatchCallback on_match;
  EXPECT_CALL(on_match, Call(old_a_0, new_a_1, 0u, 0u));
  auto diff = ElementTemplateInstance::DiffSlotChildren(
      {old_a_0}, {new_a_1, new_a_0}, IsSameType, on_match.AsStdFunction(),
      false);

  EXPECT_THAT(diff.inserts_, ElementsAre(nullptr, new_a_0));
  EXPECT_TRUE(diff.removes_.empty());
  EXPECT_THAT(diff.moves_, ElementsAre(kNoMove));
  EXPECT_EQ(diff.insert_count_, 1u);
  EXPECT_EQ(diff.move_count_, 0u);
}

TEST_F(ListElementTemplateSlotDiffTest,
       DiffByTypeMixedOperationsPreserveNullIndices) {
  /**
   * Purpose: Preserve null placeholders and original indices in a mixed type
   * diff. Input: Match by template type with has_item_key=false. Letters
   * a/b/c/d correspond to _et_A/_et_B/_et_C/_et_D.
   *
   * index  | 0       | 1       | 2       | 3       | 4       | 5
   * before | null    | old_a_0 | old_b_0 | null    | old_a_1 | old_c_0
   * after  | new_b_0 | null    | new_a_0 | new_d_0 | new_a_1 | null
   *
   * Matches: old_b_0->new_b_0 (2->0), old_a_0->new_a_0 (1->2),
   *          old_a_1->new_a_1 (4->4). The A group consumes old candidates in
   * order.
   *
   * Expected: Insert new_d_0 at new index 3; remove old_c_0 at old index 5.
   *           Matching old_b_0 sets last_placed_index=2. old_a_0 has old index
   * 1, so record its move from old index 1 to new index 2. inserts_ uses new
   * indices: [null,null,null,new_d_0,null,null]. removes_ holds old indices:
   * [5]. moves_ uses old indices: [kNoMove,2,kNoMove,kNoMove,kNoMove,kNoMove].
   */
  // The old list contains two A instances and nulls at indices 0 and 3.
  auto old_a_0 = CreateItem("a_0", "_et_A");
  auto old_b_0 = CreateItem("b_0", "_et_B");
  auto old_a_1 = CreateItem("a_1", "_et_A");
  auto old_c_0 = CreateItem("c_0", "_et_C");

  // The new list contains B, A, D, A and nulls at indices 1 and 5.
  auto new_b_0 = CreateItem("b_0", "_et_B");
  auto new_a_0 = CreateItem("a_0", "_et_A");
  auto new_d_0 = CreateItem("d_0", "_et_D");
  auto new_a_1 = CreateItem("a_1", "_et_A");
  const base::Vector<InstanceRefPtr> before_instances = {
      nullptr, old_a_0, old_b_0, nullptr, old_a_1, old_c_0};
  const base::Vector<InstanceRefPtr> after_instances = {
      new_b_0, nullptr, new_a_0, new_d_0, new_a_1, nullptr};

  MatchCallback on_match;
  InSequence sequence;
  EXPECT_CALL(on_match, Call(old_b_0, new_b_0, 2u, 0u));
  EXPECT_CALL(on_match, Call(old_a_0, new_a_0, 1u, 2u));
  EXPECT_CALL(on_match, Call(old_a_1, new_a_1, 4u, 4u));
  auto diff = ElementTemplateInstance::DiffSlotChildren(
      before_instances, after_instances, IsSameType, on_match.AsStdFunction(),
      false);

  // inserts_[3] holds the new D instance at new index 3.
  EXPECT_THAT(diff.inserts_, ElementsAre(nullptr, nullptr, nullptr, new_d_0,
                                         nullptr, nullptr));
  // removes_[0]=5 removes before_instances[5], which is old_c_0.
  EXPECT_THAT(diff.removes_, ElementsAre(5u));
  // moves_[1]=2 moves old_a_0 from old index 1 to after_instances index 2.
  EXPECT_THAT(diff.moves_,
              ElementsAre(kNoMove, 2u, kNoMove, kNoMove, kNoMove, kNoMove));
  EXPECT_EQ(diff.insert_count_, 1u);
  EXPECT_EQ(diff.move_count_, 1u);
}

TEST_F(ListElementTemplateSlotDiffTest, DiffByKeyRecordsInsertRemoveAndMove) {
  /**
   * Purpose: Check complete operation arrays and match callbacks for a mixed
   * diff. Input: before=[a0,b0,c0,d0], after=[c1,x1,a1,d1], matched by
   * item-key. Instances with the same letter and suffixes 0/1 share an
   * item-key. Expected: Insert x1 at new index 1, remove b0 at old index 1, and
   * move a0 from old index 0 to new index 2.
   */
  auto a0 = CreateItem("a");
  auto b0 = CreateItem("b");
  auto c0 = CreateItem("c");
  auto d0 = CreateItem("d");
  auto c1 = CreateItem("c");
  auto x1 = CreateItem("x");
  auto a1 = CreateItem("a");
  auto d1 = CreateItem("d");
  MatchCallback on_match;
  InSequence sequence;
  EXPECT_CALL(on_match, Call(c0, c1, 2u, 0u));
  EXPECT_CALL(on_match, Call(a0, a1, 0u, 2u));
  EXPECT_CALL(on_match, Call(d0, d1, 3u, 3u));
  auto diff = ElementTemplateInstance::DiffSlotChildren(
      {a0, b0, c0, d0}, {c1, x1, a1, d1}, IsSameType, on_match.AsStdFunction(),
      true);

  EXPECT_THAT(diff.inserts_, ElementsAre(nullptr, x1, nullptr, nullptr));
  EXPECT_THAT(diff.removes_, ElementsAre(1u));
  EXPECT_THAT(diff.moves_, ElementsAre(2u, kNoMove, kNoMove, kNoMove));
  EXPECT_EQ(diff.insert_count_, 1u);
  EXPECT_EQ(diff.move_count_, 1u);
}

TEST_F(ListElementTemplateSlotDiffTest, DiffEmptyStringKeyHasItsOwnBucket) {
  /**
   * Purpose: Keep missing keys and empty-string keys in separate groups.
   * Input: before=[a0(missing),b0(empty)], after=[b1(empty),a1(missing)].
   * Expected: Match b0->b1 and a0->a1; move a0 to new index 1.
   */
  auto a0 = CreateItem("a0");
  auto b0 = CreateItem("");
  auto b1 = CreateItem("");
  auto a1 = CreateItem("a1");
  a0->SetOptions(lepus::Value());
  a1->SetOptions(lepus::Value());
  MatchCallback on_match;
  InSequence sequence;
  EXPECT_CALL(on_match, Call(b0, b1, 1u, 0u));
  EXPECT_CALL(on_match, Call(a0, a1, 0u, 1u));
  auto diff = ElementTemplateInstance::DiffSlotChildren(
      {a0, b0}, {b1, a1}, IsSameType, on_match.AsStdFunction(), true);

  EXPECT_THAT(diff.inserts_, ElementsAre(nullptr, nullptr));
  EXPECT_TRUE(diff.removes_.empty());
  EXPECT_THAT(diff.moves_, ElementsAre(1u, kNoMove));
  EXPECT_EQ(diff.insert_count_, 0u);
  EXPECT_EQ(diff.move_count_, 1u);
}

TEST_F(ListElementTemplateSlotDiffTest,
       DiffMatchesUseOriginalIndicesAcrossNulls) {
  /**
   * Purpose: Use original indices for matches and moves when skipping nulls.
   * Input: before=[null,a0,null,b0], after=[b1,null,a1,null].
   * Instances with the same letter and suffixes 0/1 share an item-key.
   * Expected: Match indices (3,0) and (1,2); move a0 from old index 1 to new
   * index 2.
   */
  auto a0 = CreateItem("a");
  auto b0 = CreateItem("b");
  auto b1 = CreateItem("b");
  auto a1 = CreateItem("a");
  MatchCallback on_match;
  InSequence sequence;
  EXPECT_CALL(on_match, Call(b0, b1, 3u, 0u));
  EXPECT_CALL(on_match, Call(a0, a1, 1u, 2u));
  auto diff = ElementTemplateInstance::DiffSlotChildren(
      {nullptr, a0, nullptr, b0}, {b1, nullptr, a1, nullptr}, IsSameType,
      on_match.AsStdFunction(), true);

  EXPECT_THAT(diff.inserts_, ElementsAre(nullptr, nullptr, nullptr, nullptr));
  EXPECT_TRUE(diff.removes_.empty());
  EXPECT_THAT(diff.moves_, ElementsAre(kNoMove, 2u, kNoMove, kNoMove));
  EXPECT_EQ(diff.insert_count_, 0u);
  EXPECT_EQ(diff.move_count_, 1u);
}

TEST_F(ListElementTemplateSlotDiffTest, ApplyEmptyDiffReturnsOldInstances) {
  /**
   * Purpose: Check the result and structural callbacks for an empty diff.
   * Input: before=[a0,b0], with no operations.
   * Expected: Return the original instances [a0,b0]; no insert/remove/move
   * callbacks.
   */
  auto a0 = CreateItem("a0");
  auto b0 = CreateItem("b0");
  DiffResult diff;
  InsertCallback on_insert;
  RemoveCallback on_remove;
  MoveCallback on_move;
  auto result = ElementTemplateInstance::ApplySlotChildrenDiffResult(
      {a0, b0}, diff, on_insert.AsStdFunction(), on_remove.AsStdFunction(),
      on_move.AsStdFunction());

  EXPECT_THAT(result, ElementsAre(a0, b0));
}

TEST_F(ListElementTemplateSlotDiffTest, ApplyInsertsIntoEmptySequence) {
  /**
   * Purpose: Check consecutive insertions into an empty old array.
   * Input: before=[]; insert a1 and b1 at new indices 0 and 1.
   * Expected: Call insert(a1,null), then insert(b1,null); return [a1,b1].
   */
  auto a1 = CreateItem("a1");
  auto b1 = CreateItem("b1");
  DiffResult diff;
  diff.inserts_ = {a1, b1};
  diff.insert_count_ = 2;
  InsertCallback on_insert;
  RemoveCallback on_remove;
  MoveCallback on_move;
  InSequence sequence;
  EXPECT_CALL(on_insert, Call(a1, ::testing::IsNull())).WillOnce(Return(a1));
  EXPECT_CALL(on_insert, Call(b1, ::testing::IsNull())).WillOnce(Return(b1));
  auto result = ElementTemplateInstance::ApplySlotChildrenDiffResult(
      {}, diff, on_insert.AsStdFunction(), on_remove.AsStdFunction(),
      on_move.AsStdFunction());

  EXPECT_THAT(result, ElementsAre(a1, b1));
}

TEST_F(ListElementTemplateSlotDiffTest,
       ApplyInsertionsKeepAnchorAndStoreReturnValue) {
  /**
   * Purpose: Check consecutive insertion anchors, appends, and callback
   * results. Input: before=[a0,b0]; insert c1,d1,e1 at new indices 0,1,4;
   *        inserting c1 returns x1.
   * Expected: Anchors are a0,a0,null; return [x1,d1,a0,b0,e1].
   */
  auto a0 = CreateItem("a0");
  auto b0 = CreateItem("b0");
  auto c1 = CreateItem("c1");
  auto d1 = CreateItem("d1");
  auto e1 = CreateItem("e1");
  auto x1 = CreateItem("x1");
  DiffResult diff;
  diff.inserts_ = {c1, d1, nullptr, nullptr, e1};
  diff.moves_ = {kNoMove, kNoMove};
  diff.insert_count_ = 3;
  InsertCallback on_insert;
  RemoveCallback on_remove;
  MoveCallback on_move;
  InSequence sequence;
  EXPECT_CALL(on_insert, Call(c1, a0)).WillOnce(Return(x1));
  EXPECT_CALL(on_insert, Call(d1, a0)).WillOnce(Return(d1));
  EXPECT_CALL(on_insert, Call(e1, ::testing::IsNull())).WillOnce(Return(e1));
  auto result = ElementTemplateInstance::ApplySlotChildrenDiffResult(
      {a0, b0}, diff, on_insert.AsStdFunction(), on_remove.AsStdFunction(),
      on_move.AsStdFunction());

  EXPECT_THAT(result, ElementsAre(x1, d1, a0, b0, e1));
}

TEST_F(ListElementTemplateSlotDiffTest, ApplyRemovalsFollowOldSequenceOrder) {
  /**
   * Purpose: Invoke removal callbacks in old-array order.
   * Input: before=[a0,b0,c0,d0], removes=[3,1].
   * Expected: Call remove(b0), then remove(d0); retain [a0,c0].
   */
  auto a0 = CreateItem("a0");
  auto b0 = CreateItem("b0");
  auto c0 = CreateItem("c0");
  auto d0 = CreateItem("d0");
  DiffResult diff;
  diff.removes_ = {3, 1};
  diff.moves_ = {kNoMove, kNoMove, kNoMove, kNoMove};
  InsertCallback on_insert;
  RemoveCallback on_remove;
  MoveCallback on_move;
  InSequence sequence;
  EXPECT_CALL(on_remove, Call(b0));
  EXPECT_CALL(on_remove, Call(d0));
  auto result = ElementTemplateInstance::ApplySlotChildrenDiffResult(
      {a0, b0, c0, d0}, diff, on_insert.AsStdFunction(),
      on_remove.AsStdFunction(), on_move.AsStdFunction());

  EXPECT_THAT(result, ElementsAre(a0, c0));
}

TEST_F(ListElementTemplateSlotDiffTest, ApplyRemovesAllInstances) {
  /**
   * Purpose: Stop correctly after removing all old instances.
   * Input: before=[a0,b0], removes=[0,1].
   * Expected: Remove a0 then b0; return []; no insert/move callbacks.
   */
  auto a0 = CreateItem("a0");
  auto b0 = CreateItem("b0");
  DiffResult diff;
  diff.removes_ = {0, 1};
  diff.moves_ = {kNoMove, kNoMove};
  InsertCallback on_insert;
  RemoveCallback on_remove;
  MoveCallback on_move;
  InSequence sequence;
  EXPECT_CALL(on_remove, Call(a0));
  EXPECT_CALL(on_remove, Call(b0));
  auto result = ElementTemplateInstance::ApplySlotChildrenDiffResult(
      {a0, b0}, diff, on_insert.AsStdFunction(), on_remove.AsStdFunction(),
      on_move.AsStdFunction());

  EXPECT_TRUE(result.empty());
}

TEST_F(ListElementTemplateSlotDiffTest,
       ApplyConsecutiveMovesKeepCurrentAnchor) {
  /**
   * Purpose: Anchor consecutive buffered moves to the current old instance.
   * Input: before=[a0,b0,c0,d0]; move a0 to new index 1 and b0 to new index 2.
   * Expected: Call move(a0,d0), then move(b0,d0); return [c0,a0,b0,d0].
   */
  auto a0 = CreateItem("a0");
  auto b0 = CreateItem("b0");
  auto c0 = CreateItem("c0");
  auto d0 = CreateItem("d0");
  DiffResult diff;
  diff.moves_ = {1, 2, kNoMove, kNoMove};
  diff.move_count_ = 2;
  InsertCallback on_insert;
  RemoveCallback on_remove;
  MoveCallback on_move;
  InSequence sequence;
  EXPECT_CALL(on_move, Call(a0, d0));
  EXPECT_CALL(on_move, Call(b0, d0));
  auto result = ElementTemplateInstance::ApplySlotChildrenDiffResult(
      {a0, b0, c0, d0}, diff, on_insert.AsStdFunction(),
      on_remove.AsStdFunction(), on_move.AsStdFunction());

  EXPECT_THAT(result, ElementsAre(c0, a0, b0, d0));
}

TEST_F(ListElementTemplateSlotDiffTest, ApplyMovesAfterOldSequenceEnds) {
  /**
   * Purpose: Emit remaining buffered moves after exhausting the old array.
   * Input: before=[a0,b0,c0]; move a0 to new index 1 and b0 to new index 2.
   * Expected: Call move(a0,null), then move(b0,null); return [c0,a0,b0].
   */
  auto a0 = CreateItem("a0");
  auto b0 = CreateItem("b0");
  auto c0 = CreateItem("c0");
  DiffResult diff;
  diff.moves_ = {1, 2, kNoMove};
  diff.move_count_ = 2;
  InsertCallback on_insert;
  RemoveCallback on_remove;
  MoveCallback on_move;
  InSequence sequence;
  EXPECT_CALL(on_move, Call(a0, ::testing::IsNull()));
  EXPECT_CALL(on_move, Call(b0, ::testing::IsNull()));
  auto result = ElementTemplateInstance::ApplySlotChildrenDiffResult(
      {a0, b0, c0}, diff, on_insert.AsStdFunction(), on_remove.AsStdFunction(),
      on_move.AsStdFunction());

  EXPECT_THAT(result, ElementsAre(c0, a0, b0));
}

TEST_F(ListElementTemplateSlotDiffTest,
       ApplyMixedDiffChecksCallbackOrderAndAnchors) {
  /**
   * Purpose: Check callback order, anchors, and results for a mixed diff.
   * Input: before=[a0,b0,c0,d0]; remove b0, insert x1 at new index 1, and move
   *        a0 to new index 2.
   * Expected: Call remove(b0), insert(x1,d0), then move(a0,d0);
   *           return [c0,x1,a0,d0].
   */
  auto a0 = CreateItem("a0");
  auto b0 = CreateItem("b0");
  auto c0 = CreateItem("c0");
  auto d0 = CreateItem("d0");
  auto x1 = CreateItem("x1");
  DiffResult diff;
  diff.inserts_ = {nullptr, x1, nullptr, nullptr};
  diff.removes_ = {1};
  diff.moves_ = {2, kNoMove, kNoMove, kNoMove};
  diff.insert_count_ = 1;
  diff.move_count_ = 1;
  InsertCallback on_insert;
  RemoveCallback on_remove;
  MoveCallback on_move;
  InSequence sequence;
  EXPECT_CALL(on_remove, Call(b0));
  EXPECT_CALL(on_insert, Call(x1, d0)).WillOnce(Return(x1));
  EXPECT_CALL(on_move, Call(a0, d0));
  auto result = ElementTemplateInstance::ApplySlotChildrenDiffResult(
      {a0, b0, c0, d0}, diff, on_insert.AsStdFunction(),
      on_remove.AsStdFunction(), on_move.AsStdFunction());

  EXPECT_THAT(result, ElementsAre(c0, x1, a0, d0));
}

}  // namespace

INSTANTIATE_TEST_SUITE_P(ListElementTemplateInstanceTestModule,
                         ListElementTemplateInstanceTest,
                         ::testing::ValuesIn(fiber_element_generation_params));

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
