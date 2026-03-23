// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/dom/fiber/frame_element.h"

#include <memory>
#include <string>

#include "core/public/page_options.h"
#include "core/renderer/dom/element_manager_delegate.h"
#include "core/renderer/dom/testing/fiber_element_test.h"
#include "core/renderer/dom/testing/fiber_mock_painting_context.h"
#include "core/renderer/pipeline/pipeline_context.h"
#include "core/renderer/template_assembler.h"
#include "core/shell/testing/mock_layout_platform.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {
namespace {

constexpr char kFrameLoaded[] = "frameLoaded";
constexpr char kDetail[] = "detail";
constexpr char kFrameElement[] = "frameElement";
constexpr char kLoad[] = "load";
constexpr char kLoadType[] = "bindEvent";
constexpr char kLoadHandler[] = "onFrameLoad";
constexpr char kSrc[] = "src";
constexpr char kURL[] = "url";
constexpr char kStatusCode[] = "statusCode";
constexpr char kStatusMessage[] = "statusMessage";
constexpr char kFrameURL[] = "https://lynx.dev/frame.js";
constexpr char kFailureMessage[] = "load failed";

class TrackingFiberElementMockTasmDelegate
    : public FiberElementMockTasmDelegate {
 public:
  void SendNativeCustomEvent(const std::string& name, int tag,
                             const lepus::Value& param_value,
                             const std::string& param_name) override {
    ++native_custom_event_count_;
    last_native_custom_event_name_ = name;
    last_native_custom_event_tag_ = tag;
    last_native_custom_event_value_ = param_value;
    last_native_custom_event_param_name_ = param_name;
  }

  void ResetNativeCustomEvent() {
    native_custom_event_count_ = 0;
    last_native_custom_event_name_.clear();
    last_native_custom_event_tag_ = 0;
    last_native_custom_event_value_ = lepus::Value();
    last_native_custom_event_param_name_.clear();
  }

  int native_custom_event_count_{0};
  std::string last_native_custom_event_name_;
  int last_native_custom_event_tag_{0};
  lepus::Value last_native_custom_event_value_;
  std::string last_native_custom_event_param_name_;
};

class FrameElementTest : public ::testing::Test {
 public:
  static void SetUpTestSuite() { base::UIThread::Init(); }

  void SetUp() override {
    LynxEnvConfig lynx_env_config(kWidth, kHeight, kDefaultLayoutsUnitPerPx,
                                  kDefaultPhysicalPixelsPerLayoutUnit);
    vsync_monitor_ = std::make_shared<TestVSyncMonitor>();
    vsync_monitor_->BindToCurrentThread();
    auto unique_manager = std::make_unique<ElementManager>(
        std::make_unique<FiberMockPaintingContext>(), &tasm_mediator_,
        lynx_env_config, PageOptions(), tasm::report::kUnknownInstanceId,
        vsync_monitor_, std::make_unique<test::MockPlatformImpl>());
    manager_ = unique_manager.get();

    tasm_ = std::make_shared<TemplateAssembler>(
        tasm_mediator_, std::move(unique_manager), &tasm_mediator_, 0);

    auto test_entry = std::make_shared<TemplateEntry>();
    tasm_->template_entries_.insert({"test_entry", test_entry});

    auto config = std::make_shared<PageConfig>();
    config->SetEnableZIndex(true);
    config->SetEnableFiberArch(true);
    manager_->SetConfig(config);
    tasm_->page_config_ = config;
    manager_->SetThreadStrategy(base::ThreadStrategyForRendering::ALL_ON_UI);
    manager_->SetEnableParallelElement(false);
    const_cast<DynamicCSSConfigs&>(manager_->GetDynamicCSSConfigs())
        .unify_vw_vh_behavior_ = true;
  }

 protected:
  ElementManager* manager_{nullptr};
  ::testing::NiceMock<TrackingFiberElementMockTasmDelegate> tasm_mediator_;
  std::shared_ptr<TemplateAssembler> tasm_;
  std::shared_ptr<TestVSyncMonitor> vsync_monitor_;
};

class RecordingElementManagerDelegate : public ElementManagerDelegate {
 public:
  void LoadFrameBundle(const std::string& src, FrameElement* element) override {
    load_frame_bundle_src_ = src;
    load_frame_bundle_element_ = element;
  }

  void DidFrameBundleLoaded(
      const LazyBundleLoader::CallBackInfo& callback_info) override {}

  void OnFrameRemoved(FrameElement* element) override {
    removed_frame_ = element;
  }

  PipelineContext* GetCurrentPipelineContext() override { return nullptr; }

  PipelineContext* CreateAndUpdateCurrentPipelineContext(
      const std::shared_ptr<PipelineOptions>& pipeline_options,
      bool is_major_updated) override {
    return nullptr;
  }

  void SendGlobalEvent(const std::string& event,
                       const lepus::Value& info) override {}

  void TriggerLepusGlobalEvent(const std::string& event,
                               const lepus::Value& info) override {
    ++trigger_lepus_global_event_count_;
    last_triggered_event_ = event;
    last_triggered_info_ = info;
  }

  void OnLayoutAfter(PipelineLayoutData& data) override {}

  void ResetTriggeredLepusEvent() {
    trigger_lepus_global_event_count_ = 0;
    last_triggered_event_.clear();
    last_triggered_info_ = lepus::Value();
  }

  int trigger_lepus_global_event_count_{0};
  std::string last_triggered_event_;
  lepus::Value last_triggered_info_;
  std::string load_frame_bundle_src_;
  FrameElement* load_frame_bundle_element_{nullptr};
  FrameElement* removed_frame_{nullptr};
};

class ScopedElementManagerDelegateOverride {
 public:
  ScopedElementManagerDelegateOverride(ElementManager* manager,
                                       ElementManagerDelegate* delegate)
      : manager_(manager),
        original_delegate_(manager->element_manager_delegate()) {
    manager_->SetElementManagerDelegate(delegate);
  }

  ~ScopedElementManagerDelegateOverride() {
    manager_->SetElementManagerDelegate(original_delegate_);
  }

 private:
  ElementManager* manager_;
  ElementManagerDelegate* original_delegate_;
};

std::shared_ptr<FrameElementData> CreateFrameData(
    const std::string& src, int32_t error_code,
    const std::string& error_message, bool with_bundle) {
  auto bundle = with_bundle ? std::make_shared<LynxTemplateBundle>() : nullptr;
  return std::make_shared<FrameElementData>(src, std::move(bundle), error_code,
                                            error_message);
}

void ExpectFrameLoadedPayload(const RecordingElementManagerDelegate& delegate,
                              const FrameElement* frame, const std::string& url,
                              int32_t status_code,
                              const std::string& status_message) {
  ASSERT_EQ(delegate.trigger_lepus_global_event_count_, 1);
  EXPECT_EQ(delegate.last_triggered_event_, kFrameLoaded);
  ASSERT_TRUE(delegate.last_triggered_info_.IsTable());

  const auto payload = delegate.last_triggered_info_.Table();
  const auto detail = payload->GetValue(kDetail);
  ASSERT_TRUE(detail.IsTable());
  EXPECT_EQ(detail.Table()->GetValue(kURL).StdString(), url);
  EXPECT_EQ(
      static_cast<int32_t>(detail.Table()->GetValue(kStatusCode).Number()),
      status_code);
  EXPECT_EQ(detail.Table()->GetValue(kStatusMessage).StdString(),
            status_message);

  const auto frame_element = payload->GetValue(kFrameElement);
  ASSERT_TRUE(frame_element.IsRefCounted());
  const auto frame_ref =
      fml::static_ref_ptr_cast<FiberElement>(frame_element.RefCounted());
  ASSERT_TRUE(frame_ref);
  EXPECT_EQ(frame_ref.get(), static_cast<const FiberElement*>(frame));
}

}  // namespace

TEST_F(FrameElementTest, FrameLoadEventWithBindloadAlsoTriggersFrameLoaded) {
  RecordingElementManagerDelegate delegate;
  ScopedElementManagerDelegateOverride delegate_override(manager_, &delegate);

  auto page = manager_->CreateFiberPage("0", 0);
  manager_->SetFiberPageElement(page);
  auto frame = manager_->CreateFiberFrame();
  frame->data_model()->SetStaticEvent(kLoadType, kLoad, kLoadHandler);
  frame->SetAttribute(kSrc, lepus::Value(kFrameURL));
  delegate.ResetTriggeredLepusEvent();
  tasm_mediator_.ResetNativeCustomEvent();

  page->InsertNode(frame);
  page->FlushActionsAsRoot();

  const auto frame_data =
      CreateFrameData(kFrameURL, error::E_SUCCESS, std::string(), true);

  EXPECT_TRUE(frame->DidBundleLoaded(frame_data));
  EXPECT_EQ(tasm_mediator_.native_custom_event_count_, 1);
  EXPECT_EQ(tasm_mediator_.last_native_custom_event_name_, kLoad);
  EXPECT_EQ(tasm_mediator_.last_native_custom_event_tag_, frame->impl_id());
  EXPECT_EQ(tasm_mediator_.last_native_custom_event_param_name_, kDetail);
  ASSERT_TRUE(tasm_mediator_.last_native_custom_event_value_.IsTable());
  EXPECT_EQ(tasm_mediator_.last_native_custom_event_value_.Table()
                ->GetValue(kURL)
                .StdString(),
            kFrameURL);
  EXPECT_EQ(static_cast<int32_t>(
                tasm_mediator_.last_native_custom_event_value_.Table()
                    ->GetValue(kStatusCode)
                    .Number()),
            error::E_SUCCESS);
  EXPECT_TRUE(tasm_mediator_.last_native_custom_event_value_.Table()
                  ->GetValue(kStatusMessage)
                  .StdString()
                  .empty());
  ExpectFrameLoadedPayload(delegate, frame.get(), kFrameURL, error::E_SUCCESS,
                           std::string());
}

TEST_F(FrameElementTest,
       FrameLoadFailureWithoutBindloadStillTriggersFrameLoaded) {
  RecordingElementManagerDelegate delegate;
  ScopedElementManagerDelegateOverride delegate_override(manager_, &delegate);

  auto page = manager_->CreateFiberPage("0", 0);
  manager_->SetFiberPageElement(page);
  auto frame = manager_->CreateFiberFrame();
  frame->SetAttribute(kSrc, lepus::Value(kFrameURL));
  delegate.ResetTriggeredLepusEvent();
  tasm_mediator_.ResetNativeCustomEvent();

  page->InsertNode(frame);
  page->FlushActionsAsRoot();

  const auto frame_data =
      CreateFrameData(kFrameURL, error::E_LAZY_BUNDLE_LOAD_BAD_RESPONSE,
                      kFailureMessage, false);

  EXPECT_TRUE(frame->DidBundleLoaded(frame_data));
  EXPECT_EQ(tasm_mediator_.native_custom_event_count_, 0);
  ExpectFrameLoadedPayload(delegate, frame.get(), kFrameURL,
                           error::E_LAZY_BUNDLE_LOAD_BAD_RESPONSE,
                           kFailureMessage);
}

TEST_F(FrameElementTest, FrameDeferredLoadTriggersEventsOnceAfterFlush) {
  RecordingElementManagerDelegate delegate;
  ScopedElementManagerDelegateOverride delegate_override(manager_, &delegate);

  auto page = manager_->CreateFiberPage("0", 0);
  manager_->SetFiberPageElement(page);
  auto frame = manager_->CreateFiberFrame();
  frame->data_model()->SetStaticEvent(kLoadType, kLoad, kLoadHandler);
  frame->SetAttribute(kSrc, lepus::Value(kFrameURL));
  delegate.ResetTriggeredLepusEvent();
  tasm_mediator_.ResetNativeCustomEvent();

  const auto frame_data =
      CreateFrameData(kFrameURL, error::E_SUCCESS, std::string(), true);

  EXPECT_TRUE(frame->DidBundleLoaded(frame_data));
  EXPECT_EQ(delegate.trigger_lepus_global_event_count_, 0);
  EXPECT_EQ(tasm_mediator_.native_custom_event_count_, 0);

  page->InsertNode(frame);
  page->FlushActionsAsRoot();

  EXPECT_EQ(tasm_mediator_.native_custom_event_count_, 1);
  EXPECT_EQ(tasm_mediator_.last_native_custom_event_name_, kLoad);
  EXPECT_EQ(tasm_mediator_.last_native_custom_event_tag_, frame->impl_id());
  EXPECT_EQ(tasm_mediator_.last_native_custom_event_param_name_, kDetail);
  ExpectFrameLoadedPayload(delegate, frame.get(), kFrameURL, error::E_SUCCESS,
                           std::string());
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
