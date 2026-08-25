// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/modifier_element.h"

#include <vector>

#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/compose_element_handle.h"
#include "core/renderer/dom/fiber/compose_modifier_applicator.h"
#include "core/renderer/dom/fiber/image_element.h"
#include "core/renderer/dom/fiber/text_element.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/dom/testing/fiber_element_test.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

namespace {

std::vector<ModifierElement*> CollectModifierFrames(Element* root,
                                                    Element* owner) {
  std::vector<ModifierElement*> frames;
  for (auto* element = root; element != owner;) {
    if (element == nullptr || !element->is_modifier() ||
        element->GetChildCount() != 1) {
      return {};
    }
    frames.push_back(static_cast<ModifierElement*>(element));
    element = element->GetChildAt(0);
  }
  return frames;
}

}  // namespace

class ModifierElementTest : public FiberElementTest {};

TEST_P(ModifierElementTest, UsesDefaultElementBehaviorWithViewTag) {
  auto element = manager->CreateFiberModifierElement();

  EXPECT_TRUE(element->GetTag().IsEqual(kElementViewTag));
  EXPECT_FALSE(element->is_view());
  EXPECT_FALSE(element->is_wrapper());

  const auto& styles = element->GetCurrentRawInlineStyles();
  ASSERT_TRUE(styles.has_value());
  EXPECT_EQ(styles->at(kPropertyIDDisplay), lepus::Value("box"));
  EXPECT_EQ(styles->at(kPropertyIDBoxSizing), lepus::Value("border-box"));
}

TEST_P(ModifierElementTest, MaterializesOnlyPaddingAsDetachedFrame) {
  auto padding = lepus::Dictionary::Create();
  padding->SetValue("op", lepus::Value(7));
  padding->SetValue("start", lepus::Value(1.0));
  padding->SetValue("top", lepus::Value(2.0));
  padding->SetValue("end", lepus::Value(3.0));
  padding->SetValue("bottom", lepus::Value(4.0));

  auto size = lepus::Dictionary::Create();
  size->SetValue("op", lepus::Value(8));
  size->SetValue("specifiedAxes", lepus::Value(3));
  size->SetValue("width", lepus::Value(100.0));
  size->SetValue("height", lepus::Value(50.0));
  size->SetValue("previous", lepus::Value(padding));

  auto owner = manager->CreateFiberView();
  auto handle = fml::AdoptRef<ComposeElementHandle>(new ComposeElementHandle(
      ComposeElementKind::kView, fml::RefPtr<Element>(owner)));
  auto result = ComposeModifierApplicator::Apply(
      handle.get(), lepus::Value(size), /*registration_context=*/nullptr);
  ASSERT_TRUE(result.success) << result.error_message;
  auto mount_root = handle->mount_root();
  ASSERT_NE(mount_root, nullptr);
  EXPECT_NE(mount_root.get(), owner.get());
  const auto frames = CollectModifierFrames(mount_root.get(), owner.get());
  ASSERT_EQ(frames.size(), 1u);
  EXPECT_EQ(frames[0]->parent(), nullptr);
  EXPECT_EQ(mount_root.get(), frames[0]);
  ASSERT_EQ(frames[0]->GetChildCount(), 1u);
  EXPECT_EQ(frames[0]->GetChildAt(0), owner.get());
  EXPECT_EQ(owner->parent(), frames[0]);

  const auto& owner_styles = owner->GetCurrentRawInlineStyles();
  ASSERT_TRUE(owner_styles.has_value());
  EXPECT_EQ(owner_styles->at(kPropertyIDWidth), lepus::Value("100px"));
  EXPECT_EQ(owner_styles->at(kPropertyIDHeight), lepus::Value("50px"));
  EXPECT_EQ(owner_styles->at(kPropertyIDBoxSizing), lepus::Value("border-box"));
}

TEST_P(ModifierElementTest, KeepsPhysicalRootsPrivateToEachOwner) {
  auto padding = lepus::Dictionary::Create();
  padding->SetValue("op", lepus::Value(7));
  padding->SetValue("start", lepus::Value(1.0));
  padding->SetValue("top", lepus::Value(1.0));
  padding->SetValue("end", lepus::Value(1.0));
  padding->SetValue("bottom", lepus::Value(1.0));

  auto first_owner = manager->CreateFiberView();
  auto first_handle =
      fml::AdoptRef<ComposeElementHandle>(new ComposeElementHandle(
          ComposeElementKind::kView, fml::RefPtr<Element>(first_owner)));
  auto first_result = ComposeModifierApplicator::Apply(
      first_handle.get(), lepus::Value(padding),
      /*registration_context=*/nullptr);
  ASSERT_TRUE(first_result.success) << first_result.error_message;

  auto second_owner = manager->CreateFiberView();
  auto second_handle =
      fml::AdoptRef<ComposeElementHandle>(new ComposeElementHandle(
          ComposeElementKind::kView, fml::RefPtr<Element>(second_owner)));
  auto second_result = ComposeModifierApplicator::Apply(
      second_handle.get(), lepus::Value(padding),
      /*registration_context=*/nullptr);
  ASSERT_TRUE(second_result.success) << second_result.error_message;

  auto first_root = first_handle->mount_root();
  auto second_root = second_handle->mount_root();
  EXPECT_NE(first_root, second_root);
  EXPECT_EQ(CollectModifierFrames(first_root.get(), first_owner.get()).size(),
            1u);
  EXPECT_EQ(CollectModifierFrames(second_root.get(), second_owner.get()).size(),
            1u);
}

TEST_P(ModifierElementTest, AcceptsComposeModifierSingletonAsChainEnd) {
  auto compose_modifier = lepus::Dictionary::Create();
  auto size = lepus::Dictionary::Create();
  size->SetValue("op", lepus::Value(8));
  size->SetValue("specifiedAxes", lepus::Value(1));
  size->SetValue("width", lepus::Value(40.0));
  size->SetValue("height", lepus::Value(0.0));
  size->SetValue("previous", lepus::Value(compose_modifier));

  auto owner = manager->CreateFiberView();
  auto handle = fml::AdoptRef<ComposeElementHandle>(new ComposeElementHandle(
      ComposeElementKind::kView, fml::RefPtr<Element>(owner)));
  auto result = ComposeModifierApplicator::Apply(
      handle.get(), lepus::Value(size), /*registration_context=*/nullptr);

  ASSERT_TRUE(result.success) << result.error_message;
  EXPECT_EQ(handle->mount_root(), owner);
  ASSERT_TRUE(owner->GetCurrentRawInlineStyles().has_value());
  EXPECT_EQ(owner->GetCurrentRawInlineStyles()->at(kPropertyIDWidth),
            lepus::Value("40px"));
}

TEST_P(ModifierElementTest, SupportsEveryComposeOwnerElementKind) {
  auto padding = lepus::Dictionary::Create();
  padding->SetValue("op", lepus::Value(7));
  padding->SetValue("start", lepus::Value(2.0));
  padding->SetValue("top", lepus::Value(2.0));
  padding->SetValue("end", lepus::Value(2.0));
  padding->SetValue("bottom", lepus::Value(2.0));

  std::vector<std::pair<ComposeElementKind, fml::RefPtr<Element>>> owners = {
      {ComposeElementKind::kView, manager->CreateFiberView()},
      {ComposeElementKind::kText, manager->CreateFiberText("text")},
      {ComposeElementKind::kImage, manager->CreateFiberImage("image")}};
  for (const auto& [kind, owner] : owners) {
    auto handle = fml::AdoptRef<ComposeElementHandle>(
        new ComposeElementHandle(kind, fml::RefPtr<Element>(owner)));
    auto result = ComposeModifierApplicator::Apply(
        handle.get(), lepus::Value(padding), /*registration_context=*/nullptr);
    ASSERT_TRUE(result.success) << result.error_message;
    auto root = handle->mount_root();
    EXPECT_NE(root.get(), owner.get());
    EXPECT_EQ(CollectModifierFrames(root.get(), owner.get()).size(), 1u);
  }
}

TEST_P(ModifierElementTest,
       HandleRetainsDetachedChainAndBecomesStaleOnTeardown) {
  auto padding = lepus::Dictionary::Create();
  padding->SetValue("op", lepus::Value(7));
  padding->SetValue("start", lepus::Value(3.0));
  padding->SetValue("top", lepus::Value(3.0));
  padding->SetValue("end", lepus::Value(3.0));
  padding->SetValue("bottom", lepus::Value(3.0));

  auto owner = manager->CreateFiberView();
  auto handle = fml::AdoptRef<ComposeElementHandle>(new ComposeElementHandle(
      ComposeElementKind::kView, fml::RefPtr<Element>(owner)));
  auto result = ComposeModifierApplicator::Apply(
      handle.get(), lepus::Value(padding), /*registration_context=*/nullptr);
  ASSERT_TRUE(result.success) << result.error_message;
  auto root = handle->mount_root();
  auto owner_weak = owner->WeakFromThis();
  auto root_weak = root->WeakFromThis();

  owner = nullptr;
  root = nullptr;
  // The handle, not ElementManager, retains the detached physical chain.
  EXPECT_TRUE(owner_weak);
  EXPECT_TRUE(root_weak);

  manager->node_manager()->WillDestroy();
  std::string error;
  EXPECT_FALSE(
      ComposeModifierApplicator::ValidateTopology(handle.get(), &error));
  EXPECT_TRUE(owner_weak);
  EXPECT_TRUE(root_weak);
  handle = nullptr;
  EXPECT_FALSE(owner_weak);
  EXPECT_FALSE(root_weak);
}

INSTANTIATE_TEST_SUITE_P(ModifierElementTestModule, ModifierElementTest,
                         ::testing::ValuesIn(fiber_element_generation_params));

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
