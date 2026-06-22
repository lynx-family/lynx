// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <mutex>
#include <optional>
#include <string>

#define private public
#define protected public

#include "core/list/decoupled_list_container_impl.h"
#include "core/renderer/dom/fiber/list_element.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/renderer/ui_component/list/list_container_impl.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

static constexpr int32_t kWidth = 1080;
static constexpr int32_t kHeight = 1920;
static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;

class ScopedExternalBoolEnv {
 public:
  ScopedExternalBoolEnv(LynxEnv::Key key, bool value) : key_(key) {
    auto& env = LynxEnv::GetInstance();
    {
      std::lock_guard<std::recursive_mutex> lock(env.external_env_mutex_);
      auto it = env.external_env_map_.find(key_);
      if (it != env.external_env_map_.end()) {
        previous_value_ = it->second;
      }
    }
    std::lock_guard<std::recursive_mutex> lock(env.external_env_mutex_);
    env.external_env_map_[key_] =
        value ? LynxEnv::kLocalEnvValueTrue : LynxEnv::kLocalEnvValueFalse;
  }

  ~ScopedExternalBoolEnv() {
    auto& env = LynxEnv::GetInstance();
    std::lock_guard<std::recursive_mutex> lock(env.external_env_mutex_);
    if (previous_value_) {
      env.external_env_map_[key_] = *previous_value_;
    } else {
      env.external_env_map_.erase(key_);
    }
  }

 private:
  LynxEnv::Key key_;
  std::optional<std::string> previous_value_;
};

class ListElementTest : public ::testing::Test {
 public:
  ListElementTest() = default;
  ~ListElementTest() override = default;

  fml::RefPtr<PageElement> page_;
  fml::RefPtr<ListElement> list_element_;

  std::shared_ptr<::testing::NiceMock<test::MockTasmDelegate>> tasm_mediator_;
  std::shared_ptr<lynx::tasm::TemplateAssembler> tasm_;
  std::shared_ptr<PageConfig> page_config_;
  ElementManager* manager_ = nullptr;

  void SetUp() override {
    LynxEnvConfig lynx_env_config(kWidth, kHeight, kDefaultLayoutsUnitPerPx,
                                  kDefaultPhysicalPixelsPerLayoutUnit);
    tasm_mediator_ = std::make_shared<
        ::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>();
    auto manager = std::make_unique<lynx::tasm::ElementManager>(
        std::make_unique<MockPaintingContext>(), tasm_mediator_.get(),
        lynx_env_config);
    page_config_ = std::make_shared<PageConfig>();
    page_config_->SetEnableFiberArch(true);
    manager->SetConfig(page_config_);
    manager_ = manager.get();
    tasm_ = std::make_shared<lynx::tasm::TemplateAssembler>(
        *tasm_mediator_.get(), std::move(manager), tasm_mediator_.get(), 0);

    page_ = fml::AdoptRef<PageElement>(new PageElement(manager_, "page", 0));
    list_element_ = fml::AdoptRef<ListElement>(new ListElement(
        manager_, "list", lepus::Value(), lepus::Value(), lepus::Value()));
    page_->InsertNode(list_element_);
    list_element_->set_tasm(tasm_.get());
  }

  fml::RefPtr<ListElement> CreateNewStylingNativeList(bool decoupled) {
    page_config_->SetEnableNativeList(TernaryBool::TRUE_VALUE);
    page_config_->SetEnableNewStylingPipeline(true);
    manager_->SetConfig(page_config_);

    page_ = manager_->CreateFiberPage("page", 11);
    manager_->SetFiberPageElement(page_);

    auto list = manager_->CreateFiberList(tasm_.get(), "list", lepus::Value(),
                                          lepus::Value(), lepus::Value());
    list->SetAttribute(lynx::list::kPropEnableDecoupledList,
                       lepus::Value(decoupled));
    list->SetRawInlineStyles(
        "list-main-axis-gap: 12px; list-cross-axis-gap: 4px;");
    page_->InsertNode(list);
    page_->FlushActionsAsRoot();
    return list;
  }

  std::pair<float, float> ListAxisGaps(ListElement* list) {
    if (list->UseInternalList()) {
      auto* container = static_cast<ListContainerImpl*>(
          list->list_container_delegate_internal_.get());
      auto* layout_manager = container->list_layout_manager();
      return {layout_manager->main_axis_gap(),
              layout_manager->cross_axis_gap()};
    }

    auto* container = static_cast<lynx::list::ListContainerImpl*>(
        list->list_mediator_->list_container_delegate_.get());
    auto* layout_manager = container->list_layout_manager();
    return {layout_manager->main_axis_gap(), layout_manager->cross_axis_gap()};
  }

  std::pair<bool, bool> ListScrollDirections(ListElement* list) {
    if (list->UseInternalList()) {
      auto* container = static_cast<ListContainerImpl*>(
          list->list_container_delegate_internal_.get());
      auto* layout_manager = container->list_layout_manager();
      return {layout_manager->CanScrollHorizontally(),
              layout_manager->CanScrollVertically()};
    }

    auto* container = static_cast<lynx::list::ListContainerImpl*>(
        list->list_mediator_->list_container_delegate_.get());
    auto* layout_manager = container->list_layout_manager();
    return {layout_manager->CanScrollHorizontally(),
            layout_manager->CanScrollVertically()};
  }

  int ListSpanCount(ListElement* list) {
    if (list->UseInternalList()) {
      auto* container = static_cast<ListContainerImpl*>(
          list->list_container_delegate_internal_.get());
      return container->list_layout_manager()->span_count();
    }

    auto* container = static_cast<lynx::list::ListContainerImpl*>(
        list->list_mediator_->list_container_delegate_.get());
    return container->list_layout_manager()->span_count();
  }

  bool ResolvedStyleHasValue(ListElement* list, CSSPropertyID id,
                             const CSSValue& value) {
    const auto& resolved_values =
        list->computed_css_style()->GetResolvedValues();
    auto it = resolved_values.find(id);
    return it != resolved_values.end() && it->second == value;
  }
};

class SSRListElementTest : public ListElementTest {};

TEST_F(ListElementTest, ResolveEnableNativeListUsesShellFirst) {
  // Shell enableNativeList has the highest priority for the native-list
  // decision, while custom-list-name still controls the platform node tag.
  ScopedExternalBoolEnv enable_native_list_env(LynxEnv::Key::ENABLE_NATIVE_LIST,
                                               false);
  manager_->SetEnableNativeListFromShell(true);
  page_config_->SetEnableNativeList(TernaryBool::FALSE_VALUE);
  list_element_->SetAttribute(base::String(list::kCustomLisName),
                              lepus::Value("my-list"));

  list_element_->ResolveEnableNativeList();
  list_element_->ResolvePlatformNodeTag();

  EXPECT_TRUE(list_element_->DisableListPlatformImplementation());
  EXPECT_FALSE(list_element_->enable_native_list_only_from_env_);
  EXPECT_EQ(list_element_->GetPlatformNodeTag().str(), "my-list");
}

TEST_F(ListElementTest, ResolveEnableNativeListUsesListContainerName) {
  // custom-list-name=list-container should enable native list through the
  // property branch, even when lower-priority switches are disabled.
  ScopedExternalBoolEnv enable_native_list_env(LynxEnv::Key::ENABLE_NATIVE_LIST,
                                               false);
  page_config_->SetEnableNativeList(TernaryBool::FALSE_VALUE);
  list_element_->SetAttribute(base::String(list::kCustomLisName),
                              lepus::Value(list::kListContainer));

  list_element_->ResolveEnableNativeList();
  list_element_->ResolvePlatformNodeTag();

  EXPECT_TRUE(list_element_->DisableListPlatformImplementation());
  EXPECT_FALSE(list_element_->enable_native_list_only_from_env_);
  EXPECT_EQ(list_element_->GetPlatformNodeTag().str(), list::kListContainer);
}

TEST_F(ListElementTest, ResolveEnableNativeListUsesCustomListNameFirst) {
  // custom-list-name has higher priority than config and env. A non
  // list-container value should keep the platform implementation even when the
  // lower-priority switches request native list.
  ScopedExternalBoolEnv enable_native_list_env(LynxEnv::Key::ENABLE_NATIVE_LIST,
                                               true);
  page_config_->SetEnableNativeList(TernaryBool::TRUE_VALUE);
  list_element_->SetAttribute(base::String(list::kCustomLisName),
                              lepus::Value("my-list"));

  list_element_->ResolveEnableNativeList();
  list_element_->ResolvePlatformNodeTag();

  EXPECT_FALSE(list_element_->DisableListPlatformImplementation());
  EXPECT_FALSE(list_element_->enable_native_list_only_from_env_);
  EXPECT_EQ(list_element_->GetPlatformNodeTag().str(), "my-list");
}

TEST_F(ListElementTest, ResolveEnableNativeListUsesPageConfigTrue) {
  // A true value from PageConfig should enable native list through the config
  // branch and must not mark the decision as env-only.
  ScopedExternalBoolEnv enable_native_list_env(LynxEnv::Key::ENABLE_NATIVE_LIST,
                                               false);
  page_config_->SetEnableNativeList(TernaryBool::TRUE_VALUE);

  list_element_->ResolveEnableNativeList();
  list_element_->ResolvePlatformNodeTag();

  EXPECT_TRUE(list_element_->DisableListPlatformImplementation());
  EXPECT_FALSE(list_element_->enable_native_list_only_from_env_);
  EXPECT_EQ(list_element_->GetPlatformNodeTag().str(), list::kListContainer);
}

TEST_F(ListElementTest, ResolveEnableNativeListUsesEnvWhenPageConfigUndefined) {
  // When PageConfig keeps enableNativeList undefined, enable_native_list from
  // env should work as a fallback and mark this native-list choice as env-only.
  ScopedExternalBoolEnv enable_native_list_env(LynxEnv::Key::ENABLE_NATIVE_LIST,
                                               true);
  page_config_->SetEnableNativeList(TernaryBool::UNDEFINE_VALUE);

  list_element_->ResolveEnableNativeList();
  list_element_->ResolvePlatformNodeTag();

  EXPECT_TRUE(list_element_->DisableListPlatformImplementation());
  EXPECT_TRUE(list_element_->enable_native_list_only_from_env_);
  EXPECT_EQ(list_element_->GetPlatformNodeTag().str(), "list-container");
}

TEST_F(ListElementTest, ResolveEnableNativeListPageConfigFalseOverridesEnv) {
  // An explicit false from PageConfig should override env and avoid marking the
  // native-list choice as env-only.
  ScopedExternalBoolEnv enable_native_list_env(LynxEnv::Key::ENABLE_NATIVE_LIST,
                                               true);
  page_config_->SetEnableNativeList(TernaryBool::FALSE_VALUE);

  list_element_->ResolveEnableNativeList();
  list_element_->ResolvePlatformNodeTag();

  EXPECT_FALSE(list_element_->DisableListPlatformImplementation());
  EXPECT_FALSE(list_element_->enable_native_list_only_from_env_);
  EXPECT_EQ(list_element_->GetPlatformNodeTag().str(), list::kList);
}

TEST_F(ListElementTest, ResolveEnableNativeListUsesEnvFalseFallback) {
  // When PageConfig is undefined and env is false, ResolveEnableNativeList
  // should keep the platform implementation and avoid setting the env-only bit.
  ScopedExternalBoolEnv enable_native_list_env(LynxEnv::Key::ENABLE_NATIVE_LIST,
                                               false);
  page_config_->SetEnableNativeList(TernaryBool::UNDEFINE_VALUE);

  list_element_->ResolveEnableNativeList();
  list_element_->ResolvePlatformNodeTag();

  EXPECT_FALSE(list_element_->DisableListPlatformImplementation());
  EXPECT_FALSE(list_element_->enable_native_list_only_from_env_);
  EXPECT_EQ(list_element_->GetPlatformNodeTag().str(), list::kList);
}

TEST_F(ListElementTest, NewStylingInternalListReplaysListAxisGap) {
  auto list = CreateNewStylingNativeList(false);

  EXPECT_EQ(ListAxisGaps(list.get()), std::make_pair(12.f, 4.f));

  list->SetRawInlineStyles(
      "list-main-axis-gap: 20px; list-cross-axis-gap: 6px;");
  page_->FlushActionsAsRoot();
  EXPECT_EQ(ListAxisGaps(list.get()), std::make_pair(20.f, 6.f));

  list->RemoveAllInlineStyles();
  page_->FlushActionsAsRoot();
  EXPECT_EQ(ListAxisGaps(list.get()), std::make_pair(0.f, 0.f));
}

TEST_F(ListElementTest, NewStylingDecoupledListReplaysListAxisGap) {
  auto list = CreateNewStylingNativeList(true);

  EXPECT_EQ(ListAxisGaps(list.get()), std::make_pair(12.f, 4.f));

  list->SetRawInlineStyles(
      "list-main-axis-gap: 20px; list-cross-axis-gap: 6px;");
  page_->FlushActionsAsRoot();
  EXPECT_EQ(ListAxisGaps(list.get()), std::make_pair(20.f, 6.f));

  list->RemoveAllInlineStyles();
  page_->FlushActionsAsRoot();
  EXPECT_EQ(ListAxisGaps(list.get()), std::make_pair(0.f, 0.f));
}

TEST_F(ListElementTest,
       NewStylingInternalListResolvesAttributeDerivedLayoutInputs) {
  auto list = CreateNewStylingNativeList(false);

  list->SetAttribute(list::kScrollOrientation, lepus::Value("horizontal"));
  list->SetAttribute(list::kSpanCount, lepus::Value(3));
  page_->FlushActionsAsRoot();

  EXPECT_EQ(ListScrollDirections(list.get()), std::make_pair(true, false));
  EXPECT_TRUE(ResolvedStyleHasValue(
      list.get(), kPropertyIDLinearOrientation,
      CSSValue(starlight::LinearOrientationType::kHorizontal)));
  EXPECT_EQ(ListSpanCount(list.get()), 3);

  list->SetAttribute(list::kScrollOrientation, lepus::Value("vertical"));
  page_->FlushActionsAsRoot();

  EXPECT_EQ(ListScrollDirections(list.get()), std::make_pair(false, true));
  EXPECT_TRUE(ResolvedStyleHasValue(
      list.get(), kPropertyIDLinearOrientation,
      CSSValue(starlight::LinearOrientationType::kVertical)));
}

TEST_F(ListElementTest,
       NewStylingDecoupledListResolvesAttributeDerivedLayoutInputs) {
  auto list = CreateNewStylingNativeList(true);

  list->SetAttribute(list::kScrollOrientation, lepus::Value("horizontal"));
  list->SetAttribute(list::kSpanCount, lepus::Value(3));
  page_->FlushActionsAsRoot();

  EXPECT_EQ(ListScrollDirections(list.get()), std::make_pair(true, false));
  EXPECT_TRUE(ResolvedStyleHasValue(
      list.get(), kPropertyIDLinearOrientation,
      CSSValue(starlight::LinearOrientationType::kHorizontal)));
  EXPECT_EQ(ListSpanCount(list.get()), 3);

  list->SetAttribute(list::kScrollOrientation, lepus::Value("vertical"));
  page_->FlushActionsAsRoot();

  EXPECT_EQ(ListScrollDirections(list.get()), std::make_pair(false, true));
  EXPECT_TRUE(ResolvedStyleHasValue(
      list.get(), kPropertyIDLinearOrientation,
      CSSValue(starlight::LinearOrientationType::kVertical)));
}

TEST_F(ListElementTest, NewStylingInternalListKeepsAxisGapAfterListTypeChange) {
  auto list = CreateNewStylingNativeList(false);

  list->SetAttribute(list::kListType, lepus::Value(list::kListTypeFlow));
  page_->FlushActionsAsRoot();

  EXPECT_EQ(ListAxisGaps(list.get()), std::make_pair(12.f, 4.f));
}

TEST_F(ListElementTest,
       NewStylingDecoupledListKeepsAxisGapAfterListTypeChange) {
  auto list = CreateNewStylingNativeList(true);

  list->SetAttribute(list::kListType, lepus::Value(list::kListTypeFlow));
  page_->FlushActionsAsRoot();

  EXPECT_EQ(ListAxisGaps(list.get()), std::make_pair(12.f, 4.f));
}

TEST_F(SSRListElementTest, AttributeStyleCacheMirrorsCommittedStyleCache) {
  list_element_->CacheStyleFromAttributes(CSSPropertyID::kPropertyIDWidth,
                                          CSSValue(120, CSSValuePattern::PX));

  const auto* cached_styles = list_element_->PeekCachedStylesFromAttributes();
  ASSERT_NE(cached_styles, nullptr);
  auto cached_it = cached_styles->find(CSSPropertyID::kPropertyIDWidth);
  ASSERT_TRUE(cached_it != cached_styles->end());
  EXPECT_EQ(cached_it->second, CSSValue(120, CSSValuePattern::PX));

  const auto* committed_styles =
      list_element_->PeekCommittedStylesFromAttributes();
  ASSERT_NE(committed_styles, nullptr);
  auto committed_it = committed_styles->find(CSSPropertyID::kPropertyIDWidth);
  ASSERT_TRUE(committed_it != committed_styles->end());
  EXPECT_EQ(committed_it->second, CSSValue(120, CSSValuePattern::PX));

  list_element_->RemoveStyleFromAttributes(CSSPropertyID::kPropertyIDWidth);
  EXPECT_EQ(list_element_->PeekCachedStylesFromAttributes(), nullptr);
  EXPECT_EQ(list_element_->PeekCommittedStylesFromAttributes(), nullptr);
}

TEST_F(SSRListElementTest, ScrollOrientationAttributeUsesAttributeStyleCache) {
  list_element_->SetAttributeInternal(base::String("scroll-orientation"),
                                      lepus::Value("horizontal"));

  const auto* cached_styles = list_element_->PeekCachedStylesFromAttributes();
  ASSERT_NE(cached_styles, nullptr);
  auto cached_it =
      cached_styles->find(CSSPropertyID::kPropertyIDLinearOrientation);
  ASSERT_TRUE(cached_it != cached_styles->end());
  EXPECT_EQ(cached_it->second,
            CSSValue(starlight::LinearOrientationType::kHorizontal));

  const auto* committed_styles =
      list_element_->PeekCommittedStylesFromAttributes();
  ASSERT_NE(committed_styles, nullptr);
  auto committed_it =
      committed_styles->find(CSSPropertyID::kPropertyIDLinearOrientation);
  ASSERT_TRUE(committed_it != committed_styles->end());
  EXPECT_EQ(committed_it->second,
            CSSValue(starlight::LinearOrientationType::kHorizontal));
  EXPECT_TRUE(list_element_->parsed_styles_map_.find(
                  CSSPropertyID::kPropertyIDLinearOrientation) ==
              list_element_->parsed_styles_map_.end());
}

TEST_F(SSRListElementTest, ListElementSSRHelper_ComponentAtIndexInSSR) {
  auto items = std::vector<fml::RefPtr<FiberElement>>();
  ListElementSSRHelper ssr_helper(list_element_.get());

  static const size_t kItemCounts = 10;
  for (size_t index = 0; index < kItemCounts; index++) {
    auto item = fml::AdoptRef<ComponentElement>(
        new ComponentElement(manager_, "", 1, "", "", ""));
    ssr_helper.AppendChild(item);
    items.emplace_back(item);
  }

  // ssr list init
  list_element_->SetSsrHelper(std::move(ssr_helper));
  for (size_t index = 0; index < kItemCounts; index++) {
    EXPECT_TRUE(ListElementSSRHelper::SSRItemStatus::kWaitingRender ==
                list_element_->ssr_helper_->ssr_elements_[index].second);
  }
  EXPECT_TRUE(0 == list_element_->children().size());
  EXPECT_FALSE(list_element_->element_manager() == nullptr);
  // list first screen.
  list_element_->ComponentAtIndex(0, -1, false);
  list_element_->ComponentAtIndex(1, -1, false);
  list_element_->ComponentAtIndex(2, -1, false);

  EXPECT_TRUE(ListElementSSRHelper::SSRItemStatus::kRendered ==
              list_element_->ssr_helper_->ssr_elements_[0].second);
  EXPECT_TRUE(ListElementSSRHelper::SSRItemStatus::kRendered ==
              list_element_->ssr_helper_->ssr_elements_[1].second);
  EXPECT_TRUE(ListElementSSRHelper::SSRItemStatus::kRendered ==
              list_element_->ssr_helper_->ssr_elements_[2].second);
  EXPECT_TRUE(3 == list_element_->children().size());

  // list scroll down.
  list_element_->EnqueueComponent(items[0]->impl_id());
  list_element_->ComponentAtIndex(3, -1, false);
  list_element_->EnqueueComponent(items[1]->impl_id());
  list_element_->ComponentAtIndex(4, -1, false);
  list_element_->EnqueueComponent(items[2]->impl_id());
  list_element_->ComponentAtIndex(5, -1, false);

  EXPECT_TRUE(ListElementSSRHelper::SSRItemStatus::kEnqueued ==
              list_element_->ssr_helper_->ssr_elements_[0].second);
  EXPECT_TRUE(ListElementSSRHelper::SSRItemStatus::kEnqueued ==
              list_element_->ssr_helper_->ssr_elements_[1].second);
  EXPECT_TRUE(ListElementSSRHelper::SSRItemStatus::kEnqueued ==
              list_element_->ssr_helper_->ssr_elements_[2].second);
  EXPECT_TRUE(ListElementSSRHelper::SSRItemStatus::kRendered ==
              list_element_->ssr_helper_->ssr_elements_[3].second);
  EXPECT_TRUE(ListElementSSRHelper::SSRItemStatus::kRendered ==
              list_element_->ssr_helper_->ssr_elements_[4].second);
  EXPECT_TRUE(ListElementSSRHelper::SSRItemStatus::kRendered ==
              list_element_->ssr_helper_->ssr_elements_[5].second);
  EXPECT_TRUE(6 == list_element_->children().size());

  // list scroll up.
  list_element_->EnqueueComponent(items[5]->impl_id());
  list_element_->ComponentAtIndex(2, -1, false);
  list_element_->EnqueueComponent(items[4]->impl_id());
  list_element_->ComponentAtIndex(1, -1, false);

  EXPECT_TRUE(ListElementSSRHelper::SSRItemStatus::kEnqueued ==
              list_element_->ssr_helper_->ssr_elements_[0].second);
  EXPECT_TRUE(ListElementSSRHelper::SSRItemStatus::kRendered ==
              list_element_->ssr_helper_->ssr_elements_[1].second);
  EXPECT_TRUE(ListElementSSRHelper::SSRItemStatus::kRendered ==
              list_element_->ssr_helper_->ssr_elements_[2].second);
  EXPECT_TRUE(ListElementSSRHelper::SSRItemStatus::kRendered ==
              list_element_->ssr_helper_->ssr_elements_[3].second);
  EXPECT_TRUE(ListElementSSRHelper::SSRItemStatus::kEnqueued ==
              list_element_->ssr_helper_->ssr_elements_[4].second);
  EXPECT_TRUE(ListElementSSRHelper::SSRItemStatus::kEnqueued ==
              list_element_->ssr_helper_->ssr_elements_[5].second);
  EXPECT_TRUE(6 == list_element_->children().size());

  // hydrate
  list_element_->Hydrate();
  EXPECT_TRUE(kItemCounts == list_element_->children().size());

  EXPECT_TRUE(ListElementSSRHelper::SSRItemStatus::kEnqueued ==
              list_element_->ssr_helper_->ssr_elements_[0].second);
  EXPECT_TRUE(ListElementSSRHelper::SSRItemStatus::kRendered ==
              list_element_->ssr_helper_->ssr_elements_[1].second);
  EXPECT_TRUE(ListElementSSRHelper::SSRItemStatus::kRendered ==
              list_element_->ssr_helper_->ssr_elements_[2].second);
  EXPECT_TRUE(ListElementSSRHelper::SSRItemStatus::kRendered ==
              list_element_->ssr_helper_->ssr_elements_[3].second);
  EXPECT_TRUE(ListElementSSRHelper::SSRItemStatus::kEnqueued ==
              list_element_->ssr_helper_->ssr_elements_[4].second);
  EXPECT_TRUE(ListElementSSRHelper::SSRItemStatus::kEnqueued ==
              list_element_->ssr_helper_->ssr_elements_[5].second);
  EXPECT_TRUE(ListElementSSRHelper::SSRItemStatus::kEnqueued ==
              list_element_->ssr_helper_->ssr_elements_[6].second);
  EXPECT_TRUE(ListElementSSRHelper::SSRItemStatus::kEnqueued ==
              list_element_->ssr_helper_->ssr_elements_[7].second);
  EXPECT_TRUE(ListElementSSRHelper::SSRItemStatus::kEnqueued ==
              list_element_->ssr_helper_->ssr_elements_[8].second);
  EXPECT_TRUE(ListElementSSRHelper::SSRItemStatus::kEnqueued ==
              list_element_->ssr_helper_->ssr_elements_[9].second);
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
