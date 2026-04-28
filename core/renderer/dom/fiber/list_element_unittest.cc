// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/dom/fiber/list_element.h"

#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

static constexpr int32_t kWidth = 1080;
static constexpr int32_t kHeight = 1920;
static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;

class SSRListElement : public ::testing::Test {
 public:
  SSRListElement() = default;
  ~SSRListElement() override = default;

  fml::RefPtr<PageElement> page_;
  fml::RefPtr<ListElement> list_element_;

  std::shared_ptr<::testing::NiceMock<test::MockTasmDelegate>> tasm_mediator_;
  std::shared_ptr<lynx::tasm::TemplateAssembler> tasm_;
  ElementManager* manager_ = nullptr;

  void SetUp() override {
    LynxEnvConfig lynx_env_config(kWidth, kHeight, kDefaultLayoutsUnitPerPx,
                                  kDefaultPhysicalPixelsPerLayoutUnit);
    tasm_mediator_ = std::make_shared<
        ::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>();
    auto manager = std::make_unique<lynx::tasm::ElementManager>(
        std::make_unique<MockPaintingContext>(), tasm_mediator_.get(),
        lynx_env_config);
    auto config = std::make_shared<PageConfig>();
    config->SetEnableFiberArch(true);
    manager->SetConfig(config);
    manager_ = manager.get();
    tasm_ = std::make_shared<lynx::tasm::TemplateAssembler>(
        *tasm_mediator_.get(), std::move(manager), tasm_mediator_.get(), 0);

    page_ = fml::AdoptRef<PageElement>(new PageElement(manager_, "page", 0));
    list_element_ = fml::AdoptRef<ListElement>(new ListElement(
        manager_, "list", lepus::Value(), lepus::Value(), lepus::Value()));
    page_->InsertNode(list_element_);
    list_element_->set_tasm(tasm_.get());
  }
};

TEST_F(SSRListElement, AttributeStyleCacheMirrorsCommittedStyleCache) {
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

TEST_F(SSRListElement, ListElementSSRHelper_ComponentAtIndexInSSR) {
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
