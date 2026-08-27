// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/modifier_element.h"

#include <vector>

#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/compose_element_handle.h"
#include "core/renderer/dom/fiber/compose_modifier_applicator.h"
#include "core/renderer/dom/fiber/element_utils.h"
#include "core/renderer/dom/fiber/image_element.h"
#include "core/renderer/dom/fiber/text_element.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/dom/testing/fiber_element_test.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

namespace {

fml::RefPtr<ComposeElementHandle> CreateComposeHandle(
    ElementManager* manager,
    ComposeElementKind kind = ComposeElementKind::kView) {
  return fml::AdoptRef<ComposeElementHandle>(
      new ComposeElementHandle(manager, kind));
}

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

lepus::Value MakePaddingChain(size_t frame_count, double value) {
  lepus::Value previous;
  for (size_t index = 0; index < frame_count; ++index) {
    auto background = lepus::Dictionary::Create();
    background->SetValue("op", lepus::Value(3));
    background->SetValue(
        "propertyId",
        lepus::Value(static_cast<int32_t>(kPropertyIDBackgroundColor)));
    background->SetValue("value", lepus::Value("#010101"));
    if (!previous.IsEmpty()) {
      background->SetValue("previous", previous);
    }

    auto padding = lepus::Dictionary::Create();
    padding->SetValue("op", lepus::Value(7));
    padding->SetValue("start", lepus::Value(value + index));
    padding->SetValue("top", lepus::Value(value + index));
    padding->SetValue("end", lepus::Value(value + index));
    padding->SetValue("bottom", lepus::Value(value + index));
    padding->SetValue("previous", lepus::Value(background));
    previous = lepus::Value(padding);
  }
  return previous;
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

TEST_P(ModifierElementTest, ResolvesFiberAndComposeElements) {
  auto element = manager->CreateFiberView();
  lepus::Value element_value{fml::RefPtr<Element>(element)};
  EXPECT_EQ(GetComposeContentOrFiberElementFromValue(element_value), element);
  EXPECT_EQ(GetComposeMountRootOrFiberElementFromValue(element_value), element);

  auto handle = CreateComposeHandle(manager);
  auto content = handle->content_element();
  lepus::Value handle_value{handle};
  EXPECT_EQ(GetComposeContentOrFiberElementFromValue(handle_value), content);
  EXPECT_EQ(GetComposeMountRootOrFiberElementFromValue(handle_value), content);

  auto result =
      ComposeModifierApplicator::Apply(handle.get(), MakePaddingChain(1, 1.0),
                                       /*registration_context=*/nullptr);
  ASSERT_TRUE(result.success) << result.error_message;
  EXPECT_EQ(GetComposeContentOrFiberElementFromValue(handle_value), content);
  EXPECT_EQ(GetComposeMountRootOrFiberElementFromValue(handle_value),
            handle->mount_root());
  EXPECT_NE(handle->mount_root(), content);
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

  auto handle = CreateComposeHandle(manager);
  auto owner = handle->content_element();
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

  auto first_handle = CreateComposeHandle(manager);
  auto first_owner = first_handle->content_element();
  auto first_result = ComposeModifierApplicator::Apply(
      first_handle.get(), lepus::Value(padding),
      /*registration_context=*/nullptr);
  ASSERT_TRUE(first_result.success) << first_result.error_message;

  auto second_handle = CreateComposeHandle(manager);
  auto second_owner = second_handle->content_element();
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

TEST_P(ModifierElementTest, ReparentAttachedFramesThroughTargetOwnedMove) {
  auto page = manager->CreateFiberPage("page", 0);
  manager->SetFiberPageElement(page);
  auto handle = CreateComposeHandle(manager);
  auto owner = handle->content_element();
  owner->MarkCanBeLayoutOnly(false);
  const int32_t owner_id = owner->impl_id();

  auto initial_result =
      ComposeModifierApplicator::Apply(handle.get(), MakePaddingChain(1, 4.0),
                                       /*registration_context=*/nullptr);
  ASSERT_TRUE(initial_result.success) << initial_result.error_message;
  page->InsertNode(handle->mount_root());
  page->FlushActionsAsRoot();
  platform_impl_->Flush();

  auto apply_and_flush = [&](size_t frame_count, double value) {
    const auto old_frames =
        CollectModifierFrames(handle->mount_root().get(), owner.get());
    ASSERT_FALSE(old_frames.empty());
    auto* old_source = old_frames.back();
    auto old_source_weak = old_source->WeakFromThis();
    std::vector<int32_t> old_frame_ids;
    for (auto* frame : old_frames) {
      old_frame_ids.push_back(frame->impl_id());
    }

    platform_impl_->ResetCapturedRemoveSigns();
    auto result = ComposeModifierApplicator::Apply(
        handle.get(), MakePaddingChain(frame_count, value),
        /*registration_context=*/nullptr);
    ASSERT_TRUE(result.success) << result.error_message;

    const auto new_frames =
        CollectModifierFrames(handle->mount_root().get(), owner.get());
    ASSERT_EQ(new_frames.size(), frame_count);
    auto* new_source = new_frames.back();
    EXPECT_EQ(owner->parent(), new_source);
    EXPECT_EQ(owner->render_parent(), old_source);
    EXPECT_TRUE(old_source_weak);

    page->FlushActionsAsRoot();
    EXPECT_EQ(owner->render_parent(), new_source);
    EXPECT_FALSE(old_source_weak);
    platform_impl_->Flush();

    EXPECT_EQ(owner->impl_id(), owner_id);
    EXPECT_EQ(platform_impl_->node_map_.count(owner_id), 1u);
    ASSERT_EQ(platform_impl_->node_map_.count(new_source->impl_id()), 1u);
    EXPECT_EQ(platform_impl_->node_map_.at(owner_id)->parent_->id_,
              new_source->impl_id());
    EXPECT_FALSE(platform_impl_->HasCapturedRemoveSign(owner_id));
    for (const auto old_frame_id : old_frame_ids) {
      EXPECT_EQ(platform_impl_->node_map_.count(old_frame_id), 0u);
    }
  };

  apply_and_flush(1, 8.0);
  apply_and_flush(2, 12.0);
  apply_and_flush(3, 16.0);
  apply_and_flush(2, 20.0);
  apply_and_flush(1, 24.0);
}

TEST_P(ModifierElementTest, CoalescesPendingMovesBeforeSingleFlush) {
  auto page = manager->CreateFiberPage("page", 0);
  manager->SetFiberPageElement(page);
  auto handle = CreateComposeHandle(manager);
  auto owner = handle->content_element();
  owner->MarkCanBeLayoutOnly(false);
  const int32_t owner_id = owner->impl_id();

  auto initial_result =
      ComposeModifierApplicator::Apply(handle.get(), MakePaddingChain(1, 4.0),
                                       /*registration_context=*/nullptr);
  ASSERT_TRUE(initial_result.success) << initial_result.error_message;
  page->InsertNode(handle->mount_root());
  page->FlushActionsAsRoot();
  platform_impl_->Flush();

  const auto initial_frames =
      CollectModifierFrames(handle->mount_root().get(), owner.get());
  ASSERT_EQ(initial_frames.size(), 1u);
  auto* initial_source = initial_frames.back();
  auto initial_source_weak = initial_source->WeakFromThis();
  const int32_t initial_frame_id = initial_source->impl_id();

  auto intermediate_result =
      ComposeModifierApplicator::Apply(handle.get(), MakePaddingChain(2, 8.0),
                                       /*registration_context=*/nullptr);
  ASSERT_TRUE(intermediate_result.success) << intermediate_result.error_message;
  const auto intermediate_frames =
      CollectModifierFrames(handle->mount_root().get(), owner.get());
  ASSERT_EQ(intermediate_frames.size(), 2u);
  auto* intermediate_source = intermediate_frames.back();
  auto intermediate_source_weak = intermediate_source->WeakFromThis();
  std::vector<int32_t> intermediate_frame_ids;
  for (auto* frame : intermediate_frames) {
    intermediate_frame_ids.push_back(frame->impl_id());
  }
  EXPECT_TRUE(initial_source_weak);

  auto final_result =
      ComposeModifierApplicator::Apply(handle.get(), MakePaddingChain(1, 12.0),
                                       /*registration_context=*/nullptr);
  ASSERT_TRUE(final_result.success) << final_result.error_message;
  const auto final_frames =
      CollectModifierFrames(handle->mount_root().get(), owner.get());
  ASSERT_EQ(final_frames.size(), 1u);
  auto* final_source = final_frames.back();

  EXPECT_EQ(owner->parent(), final_source);
  EXPECT_EQ(owner->render_parent(), initial_source);
  EXPECT_TRUE(initial_source_weak);

  platform_impl_->ResetCapturedRemoveSigns();
  page->FlushActionsAsRoot();
  EXPECT_EQ(owner->render_parent(), final_source);
  EXPECT_FALSE(initial_source_weak);
  EXPECT_FALSE(intermediate_source_weak);
  platform_impl_->Flush();

  EXPECT_EQ(owner->impl_id(), owner_id);
  EXPECT_EQ(platform_impl_->node_map_.count(owner_id), 1u);
  ASSERT_EQ(platform_impl_->node_map_.count(final_source->impl_id()), 1u);
  EXPECT_EQ(platform_impl_->node_map_.at(owner_id)->parent_->id_,
            final_source->impl_id());
  EXPECT_EQ(platform_impl_->node_map_.count(initial_frame_id), 0u);
  for (const auto intermediate_frame_id : intermediate_frame_ids) {
    EXPECT_EQ(platform_impl_->node_map_.count(intermediate_frame_id), 0u);
  }
  EXPECT_FALSE(platform_impl_->HasCapturedRemoveSign(owner_id));
}

TEST_P(ModifierElementTest, AcceptsComposeModifierSingletonAsChainEnd) {
  auto compose_modifier = lepus::Dictionary::Create();
  auto size = lepus::Dictionary::Create();
  size->SetValue("op", lepus::Value(8));
  size->SetValue("specifiedAxes", lepus::Value(1));
  size->SetValue("width", lepus::Value(40.0));
  size->SetValue("height", lepus::Value(0.0));
  size->SetValue("previous", lepus::Value(compose_modifier));

  auto handle = CreateComposeHandle(manager);
  auto owner = handle->content_element();
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

  const ComposeElementKind kinds[] = {ComposeElementKind::kView,
                                      ComposeElementKind::kText,
                                      ComposeElementKind::kImage};
  for (const auto kind : kinds) {
    auto handle = CreateComposeHandle(manager, kind);
    auto owner = handle->content_element();
    ASSERT_NE(owner, nullptr);
    EXPECT_EQ(handle->mount_root(), owner);
    switch (kind) {
      case ComposeElementKind::kView:
        EXPECT_TRUE(owner->is_view());
        EXPECT_TRUE(owner->GetTag().IsEqual(kElementViewTag));
        break;
      case ComposeElementKind::kText:
        EXPECT_TRUE(owner->is_text());
        EXPECT_TRUE(owner->GetTag().IsEqual(kElementTextTag));
        break;
      case ComposeElementKind::kImage:
        EXPECT_TRUE(owner->is_image());
        EXPECT_TRUE(owner->GetTag().IsEqual(kElementImageTag));
        break;
    }
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

  auto handle = CreateComposeHandle(manager);
  auto owner = handle->content_element();
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
