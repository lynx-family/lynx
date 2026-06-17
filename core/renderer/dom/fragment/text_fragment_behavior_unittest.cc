// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/text_fragment_behavior.h"

#include <memory>
#include <utility>

#include "core/event/event.h"
#include "core/event/event_listener.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/text_element.h"
#include "core/renderer/dom/fragment/fragment.h"
#include "core/renderer/lynx_env_config.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx::tasm {

namespace {

static constexpr int32_t kConfigWidth = 1080;
static constexpr int32_t kConfigHeight = 1920;
static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;

class RecordingEventListener : public event::EventListener {
 public:
  RecordingEventListener()
      : EventListener(event::EventListener::Type::kClosureEventListener) {}

  void Invoke(fml::RefPtr<event::Event> event) override {
    ++count_;
    last_event_type_ = event->type();
    last_detail_ = event->detail();
  }

  bool Matches(event::EventListener* listener) override {
    return this == listener;
  }

  int count() const { return count_; }
  const std::string& last_event_type() const { return last_event_type_; }
  const lepus::Value& last_detail() const { return last_detail_; }

 private:
  int count_{0};
  std::string last_event_type_;
  lepus::Value last_detail_{lepus::Dictionary::Create()};
};

class TextFragmentBehaviorTest : public ::testing::Test {
 protected:
  void SetUp() override {
    LynxEnvConfig lynx_env_config(kConfigWidth, kConfigHeight,
                                  kDefaultLayoutsUnitPerPx,
                                  kDefaultPhysicalPixelsPerLayoutUnit);
    tasm_mediator_ = std::make_shared<
        ::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>();
    manager_ = std::make_unique<lynx::tasm::ElementManager>(
        std::make_unique<MockPaintingContext>(), tasm_mediator_.get(),
        lynx_env_config);

    auto config = std::make_shared<PageConfig>();
    config->SetEnableFiberArch(true);
    manager_->SetConfig(config);
  }

  std::unique_ptr<lynx::tasm::ElementManager> manager_;
  std::shared_ptr<::testing::NiceMock<test::MockTasmDelegate>> tasm_mediator_;
};

TEST_F(TextFragmentBehaviorTest, DispatchLayoutEventContainsSizeAndLineData) {
  auto page = manager_->CreateFiberPage("page", 11);
  auto element = manager_->CreateFiberText("text");
  auto* text_element = static_cast<TextElement*>(element.get());

  TextLineInfoArray line_infos(new TextLineInfo[2]);
  line_infos[0].start = 0;
  line_infos[0].end = 5;
  line_infos[0].ellipsis_count = 0;
  line_infos[1].start = 5;
  line_infos[1].end = 10;
  line_infos[1].ellipsis_count = 1;
  text_element->SetTextLineLayoutInfo(std::move(line_infos), 2);

  auto listener = std::make_shared<RecordingEventListener>();
  ASSERT_TRUE(element->AddEventListener("layout", listener));

  page->InsertNode(element);
  page->FlushActionsAsRoot();

  Fragment fragment(element.get());
  fragment.SetBehavior(std::make_unique<TextFragmentBehavior>(&fragment));

  starlight::LayoutResultForRendering layout_result;
  layout_result.size_ = FloatSize(120.f, 40.f);

  fragment.UpdateLayout(layout_result);
  fragment.UpdateLayout(0.f, 0.f);

  EXPECT_EQ(listener->count(), 1);
  EXPECT_EQ(listener->last_event_type(), "layout");

  const auto& event_detail = listener->last_detail();
  ASSERT_TRUE(event_detail.IsTable());

  BASE_STATIC_STRING_DECL(kDetail, "detail");
  BASE_STATIC_STRING_DECL(kLineCount, "lineCount");
  BASE_STATIC_STRING_DECL(kLines, "lines");
  BASE_STATIC_STRING_DECL(kSize, "size");
  BASE_STATIC_STRING_DECL(kStart, "start");
  BASE_STATIC_STRING_DECL(kEnd, "end");
  BASE_STATIC_STRING_DECL(kEllipsisCount, "ellipsisCount");
  BASE_STATIC_STRING_DECL(kWidth, "width");
  BASE_STATIC_STRING_DECL(kHeight, "height");

  const auto& custom_detail = event_detail.Table()->GetValue(kDetail);
  ASSERT_TRUE(custom_detail.IsTable());

  const auto& line_count = custom_detail.Table()->GetValue(kLineCount);
  EXPECT_EQ(static_cast<int>(line_count.Number()), 2);

  const auto& lines = custom_detail.Table()->GetValue(kLines);
  ASSERT_TRUE(lines.IsArray());
  ASSERT_EQ(lines.Array()->size(), 2u);

  const auto& first_line = lines.Array()->get(0);
  ASSERT_TRUE(first_line.IsTable());
  EXPECT_EQ(static_cast<int>(first_line.Table()->GetValue(kStart).Number()), 0);
  EXPECT_EQ(static_cast<int>(first_line.Table()->GetValue(kEnd).Number()), 5);
  EXPECT_EQ(
      static_cast<int>(first_line.Table()->GetValue(kEllipsisCount).Number()),
      0);

  const auto& second_line = lines.Array()->get(1);
  ASSERT_TRUE(second_line.IsTable());
  EXPECT_EQ(static_cast<int>(second_line.Table()->GetValue(kStart).Number()),
            5);
  EXPECT_EQ(static_cast<int>(second_line.Table()->GetValue(kEnd).Number()), 10);
  EXPECT_EQ(
      static_cast<int>(second_line.Table()->GetValue(kEllipsisCount).Number()),
      1);

  const auto& size = custom_detail.Table()->GetValue(kSize);
  ASSERT_TRUE(size.IsTable());
  EXPECT_FLOAT_EQ(static_cast<float>(size.Table()->GetValue(kWidth).Number()),
                  120.f);
  EXPECT_FLOAT_EQ(static_cast<float>(size.Table()->GetValue(kHeight).Number()),
                  40.f);

  fragment.UpdateLayout(layout_result);
  fragment.UpdateLayout(0.f, 0.f);
  EXPECT_EQ(listener->count(), 2);

  layout_result.size_ = FloatSize(132.f, 44.f);
  fragment.UpdateLayout(layout_result);
  fragment.UpdateLayout(0.f, 0.f);
  EXPECT_EQ(listener->count(), 3);

  text_element->ClearTextLineLayoutInfo();
  layout_result.size_ = FloatSize(144.f, 48.f);
  fragment.UpdateLayout(layout_result);
  fragment.UpdateLayout(0.f, 0.f);
  EXPECT_EQ(listener->count(), 4);

  const auto& cleared_event_detail = listener->last_detail();
  ASSERT_TRUE(cleared_event_detail.IsTable());
  const auto& cleared_custom_detail =
      cleared_event_detail.Table()->GetValue(kDetail);
  ASSERT_TRUE(cleared_custom_detail.IsTable());
  EXPECT_FALSE(cleared_custom_detail.Table()->Contains(kLineCount));
  EXPECT_FALSE(cleared_custom_detail.Table()->Contains(kLines));

  const auto& cleared_size = cleared_custom_detail.Table()->GetValue(kSize);
  ASSERT_TRUE(cleared_size.IsTable());
  EXPECT_FLOAT_EQ(
      static_cast<float>(cleared_size.Table()->GetValue(kWidth).Number()),
      144.f);
  EXPECT_FLOAT_EQ(
      static_cast<float>(cleared_size.Table()->GetValue(kHeight).Number()),
      48.f);
}

}  // namespace

}  // namespace lynx::tasm
