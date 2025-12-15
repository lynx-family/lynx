// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#define private public
#define protected public

#include "core/renderer/dom/fiber/text_element.h"

#include <unordered_map>
#include <variant>
#include <vector>

#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/image_element.h"
#include "core/renderer/dom/fiber/raw_text_element.h"
#include "core/renderer/dom/testing/fiber_element_test.h"
#include "core/renderer/dom/testing/fiber_mock_text_layout.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/shell/tasm_operation_queue.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

class TextElementTest : public FiberElementTest {};

TEST_P(TextElementTest, TestInlineText) {
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

  auto& attributes = raw_text->data_model_->attributes();
  EXPECT_TRUE(attributes.at(RawTextElement::kTextAttr) ==
              lepus::Value("text-content"));

  auto& inline_attributes = inline_raw_text->data_model_->attributes();
  EXPECT_TRUE(inline_attributes.at(RawTextElement::kTextAttr) ==
              lepus::Value("inline-text-content"));

  // check element tree
  EXPECT_TRUE(text->GetTag() == "text");
  EXPECT_FALSE(text->is_inline_element());

  EXPECT_TRUE(inline_text->is_inline_element());
  EXPECT_TRUE(inline_text->GetTag() == "inline-text");
}

TEST_P(TextElementTest, TestXInlineText) {
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

  auto& attributes = raw_text->data_model_->attributes();
  EXPECT_TRUE(attributes.at(RawTextElement::kTextAttr) ==
              lepus::Value("text-content"));

  auto& inline_attributes = inline_raw_text->data_model_->attributes();
  EXPECT_TRUE(inline_attributes.at(RawTextElement::kTextAttr) ==
              lepus::Value("inline-text-content"));

  // check element tree
  EXPECT_EQ(text->GetTag(), "x-text");
  EXPECT_FALSE(text->is_inline_element());

  EXPECT_TRUE(inline_text->is_inline_element());
  EXPECT_EQ(inline_text->GetTag(), "x-inline-text");
}

TEST_P(TextElementTest, TestInlineTextAndImage) {
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

  auto& attributes = raw_text->data_model_->attributes();
  EXPECT_TRUE(attributes.at(RawTextElement::kTextAttr) ==
              lepus::Value("text-content"));

  auto& inline_attributes = inline_raw_text->data_model_->attributes();
  EXPECT_TRUE(inline_attributes.at(RawTextElement::kTextAttr) ==
              lepus::Value("inline-text-content"));

  auto& inline_image_attributes = inline_image->data_model_->attributes();
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

TEST_P(TextElementTest, TestSetTextOverflow) {
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
  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  auto* mock_text_painting_node_ =
      static_cast<FiberMockPaintingContext*>(
          manager->painting_context()->platform_impl_.get())
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
  painting_context->Flush();

  EXPECT_TRUE(mock_text_painting_node_->props_.size() == 2);
  EXPECT_TRUE(
      mock_text_painting_node_->props_.at(key) ==
      lepus::Value(static_cast<int>(starlight::TextOverflowType::kClip)));
}

TEST_P(TextElementTest, TestConvertContent) {
  EXPECT_EQ(TextElement::ConvertContent(lepus::Value("test")),
            base::String("test"));
  EXPECT_EQ(TextElement::ConvertContent(lepus::Value((int32_t)1)),
            base::String("1"));
  EXPECT_EQ(TextElement::ConvertContent(lepus::Value((int64_t)11231212121212)),
            base::String("11231212121212"));
  EXPECT_EQ(TextElement::ConvertContent(lepus::Value(1.00)), base::String("1"));
  EXPECT_EQ(TextElement::ConvertContent(lepus::Value(1.10)),
            base::String("1.1"));
  EXPECT_EQ(TextElement::ConvertContent(lepus::Value(1.1)),
            base::String("1.1"));
  EXPECT_EQ(TextElement::ConvertContent(lepus::Value()), base::String("null"));
}

TEST_P(TextElementTest, TestResolveStyleValue) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);
  manager->enable_layout_in_element_mode_ = true;

  auto page = manager->CreateFiberPage("page", 11);

  auto text = manager->CreateFiberText("text");

  auto raw_text = manager->CreateFiberRawText();
  auto content = lepus::Value("text-content");
  raw_text->SetText(content);

  page->InsertNode(text);
  text->InsertNode(raw_text);

  text->SetAttribute("text-maxline", lepus::Value(1));
  text->SetRawInlineStyles(base::String("color:red;"));

  page->FlushActionsAsRoot();
  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();
  auto* mock_text_painting_node_ =
      painting_context->node_map_.at(text->impl_id()).get();

  EXPECT_EQ(mock_text_painting_node_->props_.size(), 1);
  std::string key("text-overflow");
  EXPECT_EQ(text->text_props_->text_max_line, 1);
  EXPECT_TRUE(text->property_bits_.Has(kPropertyIDColor));
  EXPECT_EQ(text->computed_css_style()->GetTextAttributes()->color, 4294901760);

  text->SetAttribute("text-maxline", lepus::Value(2));
  text->SetAttribute("layout-only", lepus::Value("false"));
  text->RemoveAllInlineStyles();
  text->SetRawInlineStyles(base::String(""));

  text->FlushActionsAsRoot();

  EXPECT_EQ(text->text_props_->text_max_line, 2);
  EXPECT_TRUE(text->property_bits_.Has(kPropertyIDColor));
  EXPECT_EQ(text->computed_css_style()->GetTextAttributes()->color, 4278190080);
}

TEST_P(TextElementTest, TestMeasureCase0) {
  if (enable_parallel_element_flush_strategy > 0) {
    GTEST_SKIP();
  }
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);
  manager->enable_layout_in_element_mode_ = true;
  manager->OnUpdateViewport(720, 1, 1080, 1, true);

  tasm->layout_scheduler_ = std::make_unique<LayoutScheduler>(manager);

  auto page = manager->CreateFiberPage("page", 11);

  auto text = manager->CreateFiberText("text");

  text->SetRawInlineStyles(base::String("color:red;"));
  text->SetAttribute("text-maxline", lepus::Value(1));

  auto raw_text = manager->CreateFiberRawText();
  auto content = lepus::Value("text-content");
  raw_text->SetText(content);

  page->InsertNode(text);
  text->InsertNode(raw_text);

  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page.get());

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  auto* mock_text_painting_node_ =
      painting_context->node_map_.at(text->impl_id()).get();

  EXPECT_EQ(mock_text_painting_node_->props_.size(), 1);
  EXPECT_EQ(text->text_props_->text_max_line, 1);
  EXPECT_TRUE(text->property_bits_.Has(kPropertyIDColor));
  EXPECT_EQ(text->computed_css_style()->GetTextAttributes()->color, 4294901760);

  auto* mock_text_layout =
      static_cast<TextLayoutMock*>((painting_context->text_layout_impl_).get());

  auto& mock_text_prop_array =
      mock_text_layout->text_layout_results_.at(text->impl_id()).get()->props_;

  // check prop array
  // color
  EXPECT_TRUE(std::get<int>(mock_text_prop_array[0]) == kTextPropColor);
  EXPECT_TRUE(std::get<int>(mock_text_prop_array[1]) == -65536);

  // text max-line
  EXPECT_TRUE(std::get<int>(mock_text_prop_array[2]) == kTextPropTextMaxLine);
  EXPECT_TRUE(std::get<int>(mock_text_prop_array[3]) == 1);

  // text string
  EXPECT_TRUE(std::get<int>(mock_text_prop_array[4]) == kPropTextString);
  EXPECT_TRUE(std::get<std::string>(mock_text_prop_array[5]) == "text-content");

  // inline-text
  auto inline_text = manager->CreateFiberText("text");
  //'测试' is 'testing', used for test the word's lenght in Chinese
  inline_text->SetAttribute("text", lepus::Value("-inline-测试"));
  inline_text->SetRawInlineStyles(base::String("color:blue;"));
  text->InsertNode(inline_text);

  manager->OnPatchFinish(options, page.get());
  painting_context->Flush();

  int para_content_utf16_length = raw_text->content_utf16_length();
  int inline_content_utf16_length = inline_text->content_utf16_length();

  EXPECT_TRUE(para_content_utf16_length == 12);
  EXPECT_TRUE(inline_content_utf16_length == 10);

  auto& inline_mock_text_prop_array =
      mock_text_layout->text_layout_results_.at(text->impl_id()).get()->props_;
  (void)inline_mock_text_prop_array;

  // kPropInlineStart
  EXPECT_TRUE(std::get<int>(inline_mock_text_prop_array[0]) ==
              kPropInlineStart);
  EXPECT_TRUE(std::get<int>(inline_mock_text_prop_array[1]) ==
              para_content_utf16_length);

  // kTextPropColor
  EXPECT_TRUE(std::get<int>(inline_mock_text_prop_array[2]) == kTextPropColor);
  EXPECT_TRUE(std::get<int>(inline_mock_text_prop_array[3]) ==
              -16776961);  // blue

  // kPropInlineEnd
  EXPECT_TRUE(std::get<int>(inline_mock_text_prop_array[4]) == kPropInlineEnd);
  EXPECT_TRUE(std::get<int>(inline_mock_text_prop_array[5]) ==
              para_content_utf16_length + inline_content_utf16_length);

  //--- para
  // color
  EXPECT_TRUE(std::get<int>(inline_mock_text_prop_array[6]) == kTextPropColor);
  EXPECT_TRUE(std::get<int>(inline_mock_text_prop_array[7]) == -65536);

  // text max-line
  EXPECT_TRUE(std::get<int>(inline_mock_text_prop_array[8]) ==
              kTextPropTextMaxLine);
  EXPECT_TRUE(std::get<int>(inline_mock_text_prop_array[9]) == 1);

  // string
  EXPECT_TRUE(std::get<int>(inline_mock_text_prop_array[10]) ==
              kPropTextString);

  //'测试' is 'testing', used for test the word's lenght in Chinese
  EXPECT_TRUE(std::get<std::string>(inline_mock_text_prop_array[11]) ==
              "text-content-inline-测试");
}

TEST_P(TextElementTest, LayoutInElementFontScale) {
  if (enable_parallel_element_flush_strategy > 0) {
    GTEST_SKIP();
  }
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);
  manager->enable_layout_in_element_mode_ = true;
  manager->OnUpdateViewport(720, 1, 1080, 1, true);

  tasm->layout_scheduler_ = std::make_unique<LayoutScheduler>(manager);

  float mFontScale = 3.0f;
  // set font-scale:3
  manager->UpdateFontScale(mFontScale);

  auto page = manager->CreateFiberPage("page", 11);

  auto text = manager->CreateFiberText("text");

  auto raw_text = manager->CreateFiberRawText();
  auto content = lepus::Value("text-content");
  raw_text->SetText(content);

  page->InsertNode(text);
  text->InsertNode(raw_text);

  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page.get());

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  auto* mock_text_layout =
      static_cast<TextLayoutMock*>((painting_context->text_layout_impl_).get());

  auto& mock_text_prop_array =
      mock_text_layout->text_layout_results_.at(text->impl_id()).get()->props_;

  // check prop array
  // color
  EXPECT_TRUE(std::get<int>(mock_text_prop_array[0]) == kTextPropFontSize);
  EXPECT_TRUE(std::get<double>(mock_text_prop_array[1]) ==
              manager->GetLynxEnvConfig().DefaultFontSize() * mFontScale);

  // text string
  EXPECT_TRUE(std::get<int>(mock_text_prop_array[2]) == kPropTextString);
  EXPECT_TRUE(std::get<std::string>(mock_text_prop_array[3]) == "text-content");
}

INSTANTIATE_TEST_SUITE_P(TextElementTestModule, TextElementTest,
                         ::testing::ValuesIn(fiber_element_generation_params));

TEST_P(TextElementTest, TextGradient) {
  if (enable_parallel_element_flush_strategy > 0) {
    GTEST_SKIP();
  }
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);
  manager->enable_layout_in_element_mode_ = true;
  manager->OnUpdateViewport(720, 1, 1080, 1, true);

  tasm->layout_scheduler_ = std::make_unique<LayoutScheduler>(manager);

  auto page = manager->CreateFiberPage("page", 11);

  auto text = manager->CreateFiberText("text");

  text->SetRawInlineStyles(
      base::String("color:linear-gradient(90deg, red, blue);"));

  auto raw_text = manager->CreateFiberRawText();
  auto content = lepus::Value("text-content");
  raw_text->SetText(content);

  page->InsertNode(text);
  text->InsertNode(raw_text);

  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page.get());

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  auto* mock_text_layout =
      static_cast<TextLayoutMock*>((painting_context->text_layout_impl_).get());

  auto& mock_text_prop_array =
      mock_text_layout->text_layout_results_.at(text->impl_id()).get()->props_;

  // check prop array
  // linear-gradient
  EXPECT_TRUE(std::get<int>(mock_text_prop_array[0]) ==
              kPropColorLinearGradient);
  // angle
  EXPECT_EQ(std::get<double>(mock_text_prop_array[1]), 90.0);

  // color_count
  EXPECT_EQ(std::get<int>(mock_text_prop_array[2]), 2);
  // Colors
  // color red
  EXPECT_EQ(std::get<int>(mock_text_prop_array[3]), -65536);
  // color blue
  EXPECT_EQ(std::get<int>(mock_text_prop_array[4]), -16776961);

  // position_count
  EXPECT_EQ(std::get<int>(mock_text_prop_array[5]), 0);

  // radial-gradient
  text->SetRawInlineStyles(
      base::String("color:radial-gradient(circle, red, blue);"));
  manager->OnPatchFinish(options, page.get());
  painting_context->Flush();

  auto& mock_text_prop_array_radial =
      mock_text_layout->text_layout_results_.at(text->impl_id()).get()->props_;

  EXPECT_TRUE(std::get<int>(mock_text_prop_array_radial[0]) ==
              kPropColorRadialGradient);
  // check shape array

  // shape
  EXPECT_EQ(std::get<int>(mock_text_prop_array_radial[1]), 1);
  // shape_size
  int shape_size = std::get<int>(mock_text_prop_array_radial[2]);
  EXPECT_EQ(shape_size, 0);
  // pos_x_pattern
  EXPECT_EQ(std::get<int>(mock_text_prop_array_radial[3]),
            static_cast<int>(CSSValuePattern::PERCENT));
  // pos_x_value
  EXPECT_EQ(std::get<double>(mock_text_prop_array_radial[4]), 50.0f);
  // pos_y_pattern
  EXPECT_EQ(std::get<int>(mock_text_prop_array_radial[5]),
            static_cast<int>(CSSValuePattern::PERCENT));
  // pos_y_value
  EXPECT_EQ(std::get<double>(mock_text_prop_array_radial[6]), 50.0f);
  // color_count
  EXPECT_EQ(std::get<int>(mock_text_prop_array_radial[7]), 2);
  // colors
  EXPECT_EQ(std::get<int>(mock_text_prop_array_radial[8]), -65536);
  EXPECT_EQ(std::get<int>(mock_text_prop_array_radial[9]), -16776961);
  // positions
  EXPECT_EQ(std::get<int>(mock_text_prop_array_radial[10]), 0);
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
