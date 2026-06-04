// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/dom/element_tree_serializer.h"

#include <unordered_map>
#include <utility>

#include "core/base/threading/task_runner_manufactor.h"
#include "core/public/page_options.h"
#include "core/renderer/css/css_property_id.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/component_element.h"
#include "core/renderer/dom/fiber/fiber_element.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/starlight/layout/layout_object.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/renderer/template_entry.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/shell/tasm_operation_queue.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

namespace {

static constexpr int32_t kWidth = 1080;
static constexpr int32_t kHeight = 1920;
static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;

lepus::Value GetProperty(const lepus::Value& value, const char* key) {
  return value.GetProperty(base::String(key));
}

void ExpectRect(const lepus::Value& rect, float x, float y, float width,
                float height) {
  EXPECT_EQ(GetProperty(rect, "x").Number(), x);
  EXPECT_EQ(GetProperty(rect, "y").Number(), y);
  EXPECT_EQ(GetProperty(rect, "width").Number(), width);
  EXPECT_EQ(GetProperty(rect, "height").Number(), height);
}

void SetLayoutObjectFrame(Element* element, float x, float y, float width,
                          float height) {
  static_cast<FiberElement*>(element)->EnsureSLNode();
  auto* layout_object = element->GetLayoutObject();
  ASSERT_NE(layout_object, nullptr);
  layout_object->SetBorderBoundLeftFromParentPaddingBound(x);
  layout_object->SetBorderBoundTopFromParentPaddingBound(y);
  layout_object->SetBorderBoundWidth(width);
  layout_object->SetBorderBoundHeight(height);
}

class NodeInfoPaintingContext : public MockPaintingContext {
 public:
  void SetRectToLynxView(int64_t id, std::vector<float> rect) {
    rect_to_lynx_view_[id] = std::move(rect);
  }

  void SetRectToScreen(int64_t id, std::vector<float> rect) {
    rect_to_screen_[id] = std::move(rect);
  }

  std::vector<float> GetRectToLynxView(int64_t id) override {
    auto iter = rect_to_lynx_view_.find(id);
    return iter == rect_to_lynx_view_.end() ? std::vector<float>()
                                            : iter->second;
  }

  void GetRectToScreen(int id, float* rect) override {
    if (rect == nullptr) {
      return;
    }
    auto iter = rect_to_screen_.find(id);
    if (iter == rect_to_screen_.end()) {
      return;
    }
    for (size_t i = 0; i < iter->second.size() && i < 4; ++i) {
      rect[i] = iter->second[i];
    }
  }

 private:
  std::unordered_map<int64_t, std::vector<float>> rect_to_lynx_view_;
  std::unordered_map<int64_t, std::vector<float>> rect_to_screen_;
};

}  // namespace

class ElementTreeSerializerTest : public ::testing::Test {
 public:
  void SetUp() override {
    LynxEnvConfig lynx_env_config(kWidth, kHeight, kDefaultLayoutsUnitPerPx,
                                  kDefaultPhysicalPixelsPerLayoutUnit);
    tasm_mediator_ = std::make_shared<
        ::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>();
    auto painting_context = std::make_unique<NodeInfoPaintingContext>();
    painting_context_ptr_ = painting_context.get();
    auto manager = std::make_unique<lynx::tasm::ElementManager>(
        std::move(painting_context), tasm_mediator_.get(), lynx_env_config);
    manager_ = manager.get();
    tasm_ = std::make_shared<lynx::tasm::TemplateAssembler>(
        *tasm_mediator_.get(), std::move(manager), tasm_mediator_.get(), 0);
    PageOptions page_options;
    page_options.SetEmbeddedMode(EmbeddedMode::LAYOUT_IN_ELEMENT);
    manager_->SetPageOptions(page_options);
    auto config = std::make_shared<PageConfig>();
    config->SetEnableFiberArch(true);
    config->SetEnableZIndex(true);
    config->SetDebugMetadataUrl("https://example.com/debug-info.json");
    manager_->SetConfig(config);
  }

 protected:
  NodeInfoPaintingContext* painting_context_ptr_{nullptr};
  lynx::tasm::ElementManager* manager_{nullptr};
  std::shared_ptr<lynx::tasm::TemplateAssembler> tasm_;
  std::shared_ptr<::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>
      tasm_mediator_;
};

TEST_F(ElementTreeSerializerTest, ToLepusValueIncludesNodeInfoFields) {
  auto root = manager_->CreateFiberPage("0", 10);
  root->SetNodeIndex(100);
  root->SetIdSelector("root");
  root->data_model()->AddClass("page-class");
  root->data_model()->SetDataSet("test", lepus::Value("root-data"));

  auto child = manager_->CreateFiberNode("text");
  child->SetIdSelector("label");
  child->SetNodeIndex(200);
  child->data_model()->SetStaticAttribute("name", lepus::Value("title"));
  child->data_model()->SetStaticAttribute("hidden", lepus::Value(true));
  child->data_model()->SetStaticAttribute("ignored_nil", lepus::Value());
  child->SetStyle(kPropertyIDWidth, lepus::Value("10px"));
  child->SetStyle(kPropertyIDOpacity, lepus::Value(0.5));
  child->data_model()->AddClass("primary");
  child->data_model()->AddClass("large");
  child->SetJSEventHandler("tap", "bindEvent", "onTap");
  child->SetLepusEventHandler("focus", "catchEvent", lepus::Value("script"),
                              lepus::Value("handler"));
  root->InsertNode(child);

  painting_context_ptr_->SetRectToLynxView(root->impl_id(),
                                           {1.f, 2.f, 300.f, 400.f});
  painting_context_ptr_->SetRectToScreen(root->impl_id(),
                                         {11.f, 22.f, 300.f, 400.f});
  painting_context_ptr_->SetRectToLynxView(child->impl_id(),
                                           {5.f, 6.f, 70.f, 80.f});
  painting_context_ptr_->SetRectToScreen(child->impl_id(),
                                         {15.f, 26.f, 70.f, 80.f});

  auto root_info = ElementTreeSerializer::ToLepusValue(root.get());

  EXPECT_TRUE(GetProperty(root_info, "sign").IsNil());
  EXPECT_TRUE(GetProperty(root_info, "tagName").IsNil());
  EXPECT_EQ(GetProperty(root_info, "id").StdString(), "root");
  EXPECT_TRUE(GetProperty(root_info, "nodeIndex").IsNil());

  EXPECT_EQ(GetProperty(root_info, "nodeId").Int32(), root->impl_id());
  EXPECT_EQ(GetProperty(root_info, "tag").StdString(), "page");
  auto root_attributes = GetProperty(root_info, "attributes");
  EXPECT_EQ(GetProperty(root_attributes, "id").StdString(), "root");
  ASSERT_TRUE(GetProperty(root_attributes, "class").IsArray());
  EXPECT_EQ(GetProperty(root_attributes, "class").Array()->get(0).StdString(),
            "page-class");
  EXPECT_EQ(GetProperty(root_attributes, "data-test").StdString(), "root-data");

  auto root_position = GetProperty(root_info, "position");
  ExpectRect(GetProperty(root_position, "frameInRoot"), 1.f, 2.f, 300.f, 400.f);
  ExpectRect(GetProperty(root_position, "frameInScreen"), 11.f, 22.f, 300.f,
             400.f);

  auto root_debug_info = GetProperty(root_info, "debugInfo");
  EXPECT_EQ(GetProperty(root_debug_info, "nodeIndex").UInt32(), 100U);
  EXPECT_EQ(GetProperty(root_debug_info, "debugMetadataUrl").StdString(),
            "https://example.com/debug-info.json");

  auto children = GetProperty(root_info, "children");
  ASSERT_TRUE(children.IsArray());
  ASSERT_EQ(children.Array()->size(), 1U);
  auto child_info = children.Array()->get(0);
  EXPECT_EQ(GetProperty(child_info, "nodeId").Int32(), child->impl_id());
  EXPECT_EQ(GetProperty(child_info, "tag").StdString(), "text");
  EXPECT_TRUE(GetProperty(child_info, "tagName").IsNil());

  auto child_attributes = GetProperty(child_info, "attributes");
  EXPECT_EQ(GetProperty(child_attributes, "id").StdString(), "label");
  EXPECT_EQ(GetProperty(child_attributes, "name").StdString(), "title");
  EXPECT_TRUE(GetProperty(child_attributes, "hidden").Bool());
  EXPECT_TRUE(GetProperty(child_attributes, "ignored_nil").IsNil());
  ASSERT_TRUE(GetProperty(child_attributes, "class").IsArray());
  EXPECT_EQ(GetProperty(child_attributes, "class").Array()->get(0).StdString(),
            "primary");
  EXPECT_EQ(GetProperty(child_attributes, "class").Array()->get(1).StdString(),
            "large");
  auto child_style = GetProperty(child_attributes, "style");
  EXPECT_EQ(GetProperty(child_style, "width").StdString(), "10px");
  EXPECT_EQ(GetProperty(child_style, "opacity").Number(), 0.5);

  auto child_position = GetProperty(child_info, "position");
  ExpectRect(GetProperty(child_position, "frameInRoot"), 5.f, 6.f, 70.f, 80.f);
  ExpectRect(GetProperty(child_position, "frameInScreen"), 15.f, 26.f, 70.f,
             80.f);

  auto child_debug_info = GetProperty(child_info, "debugInfo");
  EXPECT_EQ(GetProperty(child_debug_info, "nodeIndex").UInt32(), 200U);
  EXPECT_EQ(GetProperty(child_debug_info, "debugMetadataUrl").StdString(),
            "https://example.com/debug-info.json");

  auto events = GetProperty(child_info, "events");
  ASSERT_TRUE(events.IsArray());
  ASSERT_EQ(events.Array()->size(), 2U);
  auto tap_event = events.Array()->get(0);
  EXPECT_EQ(GetProperty(tap_event, "eventName").StdString(), "tap");
  EXPECT_EQ(GetProperty(tap_event, "type").StdString(), "bindEvent");
  EXPECT_EQ(GetProperty(tap_event, "runtime").StdString(), "BTS");
  EXPECT_EQ(GetProperty(tap_event, "function").StdString(), "onTap");
  auto focus_event = events.Array()->get(1);
  EXPECT_EQ(GetProperty(focus_event, "eventName").StdString(), "focus");
  EXPECT_EQ(GetProperty(focus_event, "type").StdString(), "catchEvent");
  EXPECT_EQ(GetProperty(focus_event, "runtime").StdString(), "MTS");
  EXPECT_TRUE(GetProperty(focus_event, "function").IsNil());
}

TEST_F(ElementTreeSerializerTest,
       ToLepusValueUsesEntryDebugMetadataForComponents) {
  auto component_config = std::make_shared<PageConfig>();
  component_config->SetDebugMetadataUrl("https://example.com/component.json");
  auto component_entry = std::make_shared<TemplateEntry>();
  component_entry->template_bundle().page_configs_ = component_config;
  tasm_->template_entries_.insert_or_assign("component_entry", component_entry);

  auto root = manager_->CreateFiberPage("0", 10);
  auto component = manager_->CreateFiberComponent(
      "1", 0, "component_entry", "TestComp", "/components/TestComp");
  component->parent_component_element_ = root.get();
  root->InsertNode(component);
  auto child = manager_->CreateFiberNode("view");
  child->parent_component_element_ = component.get();
  component->InsertNode(child);

  auto root_info = ElementTreeSerializer::ToLepusValue(root.get());
  auto root_debug_info = GetProperty(root_info, "debugInfo");
  EXPECT_EQ(GetProperty(root_debug_info, "debugMetadataUrl").StdString(),
            "https://example.com/debug-info.json");

  auto component_info = GetProperty(root_info, "children").Array()->get(0);
  auto component_debug_info = GetProperty(component_info, "debugInfo");
  EXPECT_EQ(GetProperty(component_debug_info, "debugMetadataUrl").StdString(),
            "https://example.com/component.json");

  auto child_info = GetProperty(component_info, "children").Array()->get(0);
  auto child_debug_info = GetProperty(child_info, "debugInfo");
  EXPECT_EQ(GetProperty(child_debug_info, "debugMetadataUrl").StdString(),
            "https://example.com/component.json");
}

TEST_F(ElementTreeSerializerTest,
       ToLepusValueFallsBackToParentEntryDebugMetadataForComponents) {
  auto parent_config = std::make_shared<PageConfig>();
  parent_config->SetDebugMetadataUrl("https://example.com/parent.json");
  auto parent_entry = std::make_shared<TemplateEntry>();
  parent_entry->template_bundle().page_configs_ = parent_config;
  tasm_->template_entries_.insert_or_assign("parent_entry", parent_entry);

  auto root = manager_->CreateFiberPage("0", 10);
  auto parent = manager_->CreateFiberComponent(
      "1", 0, "parent_entry", "ParentComp", "/components/ParentComp");
  parent->parent_component_element_ = root.get();
  root->InsertNode(parent);
  auto child_component = manager_->CreateFiberComponent(
      "2", 0, "", "ChildComp", "/components/ChildComp");
  child_component->parent_component_element_ = parent.get();
  parent->InsertNode(child_component);
  auto grandchild_component = manager_->CreateFiberComponent(
      "3", 0, "", "GrandchildComp", "/components/GrandchildComp");
  grandchild_component->parent_component_element_ = child_component.get();
  child_component->InsertNode(grandchild_component);

  auto root_info = ElementTreeSerializer::ToLepusValue(root.get());
  auto parent_info = GetProperty(root_info, "children").Array()->get(0);
  auto parent_debug_info = GetProperty(parent_info, "debugInfo");
  EXPECT_EQ(GetProperty(parent_debug_info, "debugMetadataUrl").StdString(),
            "https://example.com/parent.json");

  auto child_component_info =
      GetProperty(parent_info, "children").Array()->get(0);
  auto child_component_debug_info =
      GetProperty(child_component_info, "debugInfo");
  EXPECT_EQ(
      GetProperty(child_component_debug_info, "debugMetadataUrl").StdString(),
      "https://example.com/parent.json");

  auto grandchild_component_info =
      GetProperty(child_component_info, "children").Array()->get(0);
  auto grandchild_component_debug_info =
      GetProperty(grandchild_component_info, "debugInfo");
  EXPECT_EQ(GetProperty(grandchild_component_debug_info, "debugMetadataUrl")
                .StdString(),
            "https://example.com/parent.json");
}

TEST_F(ElementTreeSerializerTest,
       ToLepusValueUsesElementEntryDebugMetadataAndFallsBackToParent) {
  auto parent_config = std::make_shared<PageConfig>();
  parent_config->SetDebugMetadataUrl("https://example.com/parent-entry.json");
  auto parent_entry = std::make_shared<TemplateEntry>();
  parent_entry->template_bundle().page_configs_ = parent_config;
  tasm_->template_entries_.insert_or_assign("parent_entry", parent_entry);

  auto child_config = std::make_shared<PageConfig>();
  child_config->SetDebugMetadataUrl("https://example.com/child-entry.json");
  auto child_entry = std::make_shared<TemplateEntry>();
  child_entry->template_bundle().page_configs_ = child_config;
  tasm_->template_entries_.insert_or_assign("child_entry", child_entry);

  auto root = manager_->CreateFiberPage("0", 10);
  auto parent = manager_->CreateFiberNode("view");
  parent->set_entry_name(base::String("parent_entry"));
  root->InsertNode(parent);
  auto child = manager_->CreateFiberNode("text");
  parent->InsertNode(child);
  auto override_child = manager_->CreateFiberNode("image");
  override_child->set_entry_name(base::String("child_entry"));
  parent->InsertNode(override_child);

  auto root_info = ElementTreeSerializer::ToLepusValue(root.get());
  auto parent_info = GetProperty(root_info, "children").Array()->get(0);
  auto parent_debug_info = GetProperty(parent_info, "debugInfo");
  EXPECT_EQ(GetProperty(parent_debug_info, "debugMetadataUrl").StdString(),
            "https://example.com/parent-entry.json");

  auto child_info = GetProperty(parent_info, "children").Array()->get(0);
  auto child_debug_info = GetProperty(child_info, "debugInfo");
  EXPECT_EQ(GetProperty(child_debug_info, "debugMetadataUrl").StdString(),
            "https://example.com/parent-entry.json");

  auto override_child_info =
      GetProperty(parent_info, "children").Array()->get(1);
  auto override_child_debug_info =
      GetProperty(override_child_info, "debugInfo");
  EXPECT_EQ(
      GetProperty(override_child_debug_info, "debugMetadataUrl").StdString(),
      "https://example.com/child-entry.json");
}

TEST_F(ElementTreeSerializerTest, ToLepusValueKeepsChildrenOrder) {
  auto root = manager_->CreateFiberPage("0", 10);
  auto first = manager_->CreateFiberNode("view");
  first->SetIdSelector("first");
  auto second = manager_->CreateFiberNode("image");
  second->SetIdSelector("second");
  root->InsertNode(first);
  root->InsertNode(second);

  auto children =
      GetProperty(ElementTreeSerializer::ToLepusValue(root.get()), "children");

  ASSERT_TRUE(children.IsArray());
  ASSERT_EQ(children.Array()->size(), 2U);
  EXPECT_EQ(
      GetProperty(GetProperty(children.Array()->get(0), "attributes"), "id")
          .StdString(),
      "first");
  EXPECT_EQ(
      GetProperty(GetProperty(children.Array()->get(1), "attributes"), "id")
          .StdString(),
      "second");
}

TEST_F(ElementTreeSerializerTest,
       ToLepusValueResolvesLayoutOnlyPositionFromAncestor) {
  auto root = manager_->CreateFiberPage("0", 10);
  auto layout_only = manager_->CreateFiberView();
  layout_only->parent_component_element_ = root.get();
  layout_only->computed_css_style()->SetOverflowDefaultVisible(true);
  root->InsertNode(layout_only);
  root->FlushActionsAsRoot();
  ASSERT_TRUE(layout_only->IsLayoutOnly());
  ASSERT_FALSE(layout_only->HasUIPrimitive());

  SetLayoutObjectFrame(root.get(), 0.f, 0.f, 300.f, 400.f);
  SetLayoutObjectFrame(layout_only.get(), 7.f, 9.f, 50.f, 60.f);
  painting_context_ptr_->SetRectToLynxView(root->impl_id(),
                                           {10.f, 20.f, 300.f, 400.f});
  painting_context_ptr_->SetRectToScreen(root->impl_id(),
                                         {100.f, 200.f, 300.f, 400.f});

  auto info = ElementTreeSerializer::ToLepusValue(layout_only.get());
  auto position = GetProperty(info, "position");

  ExpectRect(GetProperty(position, "frameInRoot"), 17.f, 29.f, 50.f, 60.f);
  ExpectRect(GetProperty(position, "frameInScreen"), 107.f, 209.f, 50.f, 60.f);
}

TEST_F(ElementTreeSerializerTest,
       ToLepusValueResolvesVirtualPositionFromAncestor) {
  auto root = manager_->CreateFiberPage("0", 10);
  auto virtual_node = manager_->CreateFiberNode("inline-text");
  virtual_node->is_virtual_ = true;
  root->InsertNode(virtual_node);

  painting_context_ptr_->SetRectToLynxView(root->impl_id(),
                                           {10.f, 20.f, 300.f, 400.f});
  painting_context_ptr_->SetRectToScreen(root->impl_id(),
                                         {100.f, 200.f, 300.f, 400.f});

  auto info = ElementTreeSerializer::ToLepusValue(virtual_node.get());
  auto position = GetProperty(info, "position");

  ExpectRect(GetProperty(position, "frameInRoot"), 10.f, 20.f, 300.f, 400.f);
  ExpectRect(GetProperty(position, "frameInScreen"), 100.f, 200.f, 300.f,
             400.f);
}

TEST_F(ElementTreeSerializerTest,
       ToLepusValueAllowsNegativePositionOriginsAndSkipsInvalidRects) {
  auto root = manager_->CreateFiberPage("0", 10);
  auto child = manager_->CreateFiberNode("view");
  root->InsertNode(child);

  painting_context_ptr_->SetRectToLynxView(root->impl_id(), {1.f, 2.f, 3.f});
  painting_context_ptr_->SetRectToScreen(root->impl_id(),
                                         {-1.f, 2.f, 3.f, 4.f});
  painting_context_ptr_->SetRectToLynxView(child->impl_id(),
                                           {1.f, -2.f, 3.f, 4.f});
  painting_context_ptr_->SetRectToScreen(child->impl_id(),
                                         {1.f, 2.f, -3.f, 4.f});

  auto root_position =
      GetProperty(ElementTreeSerializer::ToLepusValue(root.get()), "position");
  EXPECT_TRUE(GetProperty(root_position, "frameInRoot").IsNil());
  ExpectRect(GetProperty(root_position, "frameInScreen"), -1.f, 2.f, 3.f, 4.f);

  auto child_info =
      GetProperty(ElementTreeSerializer::ToLepusValue(root.get()), "children")
          .Array()
          ->get(0);
  auto child_position = GetProperty(child_info, "position");
  ExpectRect(GetProperty(child_position, "frameInRoot"), 1.f, -2.f, 3.f, 4.f);
  EXPECT_TRUE(GetProperty(child_position, "frameInScreen").IsNil());
}

TEST_F(ElementTreeSerializerTest, DefaultGetRectToScreenWritesInvalidRect) {
  MockPaintingContext painting_context;
  float rect[4] = {1.f, 2.f, 3.f, 4.f};

  painting_context.GetRectToScreen(1, rect);

  EXPECT_EQ(rect[0], 0.f);
  EXPECT_EQ(rect[1], 0.f);
  EXPECT_EQ(rect[2], -1.f);
  EXPECT_EQ(rect[3], -1.f);
}

TEST_F(ElementTreeSerializerTest, ToLepusValueReturnsNilForNullElement) {
  EXPECT_TRUE(ElementTreeSerializer::ToLepusValue(nullptr).IsNil());
  EXPECT_TRUE(ElementTreeSerializer::ToJSONString(nullptr).empty());
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
