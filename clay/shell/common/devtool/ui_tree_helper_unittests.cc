// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>

#include "clay/shell/common/devtool/ui_tree_helper.h"
#include "clay/ui/component/view.h"
#include "clay/ui/component/view_context.h"
#include "clay/ui/rendering/render_object.h"
#include "clay/ui/testing/ui_test.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/rapidjson/document.h"

namespace lynx::tasm::ui_tree {

class UITreeHelperTest : public clay::UITest {
 protected:
  void UISetUp() override {
    view_context_ = std::make_unique<clay::ViewContext>(page_.get(), nullptr);
    ASSERT_TRUE(view_context_->CreateView(kRootId, "page"));
    ASSERT_TRUE(view_context_->CreateView(kParentId, "view"));
    ASSERT_TRUE(view_context_->CreateView(kChildId, "text"));
    view_context_->AddView(kParentId, kRootId, 0);
    view_context_->AddView(kChildId, kParentId, 0);
    view_context_->SetBounds(kRootId, 0, 0, 400, 800);
    view_context_->SetBounds(kParentId, 10, 20, 100, 50);
    view_context_->SetBounds(kChildId, 5, 6, 70, 20);
  }

  void UITearDown() override {
    view_context_->ResetPageView();
    view_context_.reset();
  }

  static constexpr int kRootId = 1;
  static constexpr int kParentId = 2;
  static constexpr int kChildId = 3;
  std::unique_ptr<clay::ViewContext> view_context_;
};

TEST_F_UI(UITreeHelperTest, SerializesUITree) {
  rapidjson::Document tree;
  tree.Parse(GetLynxUITree(view_context_.get()).c_str());

  ASSERT_FALSE(tree.HasParseError());
  ASSERT_TRUE(tree.IsObject());
  EXPECT_STREQ(tree["name"].GetString(), "page");
  EXPECT_EQ(tree["id"].GetInt(), kRootId);
  ASSERT_EQ(tree["frame"].Size(), 4u);
  EXPECT_DOUBLE_EQ(tree["frame"][2].GetDouble(), 400);
  ASSERT_EQ(tree["children"].Size(), 1u);
  const auto& parent = tree["children"][0];
  EXPECT_STREQ(parent["name"].GetString(), "view");
  EXPECT_EQ(parent["id"].GetInt(), kParentId);
  ASSERT_EQ(parent["children"].Size(), 1u);
  EXPECT_EQ(parent["children"][0]["id"].GetInt(), kChildId);
}

TEST_F_UI(UITreeHelperTest, FlattensAnonymousViews) {
  auto* parent = view_context_->GetViewById(kParentId);
  auto* child = view_context_->GetViewById(kChildId);
  ASSERT_NE(parent, nullptr);
  ASSERT_NE(child, nullptr);

  parent->RemoveChildTemporarily(child);
  auto* anonymous = new clay::View(-1, page_.get());
  parent->AddChild(anonymous);
  anonymous->AddChild(child);

  rapidjson::Document tree;
  tree.Parse(GetLynxUITree(view_context_.get()).c_str());

  ASSERT_FALSE(tree.HasParseError());
  const auto& children = tree["children"][0]["children"];
  ASSERT_EQ(children.Size(), 1u);
  EXPECT_EQ(children[0]["id"].GetInt(), kChildId);
  EXPECT_STREQ(children[0]["name"].GetString(), "text");
}

TEST_F_UI(UITreeHelperTest, ReturnsNodeInfoAndEditsStyles) {
  rapidjson::Document info;
  info.Parse(GetUINodeInfo(view_context_.get(), kParentId).c_str());

  ASSERT_FALSE(info.HasParseError());
  EXPECT_EQ(info["id"].GetInt(), kParentId);
  EXPECT_STREQ(info["ui"]["name"].GetString(), "view");
  ASSERT_EQ(info["editableProps"]["frame"].Size(), 4u);
  EXPECT_DOUBLE_EQ(info["editableProps"]["frame"][0].GetDouble(), 10);

  EXPECT_EQ(SetUIStyle(view_context_.get(), kParentId, "margin", "1, 2, 3, 4"),
            0);
  auto* view = view_context_->GetViewById(kParentId);
  ASSERT_NE(view, nullptr);
  EXPECT_FLOAT_EQ(view->MarginTop(), 1);
  EXPECT_FLOAT_EQ(view->MarginRight(), 2);
  EXPECT_FLOAT_EQ(view->MarginBottom(), 3);
  EXPECT_FLOAT_EQ(view->MarginLeft(), 4);
  EXPECT_FLOAT_EQ(view->Left(), 14);
  EXPECT_FLOAT_EQ(view->Top(), 21);

  EXPECT_EQ(SetUIStyle(view_context_.get(), kParentId, "visible", "false"), 0);
  EXPECT_FALSE(view->Visible());
  EXPECT_EQ(SetUIStyle(view_context_.get(), kParentId, "visible", "invalid"),
            -1);
  EXPECT_EQ(SetUIStyle(view_context_.get(), kParentId, "frame", "1,2,3"), -1);
  EXPECT_EQ(SetUIStyle(view_context_.get(), 999, "frame", "1,2,3,4"), -1);
}

TEST_F_UI(UITreeHelperTest, EditsFrameBorderAndColors) {
  // cspell:ignore xddaabbccu ggbbccdd
  auto* view = view_context_->GetViewById(kParentId);
  ASSERT_NE(view, nullptr);

  EXPECT_EQ(
      SetUIStyle(view_context_.get(), kParentId, "frame", "30, 40 120, 60"), 0);
  EXPECT_FLOAT_EQ(view->Left(), 30);
  EXPECT_FLOAT_EQ(view->Top(), 40);
  EXPECT_FLOAT_EQ(view->Width(), 120);
  EXPECT_FLOAT_EQ(view->Height(), 60);

  EXPECT_EQ(
      SetUIStyle(view_context_.get(), kParentId, "frame", "-2, 2, 3e1, 4"), 0);
  EXPECT_FLOAT_EQ(view->Left(), -2);
  EXPECT_FLOAT_EQ(view->Top(), 2);
  EXPECT_FLOAT_EQ(view->Width(), 30);
  EXPECT_FLOAT_EQ(view->Height(), 4);

  EXPECT_EQ(SetUIStyle(view_context_.get(), kParentId, "border", "1 2 3 4"), 0);
  EXPECT_FLOAT_EQ(view->BorderTop(), 1);
  EXPECT_FLOAT_EQ(view->BorderRight(), 2);
  EXPECT_FLOAT_EQ(view->BorderBottom(), 3);
  EXPECT_FLOAT_EQ(view->BorderLeft(), 4);

  EXPECT_EQ(SetUIStyle(view_context_.get(), kParentId, "background-color",
                       " #11223344 "),
            0);
  ASSERT_TRUE(view->render_object()->HasBackground());
  EXPECT_EQ(view->render_object()->Background().background_color.Value(),
            0x44112233u);

  EXPECT_EQ(
      SetUIStyle(view_context_.get(), kParentId, "border-color", "#aabbccdd"),
      0);
  ASSERT_TRUE(view->render_object()->HasBorder());
  const auto& border = view->render_object()->Border();
  EXPECT_EQ(border.color_top_, 0xddaabbccu);
  EXPECT_EQ(border.color_right_, 0xddaabbccu);
  EXPECT_EQ(border.color_bottom_, 0xddaabbccu);
  EXPECT_EQ(border.color_left_, 0xddaabbccu);

  EXPECT_EQ(SetUIStyle(view_context_.get(), kParentId, "frame", "1 2 3 4 5"),
            -1);
  EXPECT_EQ(SetUIStyle(view_context_.get(), kParentId, "frame", "1 2x 3 4"),
            -1);
  EXPECT_EQ(SetUIStyle(view_context_.get(), kParentId, "frame", "1 2 3 inf"),
            -1);
  EXPECT_EQ(
      SetUIStyle(view_context_.get(), kParentId, "background-color", "#112233"),
      -1);
  EXPECT_EQ(
      SetUIStyle(view_context_.get(), kParentId, "border-color", "#ggbbccdd"),
      -1);
  EXPECT_EQ(SetUIStyle(view_context_.get(), kParentId, "border-color",
                       " #aa bb cc dd "),
            0);
  EXPECT_EQ(SetUIStyle(view_context_.get(), kParentId, "unknown", "value"), -1);
}

TEST_F_UI(UITreeHelperTest, RejectsUnavailableViews) {
  EXPECT_TRUE(GetLynxUITree(nullptr).empty());
  EXPECT_TRUE(GetUINodeInfo(nullptr, kParentId).empty());
  EXPECT_TRUE(GetUINodeInfo(view_context_.get(), 999).empty());
  EXPECT_EQ(SetUIStyle(nullptr, kParentId, "visible", "true"), -1);
}

}  // namespace lynx::tasm::ui_tree
