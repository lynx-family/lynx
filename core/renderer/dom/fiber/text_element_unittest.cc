// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#define private public
#define protected public

#include "core/renderer/dom/fiber/text_element.h"

#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/image_element.h"
#include "core/renderer/dom/fiber/raw_text_element.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/shell/tasm_operation_queue.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

static constexpr int32_t kWidth = 1080;
static constexpr int32_t kHeight = 1920;
static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;

class TextElementTest : public ::testing::Test {
 public:
  TextElementTest() {}
  ~TextElementTest() override {}
  lynx::tasm::ElementManager* manager;
  std::shared_ptr<::testing::NiceMock<test::MockTasmDelegate>> tasm_mediator;
  std::shared_ptr<lynx::tasm::TemplateAssembler> tasm;

  void SetUp() override {
    LynxEnvConfig lynx_env_config(kWidth, kHeight, kDefaultLayoutsUnitPerPx,
                                  kDefaultPhysicalPixelsPerLayoutUnit);
    tasm_mediator = std::make_shared<
        ::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>();
    auto unique_manager = std::make_unique<lynx::tasm::ElementManager>(
        std::make_unique<MockPaintingContext>(), tasm_mediator.get(),
        lynx_env_config);
    manager = unique_manager.get();
    tasm = std::make_shared<lynx::tasm::TemplateAssembler>(
        *tasm_mediator.get(), std::move(unique_manager), 0);

    auto test_entry = std::make_shared<TemplateEntry>();
    tasm->template_entries_.insert({"test_entry", test_entry});

    auto config = std::make_shared<PageConfig>();
    config->SetEnableZIndex(true);
    manager->SetConfig(config);
  }
};

TEST_F(TextElementTest, TestInlineText) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);

  auto page = manager->CreateFiberPage("page", 11);

  auto text = manager->CreateFiberText("text");

  auto raw_text = manager->CreateFiberRawText();
  auto content = lepus::Value("text-content");
  raw_text->SetText(content);

  page->InsertNode(text);
  text->InsertNode(raw_text);

  // inline text
  auto inline_text = manager->CreateFiberText("text");

  auto inline_raw_text = manager->CreateFiberRawText();
  auto inline_content = lepus::Value("inline-text-content");
  inline_raw_text->SetText(inline_content);
  inline_text->InsertNode(inline_raw_text);
  text->InsertNode(inline_text);

  page->FlushActionsAsRoot();

  const auto& attributes = raw_text->data_model_->attributes();
  EXPECT_TRUE(attributes.at(RawTextElement::kTextAttr) ==
              lepus::Value("text-content"));

  const auto& inline_attributes = inline_raw_text->data_model_->attributes();
  EXPECT_TRUE(inline_attributes.at(RawTextElement::kTextAttr) ==
              lepus::Value("inline-text-content"));

  // check element tree
  EXPECT_TRUE(text->GetTag() == "text");
  EXPECT_FALSE(text->is_inline_element());

  EXPECT_TRUE(inline_text->is_inline_element());
  EXPECT_TRUE(inline_text->GetTag() == "inline-text");
}

TEST_F(TextElementTest, TestXInlineText) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);

  auto page = manager->CreateFiberPage("page", 11);

  auto text = manager->CreateFiberText("x-text");

  auto raw_text = manager->CreateFiberRawText();
  auto content = lepus::Value("text-content");
  raw_text->SetText(content);

  page->InsertNode(text);
  text->InsertNode(raw_text);

  // inline text
  auto inline_text = manager->CreateFiberText("x-text");

  auto inline_raw_text = manager->CreateFiberRawText();
  auto inline_content = lepus::Value("inline-text-content");
  inline_raw_text->SetText(inline_content);
  inline_text->InsertNode(inline_raw_text);
  text->InsertNode(inline_text);

  page->FlushActionsAsRoot();

  const auto& attributes = raw_text->data_model_->attributes();
  EXPECT_TRUE(attributes.at(RawTextElement::kTextAttr) ==
              lepus::Value("text-content"));

  const auto& inline_attributes = inline_raw_text->data_model_->attributes();
  EXPECT_TRUE(inline_attributes.at(RawTextElement::kTextAttr) ==
              lepus::Value("inline-text-content"));

  // check element tree
  EXPECT_EQ(text->GetTag(), "x-text");
  EXPECT_FALSE(text->is_inline_element());

  EXPECT_TRUE(inline_text->is_inline_element());
  EXPECT_EQ(inline_text->GetTag(), "x-inline-text");
}

TEST_F(TextElementTest, TestInlineTextAndImage) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);

  auto page = manager->CreateFiberPage("page", 11);

  auto text = manager->CreateFiberText("text");

  auto raw_text = manager->CreateFiberRawText();
  auto content = lepus::Value("text-content");
  raw_text->SetText(content);

  page->InsertNode(text);
  text->InsertNode(raw_text);

  // inline text
  auto inline_text = manager->CreateFiberText("text");

  auto inline_raw_text = manager->CreateFiberRawText();
  auto inline_content = lepus::Value("inline-text-content");
  inline_raw_text->SetText(inline_content);
  inline_text->InsertNode(inline_raw_text);
  text->InsertNode(inline_text);

  // inline image
  auto inline_image = manager->CreateFiberImage("image");

  auto image_src = lepus::Value("inline-image-src://");
  inline_image->SetAttribute("src", image_src);
  text->InsertNode(inline_image);

  page->FlushActionsAsRoot();

  const auto& attributes = raw_text->data_model_->attributes();
  EXPECT_TRUE(attributes.at(RawTextElement::kTextAttr) ==
              lepus::Value("text-content"));

  const auto& inline_attributes = inline_raw_text->data_model_->attributes();
  EXPECT_TRUE(inline_attributes.at(RawTextElement::kTextAttr) ==
              lepus::Value("inline-text-content"));

  const auto& inline_image_attributes = inline_image->data_model_->attributes();
  EXPECT_TRUE(inline_image_attributes.at("src") ==
              lepus::Value("inline-image-src://"));

  // check element tree
  EXPECT_TRUE(text->GetTag() == "text");
  EXPECT_FALSE(text->is_inline_element());

  EXPECT_TRUE(inline_text->is_inline_element());
  EXPECT_TRUE(inline_text->GetTag() == "inline-text");
  EXPECT_TRUE(inline_text->parent() == text.get());

  EXPECT_TRUE(inline_image->is_inline_element());
  EXPECT_TRUE(inline_image->GetTag() == "inline-image");
  EXPECT_TRUE(inline_image->parent() == text.get());
}

TEST_F(TextElementTest, TestSetTextOverflow) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);

  auto page = manager->CreateFiberPage("page", 11);

  auto text = manager->CreateFiberText("text");

  auto raw_text = manager->CreateFiberRawText();
  auto content = lepus::Value("text-content");
  raw_text->SetText(content);

  page->InsertNode(text);
  text->InsertNode(raw_text);

  text->SetAttribute("text-overflow", lepus::Value("ellipsis"));

  page->FlushActionsAsRoot();
  auto* mock_text_painting_node_ =
      static_cast<MockPaintingContext*>(
          page->painting_context()->platform_impl_.get())
          ->node_map_.at(text->impl_id())
          .get();

  EXPECT_TRUE(mock_text_painting_node_->props_.size() == 1);
  std::string key("text-overflow");
  EXPECT_TRUE(
      mock_text_painting_node_->props_.at(key) ==
      lepus::Value(static_cast<int>(starlight::TextOverflowType::kEllipsis)));

  text->SetAttribute("text-overflow", lepus::Value("clip"));
  text->SetAttribute("layout-only", lepus::Value("false"));

  text->FlushActionsAsRoot();

  EXPECT_TRUE(mock_text_painting_node_->props_.size() == 2);
  EXPECT_TRUE(
      mock_text_painting_node_->props_.at(key) ==
      lepus::Value(static_cast<int>(starlight::TextOverflowType::kClip)));
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
