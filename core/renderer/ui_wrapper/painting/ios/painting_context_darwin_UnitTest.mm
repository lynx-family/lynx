// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "core/renderer/ui_wrapper/painting/ios/painting_context_darwin.h"
#import <Lynx/LynxComponentRegistry.h>
#import <Lynx/LynxImageLoadOptions.h>
#import <Lynx/LynxImageLoader.h>
#import <Lynx/LynxPerformanceController.h>
#import <Lynx/LynxTemplateRender.h>
#import <Lynx/LynxTextRenderManager.h>
#import <Lynx/LynxTextRenderer.h>
#import <Lynx/LynxTextUtils.h>
#import <Lynx/LynxUIOwner+Private.h>
#import <Lynx/LynxUIOwner.h>
#import <Lynx/LynxUIText.h>
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>
#import <objc/runtime.h>
#import "LynxTimingConstants.h"
#include "base/include/fml/message_loop.h"
#include "core/event/event.h"
#include "core/event/event_listener.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/element_manager_delegate.h"
#include "core/renderer/dom/fiber/text_element.h"
#include "core/renderer/dom/fragment/fragment.h"
#include "core/renderer/dom/fragment/text_fragment_behavior.h"
#include "core/renderer/ui_wrapper/layout/textra/text_layout_textra.h"
#import "core/renderer/ui_wrapper/painting/ios/native_painting_context_darwin.h"
#import "core/renderer/ui_wrapper/painting/ios/painting_context_darwin_utils.h"
#include "core/renderer/ui_wrapper/painting/painting_context.h"
#include "core/runtime/common/bindings/event/message_event.h"
#include "core/runtime/lepus/bindings/renderer_functions.h"
#include "core/shell/dynamic_ui_operation_queue.h"

namespace {
class TextLayoutTestElement : public lynx::tasm::TextElement {
 public:
  using Element::EnsureSLNode;
  using TextElement::TextElement;
  void MarkLayoutDirtyLite() override {
    ++layout_dirty_count;
    TextElement::MarkLayoutDirtyLite();
  }
  int layout_dirty_count{0};
};

class TextLayoutEventListener : public lynx::event::EventListener {
 public:
  TextLayoutEventListener() : EventListener(Type::kClosureEventListener) {}
  void Invoke(lynx::fml::RefPtr<lynx::event::Event> event) override {
    ++count;
    detail = event->detail();
    on_main_thread = NSThread.isMainThread;
  }
  bool Matches(EventListener* listener) override { return this == listener; }
  int count{0};
  bool on_main_thread{false};
  lynx::lepus::Value detail;
};

class TextLayoutTestNativePaintingContext : public lynx::tasm::NativePaintingCtxDarwin {
 public:
  using NativePaintingCtxDarwin::NativePaintingCtxDarwin;
  void UseTextraLayout() { text_layout_impl_ = std::make_unique<lynx::tasm::TextLayoutTextra>(0); }
};
}  // namespace

namespace lynx::tasm {
namespace {
template <typename Signature>
class TextLayoutWorkletTestDelegate;

template <typename... Args>
class TextLayoutWorkletTestDelegate<EventResult (ElementManagerDelegate::*)(Args...)>
    : public ElementManagerDelegate {
 public:
  EventResult FireElementWorkletAndRequestResolve(Args...) override {
    return static_cast<EventResult>(0);
  }
};

class TextLayoutTestDelegate
    : public ElementManager::Delegate,
      public TextLayoutWorkletTestDelegate<
          decltype(&ElementManagerDelegate::FireElementWorkletAndRequestResolve)> {
 public:
  std::unordered_map<int32_t, LayoutInfoArray> GetSubTreeLayoutInfo(
      int32_t root_id, Viewport viewport = Viewport{}) override {
    return {};
  }
  void CreateLayoutNode(int32_t id, const base::String& tag) override {}
  void UpdateLayoutNodeFontSize(int32_t id, double cur_node_font_size, double root_node_font_size,
                                double font_scale) override {}
  void InsertLayoutNode(int32_t parent_id, int32_t child_id, int index) override {}
  void SendAnimationEvent(const std::string& type, int tag, const lepus::Value& dict) override {}
  void SendNativeCustomEvent(const std::string& name, int tag, const lepus::Value& param_value,
                             const std::string& param_name) override {
    ++count;
    event_name = name;
    target_sign = tag;
    parameter_name = param_name;
    detail = lepus::Value(lepus::Dictionary::Create());
    detail.SetProperty("detail", param_value);
    on_main_thread = NSThread.isMainThread;
  }
  void InsertLayoutNodeBefore(int32_t parent_id, int32_t child_id, int32_t ref_id) override {}
  void RemoveLayoutNode(int32_t parent_id, int32_t child_id) override {}
  void DestroyLayoutNode(int32_t id) override {}
  void UpdateLayoutNodeStyle(int32_t id, CSSPropertyID css_id,
                             const tasm::CSSValue& value) override {}
  void ResetLayoutNodeStyle(int32_t id, CSSPropertyID css_id) override {}
  void UpdateLayoutNodeAttribute(int32_t id, starlight::LayoutAttribute key,
                                 const lepus::Value& value) override {}
  void SetFontFaces(const CSSFontFaceRuleMap& fontfaces) override {}
  void UpdateLayoutNodeByBundle(int32_t id, std::unique_ptr<LayoutBundle> bundle) override {}
  void UpdateLayoutNodeProps(int32_t id, const fml::RefPtr<PropBundle>& props) override {}
  void MarkLayoutDirty(int32_t id) override {}
  void AttachLayoutNodeType(int32_t id, const base::String& tag, bool allow_inline,
                            const fml::RefPtr<PropBundle>& props) override {}
  void UpdateLynxEnvForLayoutThread(LynxEnvConfig env) override {}
  void OnUpdateViewport(float width, int width_mode, float height, int height_mode,
                        bool need_layout) override {}
  void SetRootOnLayout(int32_t id) override {}
  void OnUpdateDataWithoutChange() override {}
  void SetPageConfigForLayoutThread(const std::shared_ptr<PageConfig>& config) override {}
  void OnErrorOccurred(base::LynxError error) override {}
  void BindPipelineIDWithTimingFlag(const tasm::PipelineID& pipeline_id,
                                    const tasm::timing::TimingFlag& timing_flag) override {}
  void ReportElementMemoryInfo(int64_t mem_size_bytes, int element_count) override {}
  void LoadFrameBundle(const std::string& src, FrameElement* element) override {}
  void DidFrameBundleLoaded(const LazyBundleLoader::CallBackInfo& callback_info) override {}
  void OnFrameRemoved(FrameElement* element) override {}
  PipelineContext* GetCurrentPipelineContext() override { return nullptr; }
  PipelineContext* CreateAndUpdateCurrentPipelineContext(
      const std::shared_ptr<PipelineOptions>& pipeline_options, bool is_major_updated) override {
    return nullptr;
  }
  void SendGlobalEvent(const std::string& event, const lepus::Value& info) override {}
  void TriggerLepusGlobalEvent(const std::string& event, const lepus::Value& info) override {}
  event::DispatchEventResult DispatchMessageEvent(
      fml::RefPtr<runtime::MessageEvent> event) override {
    return {};
  }
  bool EnableEventHandleRefactor() const override { return event_handle_refactor; }
  bool SupportComponentJS() const override { return false; }
  runtime::MTSRuntime* GetDefaultEntryRuntime() const override { return nullptr; }
  runtime::MTSRuntime* GetEntryRuntime(const std::string& entry_name) const override {
    return nullptr;
  }
  std::string GetDefaultEntryLogicalName() const override { return {}; }
  void OnLayoutAfter(PipelineLayoutData& data) override {}
  bool event_handle_refactor{false};
  int count{0};
  int target_sign{0};
  std::string event_name;
  std::string parameter_name;
  lepus::Value detail;
  bool on_main_thread{false};
};
}  // namespace
}  // namespace lynx::tasm

@interface ExternalMemoryPaintingTestUIOwner : LynxUIOwner
@property(nonatomic, assign) NSInteger cachedRemovedUICount;
@property(nonatomic, assign) NSInteger nodeRemovedCount;
@end

@implementation ExternalMemoryPaintingTestUIOwner

- (void)cacheRemovedUIId:(NSInteger)removeId {
  (void)removeId;
  self.cachedRemovedUICount += 1;
}

- (void)onNodeRemoved:(NSInteger)sign {
  (void)sign;
  self.nodeRemovedCount += 1;
}

@end

@interface painting_context_darwin_UnitTest : XCTestCase {
  std::unique_ptr<lynx::tasm::PaintingContextDarwin> paintingContext;
}

@end

@implementation painting_context_darwin_UnitTest

- (NSMutableArray<NSDictionary<NSString*, id>*>*)captureImageRequestsWithLoaderMock:(id)loaderMock {
  NSMutableArray<NSDictionary<NSString*, id>*>* requests = [NSMutableArray new];
  OCMStub(ClassMethod([loaderMock sharedInstance])).andReturn(loaderMock);
  void (^captureRequest)(NSInvocation*) = ^(NSInvocation* invocation) {
    __unsafe_unretained LynxImageLoadOptions* options = nil;
    [invocation getArgument:&options atIndex:2];
    [requests addObject:@{
      @"type" : @(options.imageURL.type),
      @"url" : options.imageURL.url.absoluteString,
    }];
    dispatch_block_t cancelBlock = nil;
    [invocation setReturnValue:&cancelBlock];
  };
  OCMStub([loaderMock loadImageWithOptions:[OCMArg any]]).andDo(captureRequest);
  return requests;
}

- (void)setUp {
  LynxUIOwner* uiOwner = [[LynxUIOwner alloc] initWithContainerView:nil
                                                  componentRegistry:nil
                                                      screenMetrics:nil];
  paintingContext = std::make_unique<lynx::tasm::PaintingContextDarwin>(uiOwner);
}

- (void)tearDown {
}

- (void)testScrollBy {
  // This is an example of a functional test case.
  // Use XCTAssert and related functions to verify your tests produce the correct results.
  auto runnable = ^{
    auto res = paintingContext->ScrollBy(1, 20, 20);
    XCTAssertEqual(res[2], 20);
  };
  runnable();
  dispatch_queue_t backgroundQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
  dispatch_async(backgroundQueue, ^{
    runnable();
  });
}

- (void)testExternalMemoryGateOnlyControlsCandidateCaching {
  ExternalMemoryPaintingTestUIOwner* uiOwner =
      [[ExternalMemoryPaintingTestUIOwner alloc] initWithContainerView:nil
                                                     componentRegistry:nil
                                                         screenMetrics:nil];
  lynx::tasm::PaintingContextDarwinRef platformRef(uiOwner);

  platformRef.UpdateNodeReadyPatching({}, {1}, false);
  XCTAssertEqual(uiOwner.cachedRemovedUICount, 0);
  XCTAssertEqual(uiOwner.nodeRemovedCount, 1);

  platformRef.UpdateNodeReadyPatching({}, {2}, true);
  XCTAssertEqual(uiOwner.cachedRemovedUICount, 1);
  XCTAssertEqual(uiOwner.nodeRemovedCount, 2);
}

- (void)testNativePaintingContextMarksPaintEndTiming {
  LynxUIOwner* uiOwner = [[LynxUIOwner alloc] initWithContainerView:nil
                                                  componentRegistry:nil
                                                      screenMetrics:nil];
  auto nativePaintingContext =
      std::make_unique<lynx::tasm::NativePaintingCtxDarwin>(uiOwner, nil, nullptr);
  auto* nativePaintingContextPtr = nativePaintingContext.get();
  auto sharedPaintingContext =
      std::make_unique<lynx::tasm::PaintingContext>(std::move(nativePaintingContext));
  lynx::fml::MessageLoop::EnsureInitializedForCurrentThread();
  auto queue = std::make_shared<lynx::shell::DynamicUIOperationQueue>(
      lynx::base::ThreadStrategyForRendering::ALL_ON_UI,
      lynx::fml::MessageLoop::GetCurrent().GetTaskRunner());
  sharedPaintingContext->SetUIOperationQueue(queue);

  id performanceController = OCMClassMock([LynxPerformanceController class]);
  lynx::tasm::PaintingContextDarwinUtils::SetPerformanceController(
      nativePaintingContextPtr->GetPlatformRef().get(), performanceController);

  auto options = std::make_shared<lynx::tasm::PipelineOptions>();
  options->need_timestamps = true;
  options->has_layout = true;
  NSString* pipelineID = [NSString stringWithUTF8String:options->pipeline_id.c_str()];
  OCMExpect([performanceController markTiming:kTimingPaintEnd pipelineID:pipelineID]);

  sharedPaintingContext->OnFirstScreen();
  sharedPaintingContext->AppendOptionsForTiming(options);
  sharedPaintingContext->FinishLayoutOperation(options);
  queue->ForceFlush();

  OCMVerifyAllWithDelay(performanceController, 1.0);
}

- (void)testNativePaintingContextRequestsSourceOnceWithoutPlaceholder {
  LynxUIOwner* uiOwner = [[LynxUIOwner alloc] initWithContainerView:nil
                                                  componentRegistry:nil
                                                      screenMetrics:nil];
  auto nativePaintingContext =
      std::make_unique<lynx::tasm::NativePaintingCtxDarwin>(uiOwner, nil, nullptr);
  id loaderMock = OCMClassMock([LynxImageLoader class]);
  NSMutableArray<NSDictionary<NSString*, id>*>* requests =
      [self captureImageRequestsWithLoaderMock:loaderMock];

  lynx::tasm::ImagePaintInfo paintInfo;
  auto paintImage = nativePaintingContext->CreateImage(
      1, lynx::base::String("https://example.com/source.png"), paintInfo, 100, 80, 0);

  XCTAssertNotEqual(paintImage, nullptr);
  XCTAssertEqual(requests.count, 1u);
  XCTAssertEqual([requests.firstObject[@"type"] integerValue], LynxImageRequestSrc);
  XCTAssertEqualObjects(requests.firstObject[@"url"], @"https://example.com/source.png");
  [loaderMock stopMocking];
}

- (void)testNativePaintingContextRequestsDistinctPlaceholder {
  LynxUIOwner* uiOwner = [[LynxUIOwner alloc] initWithContainerView:nil
                                                  componentRegistry:nil
                                                      screenMetrics:nil];
  auto nativePaintingContext =
      std::make_unique<lynx::tasm::NativePaintingCtxDarwin>(uiOwner, nil, nullptr);
  id loaderMock = OCMClassMock([LynxImageLoader class]);
  NSMutableArray<NSDictionary<NSString*, id>*>* requests =
      [self captureImageRequestsWithLoaderMock:loaderMock];

  lynx::tasm::ImagePaintInfo paintInfo;
  paintInfo.placeholder = lynx::base::String("https://example.com/placeholder.png");
  auto paintImage = nativePaintingContext->CreateImage(
      1, lynx::base::String("https://example.com/source.png"), paintInfo, 100, 80, 0);

  XCTAssertNotEqual(paintImage, nullptr);
  XCTAssertEqual(requests.count, 2u);
  XCTAssertEqual([requests[0][@"type"] integerValue], LynxImageRequestSrc);
  XCTAssertEqualObjects(requests[0][@"url"], @"https://example.com/source.png");
  XCTAssertEqual([requests[1][@"type"] integerValue], LynxImageRequestPlaceholder);
  XCTAssertEqualObjects(requests[1][@"url"], @"https://example.com/placeholder.png");
  [loaderMock stopMocking];
}

- (void)assertLayoutEvent:(const lynx::lepus::Value&)event
               layoutInfo:(NSDictionary*)info
                    width:(CGFloat)width
                   height:(CGFloat)height {
  const auto detail = event.GetProperty("detail");
  const auto lineCount = detail.GetProperty("lineCount");
  XCTAssertTrue(lineCount.IsNumber());
  XCTAssertEqual(lineCount.Number(), [info[@"lineCount"] intValue]);
  const auto lines = detail.GetProperty("lines");
  XCTAssertTrue(lines.IsArray());
  if (lines.IsArray()) {
    NSArray<NSDictionary*>* expectedLines = info[@"lines"];
    XCTAssertEqual(lines.Array()->size(), expectedLines.count);
    for (NSUInteger i = 0; i < expectedLines.count; ++i) {
      const auto line = lines.Array()->get(i);
      XCTAssertEqual(line.GetProperty("start").Number(), [expectedLines[i][@"start"] intValue]);
      XCTAssertEqual(line.GetProperty("end").Number(), [expectedLines[i][@"end"] intValue]);
      XCTAssertEqual(line.GetProperty("ellipsisCount").Number(),
                     [expectedLines[i][@"ellipsisCount"] intValue]);
    }
  }
  const auto size = detail.GetProperty("size");
  XCTAssertEqualWithAccuracy(size.GetProperty("width").Number(), width, 0.001);
  XCTAssertEqualWithAccuracy(size.GetProperty("height").Number(), height, 0.001);
}

- (void)testPlainTextLayoutEventKeepsFrameInstallation {
  using namespace lynx::tasm;
  TextLayoutTestDelegate delegate;
  delegate.event_handle_refactor = true;
  LynxTextRenderManager* textManager = [LynxTextRenderManager new];
  id owner = OCMClassMock(LynxUIOwner.class);
  OCMStub([owner isLayoutInElementModeOn]).andReturn(YES);
  OCMStub([owner textRenderManager]).andReturn(textManager);
  id context = OCMClassMock(LynxUIContext.class);
  OCMStub([context uiOwner]).andReturn(owner);
  id emitter = OCMClassMock(LynxEventEmitter.class);
  OCMStub([context eventEmitter]).andReturn(emitter);
  OCMReject([emitter sendCustomEvent:[OCMArg any]]);
  auto platform = std::make_unique<PaintingContextDarwin>(owner);
  PageOptions options;
  options.SetEmbeddedMode(EmbeddedMode::LAYOUT_IN_ELEMENT);
  auto manager = std::make_unique<ElementManager>(std::move(platform), &delegate,
                                                  LynxEnvConfig(100, 100, 1, 1), options);
  manager->SetElementManagerDelegate(&delegate);
  lynx::fml::MessageLoop::EnsureInitializedForCurrentThread();
  auto queue = std::make_shared<lynx::shell::DynamicUIOperationQueue>(
      lynx::base::ThreadStrategyForRendering::ALL_ON_UI,
      lynx::fml::MessageLoop::GetCurrent().GetTaskRunner());
  manager->painting_context()->SetUIOperationQueue(queue);
  auto text =
      lynx::fml::AdoptRef<TextLayoutTestElement>(new TextLayoutTestElement(manager.get(), "text"));
  text->MarkAttached();
  text->EnsureSLNode();
  text->SetJSEventHandler("layout", lynx::base::String(), lynx::base::String());
  auto listener = std::make_shared<TextLayoutEventListener>();
  XCTAssertTrue(text->AddEventListener("layout", listener));
  const int sign = text->impl_id();
  LynxAttributedTextBundle* bundle = [LynxAttributedTextBundle new];
  bundle.textStyle = [LynxTextStyle new];
  bundle.maxLineNum = -1;
  bundle.attributedString = [[NSAttributedString alloc]
      initWithString:@"first\nsecond\n"
          attributes:@{NSFontAttributeName : [UIFont systemFontOfSize:20]}];
  [textManager putAttributedTextBundle:sign textBundle:bundle];

  using ComputeLayoutInfo =
      NSDictionary* (*)(id, SEL, LynxTextRenderer*, NSAttributedString*, NSInteger);
  SEL computeSelector = @selector(computeLayoutEventInfoWithRenderer:attributedString:maxLineNum:);
  auto computeInfo = reinterpret_cast<ComputeLayoutInfo>(
      method_getImplementation(class_getClassMethod(LynxTextUtils.class, computeSelector)));
  id utils = OCMClassMock(LynxTextUtils.class);
  __block NSUInteger computed = 0;
  __block NSDictionary* latestInfo = nil;
  OCMStub(ClassMethod([utils computeLayoutEventInfoWithRenderer:[OCMArg any]
                                               attributedString:[OCMArg any]
                                                     maxLineNum:-1]))
      .andDo(^(NSInvocation* invocation) {
        XCTAssertFalse(NSThread.isMainThread);
        __unsafe_unretained LynxTextRenderer* renderer;
        __unsafe_unretained NSAttributedString* attributedString;
        [invocation getArgument:&renderer atIndex:2];
        [invocation getArgument:&attributedString atIndex:3];
        XCTAssertEqualObjects(attributedString, renderer.attrStr);
        latestInfo =
            computeInfo(LynxTextUtils.class, computeSelector, renderer, attributedString, -1);
        ++computed;
        __unsafe_unretained NSDictionary* result = latestInfo;
        [invocation setReturnValue:&result];
      });
  OCMReject([owner onReceiveUIOperation:[OCMArg any] onUI:sign]);
  TextElement* textPtr = text.get();
  dispatch_queue_t layoutQueue = dispatch_queue_create("text-layout-test", DISPATCH_QUEUE_SERIAL);
  XCTestExpectation* completed = [self expectationWithDescription:@"layout completion"];
  dispatch_async(layoutQueue, ^{
    textPtr->Measure(200, LynxMeasureModeDefinite, 1000, LynxMeasureModeIndefinite, false);
    textPtr->Measure(40, LynxMeasureModeDefinite, 1000, LynxMeasureModeIndefinite, true);
    textPtr->UpdateLayoutInfo();
    textPtr->UpdateLayoutInfo();
    [completed fulfill];
  });
  [self waitForExpectations:@[ completed ] timeout:5];
  queue->ForceFlush();
  XCTAssertEqual(computed, 2u);
  XCTAssertEqual(listener->count, 1);
  XCTAssertFalse(listener->on_main_thread);
  [self assertLayoutEvent:listener->detail
               layoutInfo:latestInfo
                    width:[latestInfo[@"size"][@"width"] doubleValue]
                   height:[latestInfo[@"size"][@"height"] doubleValue]];
  LynxTextRenderer* renderer = [textManager takeTextRender:sign];
  XCTAssertEqualWithAccuracy(renderer.layoutSpec.width, 40, 0.001);
  LynxUIText* ui = [[LynxUIText alloc] initWithView:[LynxTextView new]];
  ui.context = context;
  ui.sign = sign;
  [ui setRawEvents:[NSSet setWithObject:@"layout(bindEvent)"] andLepusRawEvents:[NSSet set]];
  for (NSUInteger i = 0; i < 3; ++i) {
    [ui updateFrame:CGRectMake(i, 0, 40, 100)
                withPadding:UIEdgeInsetsZero
                     border:UIEdgeInsetsZero
                     margin:UIEdgeInsetsZero
        withLayoutAnimation:NO];
    XCTAssertEqual(ui.renderer, renderer);
    XCTAssertEqual(ui.view.textRenderer, renderer);
  }
  XCTAssertEqual(computed, 2u);
  XCTAssertEqual(listener->count, 1);

  LynxAttributedTextBundle* newBundle = [LynxAttributedTextBundle new];
  newBundle.textStyle = [LynxTextStyle new];
  newBundle.maxLineNum = -1;
  newBundle.attributedString = [[NSAttributedString alloc]
      initWithString:@"updated text 😀"
          attributes:@{NSFontAttributeName : [UIFont systemFontOfSize:20]}];
  XCTestExpectation* unchanged = [self expectationWithDescription:@"no new measurement"];
  dispatch_async(layoutQueue, ^{
    [textManager putAttributedTextBundle:sign textBundle:newBundle];
    textPtr->UpdateLayoutInfo();
    [unchanged fulfill];
  });
  [self waitForExpectations:@[ unchanged ] timeout:5];
  XCTAssertEqual(computed, 2u);
  XCTAssertEqual(listener->count, 1);
  XCTAssertEqual([textManager takeTextRender:sign], renderer);
  XCTestExpectation* remeasured = [self expectationWithDescription:@"updated text measured"];
  dispatch_async(layoutQueue, ^{
    textPtr->Measure(200, LynxMeasureModeDefinite, 1000, LynxMeasureModeIndefinite, true);
    textPtr->UpdateLayoutInfo();
    textPtr->UpdateLayoutInfo();
    [remeasured fulfill];
  });
  [self waitForExpectations:@[ remeasured ] timeout:5];
  XCTAssertEqual(computed, 3u);
  XCTAssertEqual(listener->count, 2);
  [self assertLayoutEvent:listener->detail
               layoutInfo:latestInfo
                    width:[latestInfo[@"size"][@"width"] doubleValue]
                   height:[latestInfo[@"size"][@"height"] doubleValue]];

  LynxAttributedTextBundle* emptyBundle = [LynxAttributedTextBundle new];
  emptyBundle.textStyle = [LynxTextStyle new];
  emptyBundle.maxLineNum = -1;
  emptyBundle.attributedString = [[NSAttributedString alloc] initWithString:@""];
  XCTestExpectation* emptied = [self expectationWithDescription:@"empty text measured"];
  dispatch_async(layoutQueue, ^{
    [textManager putAttributedTextBundle:sign textBundle:emptyBundle];
    textPtr->Measure(200, LynxMeasureModeDefinite, 1000, LynxMeasureModeIndefinite, true);
    textPtr->UpdateLayoutInfo();
    [emptied fulfill];
  });
  [self waitForExpectations:@[ emptied ] timeout:5];
  XCTAssertEqual(listener->count, 3);
  XCTAssertEqual(text->GetTextLineLayoutCount(), 0);
  [self assertLayoutEvent:listener->detail
               layoutInfo:latestInfo
                    width:[latestInfo[@"size"][@"width"] doubleValue]
                   height:[latestInfo[@"size"][@"height"] doubleValue]];
  text->RemoveEvent("layout", lynx::base::String());
  XCTAssertTrue(text->RemoveEventListener("layout", listener));
  XCTAssertFalse(text->HasLayoutEvent());
  const NSUInteger computedBeforeRemoval = computed;
  XCTestExpectation* removed = [self expectationWithDescription:@"text measured without listener"];
  dispatch_async(layoutQueue, ^{
    textPtr->Measure(200, LynxMeasureModeDefinite, 1000, LynxMeasureModeIndefinite, true);
    textPtr->UpdateLayoutInfo();
    [removed fulfill];
  });
  [self waitForExpectations:@[ removed ] timeout:5];
  XCTAssertEqual(computed, computedBeforeRemoval);
  XCTAssertEqual(listener->count, 3);
  const int dirtyBeforeAddition = text->layout_dirty_count;
  auto addedListener = std::make_shared<TextLayoutEventListener>();
  text->SetJSEventHandler("layout", lynx::base::String(), lynx::base::String());
  XCTAssertTrue(text->AddEventListener("layout", addedListener));
  XCTAssertTrue(text->HasLayoutEvent());
  XCTAssertEqual(text->layout_dirty_count, dirtyBeforeAddition + 1);
  XCTestExpectation* added = [self expectationWithDescription:@"text remeasured for new listener"];
  dispatch_async(layoutQueue, ^{
    textPtr->Measure(200, LynxMeasureModeDefinite, 1000, LynxMeasureModeIndefinite, true);
    textPtr->UpdateLayoutInfo();
    [added fulfill];
  });
  [self waitForExpectations:@[ added ] timeout:5];
  XCTAssertEqual(computed, computedBeforeRemoval + 1);
  XCTAssertEqual(addedListener->count, 1);
  XCTAssertEqual(listener->count, 3);
  text->set_will_destroy(true);
  manager->DestroyText(text.get());
  XCTAssertNotNil([textManager takeTextRender:sign]);
  [textManager releaseTextRender:sign];
  manager->node_manager()->Erase(sign);
  text = nullptr;
  manager.reset();
  [utils stopMocking];
  [emitter stopMocking];
  [context stopMocking];
  [owner stopMocking];
}

- (void)testPlainTextLayoutEventWithLegacyBinding {
  [self verifyPlainTextLayoutWithLegacyBinding:@"bindEvent" expectsEvent:YES];
}

- (void)testTextLayoutEventBindingCache {
  using namespace lynx::tasm;
  LynxUIOwner* owner = [[LynxUIOwner alloc] initWithContainerView:nil
                                                componentRegistry:nil
                                                    screenMetrics:nil
                                                     errorHandler:nil
                                                         uiConfig:nil
                                                     embeddedMode:LynxEmbeddedModeLayoutInElement];
  TextLayoutTestDelegate delegate;
  PageOptions options;
  options.SetEmbeddedMode(EmbeddedMode::LAYOUT_IN_ELEMENT);
  auto manager =
      std::make_unique<ElementManager>(std::make_unique<PaintingContextDarwin>(owner), &delegate,
                                       LynxEnvConfig(100, 100, 1, 1), options);
  auto text =
      lynx::fml::AdoptRef<TextLayoutTestElement>(new TextLayoutTestElement(manager.get(), "text"));
  text->MarkAttached();
  XCTAssertFalse(text->HasLayoutEvent());
  text->SetJSEventHandler("tap", "bindEvent", "onTap");
  XCTAssertFalse(text->HasLayoutEvent());
  XCTAssertEqual(text->layout_dirty_count, 0);
  text->FiberAddEvent("bindEvent", "layout", lynx::lepus::Value("onLayout"), "");
  XCTAssertTrue(text->HasLayoutEvent());
  XCTAssertEqual(text->layout_dirty_count, 1);
  text->FiberAddEvent("bindEvent", "layout", lynx::lepus::Value("onNewLayout"), "");
  XCTAssertEqual(text->layout_dirty_count, 1);

  TextFragmentBehavior::DispatchLayoutEvent(text.get(), 100, 20);
  XCTAssertEqual(delegate.count, 1);
  text->FiberAddEvent("bindEvent", "layout", lynx::lepus::Value(), "");
  XCTAssertFalse(text->HasLayoutEvent());

  text->SetJSEventHandler("layout", "bindEvent", "onLayout");
  text->SetJSEventHandler("layout", "global-bindEvent", "onGlobalLayout");
  text->RemoveEvent("layout", "bindEvent");
  XCTAssertFalse(text->HasLayoutEvent());
  TextFragmentBehavior::DispatchLayoutEvent(text.get(), 100, 20);
  XCTAssertEqual(delegate.count, 1);
  auto globalClone = text->CloneElement(false);
  XCTAssertFalse(static_cast<TextElement*>(globalClone.get())->HasLayoutEvent());
  globalClone->set_will_destroy(true);
  globalClone = nullptr;
  text->RemoveEvent("layout", "global-bindEvent");
  XCTAssertFalse(text->HasLayoutEvent());
  const int dirtyBeforeListener = text->layout_dirty_count;
  auto listener = std::make_shared<TextLayoutEventListener>();
  XCTAssertTrue(text->AddEventListener("layout", listener));
  XCTAssertFalse(text->HasLayoutEvent());
  XCTAssertEqual(text->layout_dirty_count, dirtyBeforeListener);
  TextFragmentBehavior::DispatchLayoutEvent(text.get(), 100, 20);
  XCTAssertEqual(delegate.count, 1);
  text->SetJSEventHandler("layout", lynx::base::String(), lynx::base::String());
  XCTAssertTrue(text->HasLayoutEvent());
  XCTAssertEqual(text->layout_dirty_count, dirtyBeforeListener + 1);
  XCTAssertTrue(text->RemoveEventListener("layout", listener));
  XCTAssertTrue(text->HasLayoutEvent());
  TextFragmentBehavior::DispatchLayoutEvent(text.get(), 100, 20);
  XCTAssertEqual(delegate.count, 2);
  text->RemoveEvent("layout", lynx::base::String());
  XCTAssertFalse(text->HasLayoutEvent());
  text->SetLepusEventHandler("layout", "bindEvent", lynx::lepus::Value(), lynx::lepus::Value());
  XCTAssertTrue(text->HasLayoutEvent());
  text->RemoveAllEvents();
  XCTAssertFalse(text->HasLayoutEvent());
  text->SetWorkletEventHandler("layout", "bindEvent", lynx::lepus::Value(),
                               static_cast<lynx::runtime::MTSRuntime*>(nullptr));
  XCTAssertTrue(text->HasLayoutEvent());
  text->RemoveAllEvents();
  XCTAssertFalse(text->HasLayoutEvent());
  text->FiberAddPiperEvent("bindEvent", "layout", {{"onLayout", lynx::lepus::Value()}});
  XCTAssertTrue(text->HasLayoutEvent());
  auto clone = text->CloneElement(false);
  XCTAssertTrue(static_cast<TextElement*>(clone.get())->HasLayoutEvent());
  clone->set_will_destroy(true);
  clone = nullptr;
  text->RemoveAllEvents();
  XCTAssertFalse(text->HasLayoutEvent());

  XCTAssertTrue(text->AddEventListener("layout", std::make_shared<TextLayoutEventListener>()));
  XCTAssertTrue(text->RemoveEventListeners("layout"));
  XCTAssertFalse(text->HasLayoutEvent());
  text->set_will_destroy(true);
  manager->DestroyText(text.get());
  manager->node_manager()->Erase(text->impl_id());
  text = nullptr;
  manager.reset();
}

- (void)testRemovingAllTextEventsClearsCachedLayoutBinding {
  using namespace lynx::tasm;
  LynxUIOwner* owner = [[LynxUIOwner alloc] initWithContainerView:nil
                                                componentRegistry:nil
                                                    screenMetrics:nil
                                                     errorHandler:nil
                                                         uiConfig:nil
                                                     embeddedMode:LynxEmbeddedModeLayoutInElement];
  TextLayoutTestDelegate delegate;
  PageOptions options;
  options.SetEmbeddedMode(EmbeddedMode::LAYOUT_IN_ELEMENT);
  auto manager =
      std::make_unique<ElementManager>(std::make_unique<PaintingContextDarwin>(owner), &delegate,
                                       LynxEnvConfig(100, 100, 1, 1), options);
  auto text =
      lynx::fml::AdoptRef<TextLayoutTestElement>(new TextLayoutTestElement(manager.get(), "text"));
  text->MarkAttached();
  text->SetJSEventHandler("layout", "bindEvent", "onLayout");
  XCTAssertTrue(text->HasLayoutEvent());
  XCTAssertTrue(text->AddEventListener("layout", std::make_shared<TextLayoutEventListener>()));
  lynx::lepus::Value args[] = {lynx::lepus::Value(text)};
  RendererFunctions::FiberRemoveEventListeners(nullptr, args, 1);
  XCTAssertFalse(text->HasLayoutEvent());
  XCTAssertTrue(text->GetEventListenerMap()->IsEmpty());
  TextFragmentBehavior::DispatchLayoutEvent(text.get(), 100, 20);
  XCTAssertEqual(delegate.count, 0);
  text->set_will_destroy(true);
  manager->DestroyText(text.get());
  manager->node_manager()->Erase(text->impl_id());
  text = nullptr;
  manager.reset();
}

- (void)testPlainTextGlobalLayoutBindingDoesNotPublish {
  [self verifyPlainTextLayoutWithLegacyBinding:@"global-bindEvent" expectsEvent:NO];
}

- (void)verifyPlainTextLayoutWithLegacyBinding:(NSString*)binding expectsEvent:(BOOL)expectsEvent {
  using namespace lynx::tasm;
  LynxUIOwner* owner = [[LynxUIOwner alloc] initWithContainerView:nil
                                                componentRegistry:nil
                                                    screenMetrics:nil
                                                     errorHandler:nil
                                                         uiConfig:nil
                                                     embeddedMode:LynxEmbeddedModeLayoutInElement];
  TextLayoutTestDelegate delegate;
  PageOptions options;
  options.SetEmbeddedMode(EmbeddedMode::LAYOUT_IN_ELEMENT);
  auto manager =
      std::make_unique<ElementManager>(std::make_unique<PaintingContextDarwin>(owner), &delegate,
                                       LynxEnvConfig(100, 100, 1, 1), options);
  auto text =
      lynx::fml::AdoptRef<TextLayoutTestElement>(new TextLayoutTestElement(manager.get(), "text"));
  text->MarkAttached();
  text->EnsureSLNode();
  text->SetJSEventHandler("layout", binding.UTF8String, "onLayout");
  XCTAssertFalse(text->HasEventListener("layout"));
  XCTAssertEqual(text->HasLayoutEvent(), expectsEvent);
  __block NSUInteger computed = 0;
  id utils = nil;
  if (!expectsEvent) {
    utils = OCMClassMock(LynxTextUtils.class);
    OCMStub(ClassMethod([utils computeLayoutEventInfoWithRenderer:[OCMArg any]
                                                 attributedString:[OCMArg any]
                                                       maxLineNum:-1]))
        .andDo(^(NSInvocation* invocation) {
          ++computed;
        });
  }
  LynxAttributedTextBundle* bundle = [LynxAttributedTextBundle new];
  bundle.textStyle = [LynxTextStyle new];
  bundle.maxLineNum = -1;
  bundle.attributedString = [[NSAttributedString alloc]
      initWithString:@"legacy layout event"
          attributes:@{NSFontAttributeName : [UIFont systemFontOfSize:20]}];
  [owner.textRenderManager putAttributedTextBundle:text->impl_id() textBundle:bundle];
  TextElement* textPtr = text.get();
  XCTestExpectation* completed = [self expectationWithDescription:@"legacy layout completion"];
  dispatch_async(dispatch_queue_create("legacy-text-layout-test", DISPATCH_QUEUE_SERIAL), ^{
    textPtr->Measure(100, LynxMeasureModeDefinite, 1000, LynxMeasureModeIndefinite, true);
    textPtr->DispatchLayoutAfter();
    textPtr->DispatchLayoutAfter();
    [completed fulfill];
  });
  [self waitForExpectations:@[ completed ] timeout:5];
  if (expectsEvent) {
    XCTAssertEqual(delegate.count, 1);
    XCTAssertFalse(delegate.on_main_thread);
    XCTAssertEqual(delegate.event_name, "layout");
    XCTAssertEqual(delegate.target_sign, text->impl_id());
    XCTAssertEqual(delegate.parameter_name, "detail");
    LynxTextRenderer* renderer = [owner.textRenderManager takeTextRender:text->impl_id()];
    NSDictionary* info = [LynxTextUtils computeLayoutEventInfoWithRenderer:renderer
                                                          attributedString:renderer.attrStr
                                                                maxLineNum:-1];
    [self assertLayoutEvent:delegate.detail
                 layoutInfo:info
                      width:[info[@"size"][@"width"] doubleValue]
                     height:[info[@"size"][@"height"] doubleValue]];
  } else {
    XCTAssertEqual(delegate.count, 0);
    XCTAssertEqual(computed, 0u);
    XCTAssertEqual(text->GetTextLineLayoutInfo(), nullptr);
    XCTAssertEqual(text->GetTextLineLayoutCount(), 0);
    [utils stopMocking];
  }
  text->set_will_destroy(true);
  manager->DestroyText(text.get());
  manager->node_manager()->Erase(text->impl_id());
  text = nullptr;
  manager.reset();
}

- (void)testFLRTextLayoutEvent {
  [self verifyFLRTextLayoutWithCompatibleUI:NO];
}

- (void)testFLRCompatibleTextLayoutEventKeepsFrameInstallation {
  [self verifyFLRTextLayoutWithCompatibleUI:YES];
}

- (void)testFLRTextraKeepsFragmentLayoutEvents {
  using namespace lynx::tasm;
  TextLayoutTestDelegate delegate;
  delegate.event_handle_refactor = true;
  LynxUIOwner* owner = [[LynxUIOwner alloc] initWithContainerView:nil
                                                componentRegistry:nil
                                                    screenMetrics:nil];
  auto platform = std::make_unique<TextLayoutTestNativePaintingContext>(owner, nil, nullptr);
  platform->UseTextraLayout();
  PageOptions options;
  options.SetEmbeddedMode(EmbeddedMode::FRAGMENT_LAYER_RENDER);
  auto manager = std::make_unique<ElementManager>(std::move(platform), &delegate,
                                                  LynxEnvConfig(100, 100, 1, 1), options);
  manager->SetElementManagerDelegate(&delegate);
  lynx::fml::MessageLoop::EnsureInitializedForCurrentThread();
  auto queue = std::make_shared<lynx::shell::DynamicUIOperationQueue>(
      lynx::base::ThreadStrategyForRendering::ALL_ON_UI,
      lynx::fml::MessageLoop::GetCurrent().GetTaskRunner());
  manager->painting_context()->SetUIOperationQueue(queue);
  auto text =
      lynx::fml::AdoptRef<TextLayoutTestElement>(new TextLayoutTestElement(manager.get(), "text"));
  text->MarkAttached();
  text->EnsureSLNode();
  text->fragment_impl()->CreatePaintingNode(text->TendToFlatten(), nullptr);
  text->SetJSEventHandler("layout", lynx::base::String(), lynx::base::String());
  auto listener = std::make_shared<TextLayoutEventListener>();
  XCTAssertTrue(text->AddEventListener("layout", listener));
  TextLineInfoArray lines = std::make_unique<TextLineInfo[]>(1);
  lines[0] = {0, 5, 1};
  text->SetTextLineLayoutInfo(std::move(lines), 1);
  lynx::starlight::LayoutResultForRendering layout;
  layout.size_ = FloatSize(120, 40);
  text->fragment_impl()->UpdateLayout(layout);
  text->fragment_impl()->UpdateLayout(0, 0);
  XCTAssertEqual(listener->count, 1);
  const auto detail = listener->detail.GetProperty("detail");
  XCTAssertEqual(detail.GetProperty("lineCount").Number(), 1);
  XCTAssertEqual(detail.GetProperty("lines").Array()->get(0).GetProperty("ellipsisCount").Number(),
                 1);
  XCTAssertEqual(detail.GetProperty("size").GetProperty("width").Number(), 120);
  text->set_will_destroy(true);
  manager->DestroyText(text.get());
  manager->node_manager()->Erase(text->impl_id());
  text = nullptr;
  manager.reset();
}

- (void)verifyFLRTextLayoutWithCompatibleUI:(BOOL)compatible {
  using namespace lynx::tasm;
  TextLayoutTestDelegate delegate;
  delegate.event_handle_refactor = true;
  LynxComponentScopeRegistry* registry = [LynxComponentScopeRegistry new];
  [registry registerUI:LynxUIText.class withName:@"text"];
  LynxUIOwner* owner =
      [[LynxUIOwner alloc] initWithContainerView:nil
                               componentRegistry:registry
                                   screenMetrics:nil
                                    errorHandler:nil
                                        uiConfig:nil
                                    embeddedMode:LynxEmbeddedModeFragmentLayerRender];
  LynxTextRenderManager* textManager = owner.textRenderManager;
  id contextMock = OCMPartialMock(owner.uiContext);
  id emitter = OCMClassMock(LynxEventEmitter.class);
  OCMStub([contextMock eventEmitter]).andReturn(emitter);
  OCMReject([emitter sendCustomEvent:[OCMArg any]]);
  PageOptions options;
  options.SetEmbeddedMode(EmbeddedMode::FRAGMENT_LAYER_RENDER);
  auto platform = std::make_unique<NativePaintingCtxDarwin>(owner, registry, nullptr);
  auto manager = std::make_unique<ElementManager>(std::move(platform), &delegate,
                                                  LynxEnvConfig(100, 100, 1, 1), options);
  manager->SetElementManagerDelegate(&delegate);
  lynx::fml::MessageLoop::EnsureInitializedForCurrentThread();
  auto queue = std::make_shared<lynx::shell::DynamicUIOperationQueue>(
      lynx::base::ThreadStrategyForRendering::ALL_ON_UI,
      lynx::fml::MessageLoop::GetCurrent().GetTaskRunner());
  manager->painting_context()->SetUIOperationQueue(queue);
  auto text =
      lynx::fml::AdoptRef<TextLayoutTestElement>(new TextLayoutTestElement(manager.get(), "text"));
  text->MarkAttached();
  text->EnsureSLNode();
  text->MarkAsDirectChildOfCompatibleComponent(compatible);
  Fragment* fragment = text->fragment_impl();
  fragment->CreatePaintingNode(text->TendToFlatten(), nullptr);
  queue->ForceFlush();
  LynxUIText* ui = (LynxUIText*)[owner findUIBySign:text->impl_id()];
  XCTAssertEqual([ui isKindOfClass:LynxUIText.class], compatible);
  [ui setRawEvents:[NSSet setWithObject:@"layout(bindEvent)"] andLepusRawEvents:[NSSet set]];
  text->SetJSEventHandler("layout", lynx::base::String(), lynx::base::String());
  auto listener = std::make_shared<TextLayoutEventListener>();
  XCTAssertTrue(text->AddEventListener("layout", listener));
  LynxAttributedTextBundle* bundle = [LynxAttributedTextBundle new];
  bundle.textStyle = [LynxTextStyle new];
  bundle.maxLineNum = -1;
  bundle.attributedString = [[NSAttributedString alloc]
      initWithString:@"first\nsecond\n"
          attributes:@{NSFontAttributeName : [UIFont systemFontOfSize:20]}];
  [textManager putAttributedTextBundle:text->impl_id() textBundle:bundle];
  using ComputeLayoutInfo =
      NSDictionary* (*)(id, SEL, LynxTextRenderer*, NSAttributedString*, NSInteger);
  SEL computeSelector = @selector(computeLayoutEventInfoWithRenderer:attributedString:maxLineNum:);
  auto computeInfo = reinterpret_cast<ComputeLayoutInfo>(
      method_getImplementation(class_getClassMethod(LynxTextUtils.class, computeSelector)));
  id utils = OCMClassMock(LynxTextUtils.class);
  __block NSUInteger computed = 0;
  __block NSDictionary* latestInfo = nil;
  OCMStub(ClassMethod([utils computeLayoutEventInfoWithRenderer:[OCMArg any]
                                               attributedString:[OCMArg any]
                                                     maxLineNum:-1]))
      .andDo(^(NSInvocation* invocation) {
        XCTAssertFalse(NSThread.isMainThread);
        __unsafe_unretained LynxTextRenderer* renderer;
        __unsafe_unretained NSAttributedString* attributedString;
        [invocation getArgument:&renderer atIndex:2];
        [invocation getArgument:&attributedString atIndex:3];
        latestInfo =
            computeInfo(LynxTextUtils.class, computeSelector, renderer, attributedString, -1);
        ++computed;
        __unsafe_unretained NSDictionary* result = latestInfo;
        [invocation setReturnValue:&result];
      });
  TextElement* textPtr = text.get();
  XCTestExpectation* completed = [self expectationWithDescription:@"FLR layout completion"];
  dispatch_async(dispatch_queue_create("flr-text-layout-test", DISPATCH_QUEUE_SERIAL), ^{
    textPtr->Measure(200, LynxMeasureModeDefinite, 1000, LynxMeasureModeIndefinite, false);
    textPtr->Measure(40, LynxMeasureModeDefinite, 1000, LynxMeasureModeIndefinite, true);
    textPtr->DispatchLayoutAfter();
    textPtr->DispatchLayoutAfter();
    XCTAssertEqual(listener->count, 0);
    lynx::starlight::LayoutResultForRendering layout;
    layout.size_ = FloatSize(120, 80);
    layout.padding_[lynx::starlight::kLeft] = 10;
    layout.padding_[lynx::starlight::kRight] = 20;
    layout.padding_[lynx::starlight::kTop] = 4;
    layout.padding_[lynx::starlight::kBottom] = 6;
    layout.border_[lynx::starlight::kLeft] = 3;
    layout.border_[lynx::starlight::kRight] = 7;
    layout.border_[lynx::starlight::kTop] = 1;
    layout.border_[lynx::starlight::kBottom] = 3;
    fragment->UpdateLayout(layout);
    fragment->UpdateLayout(0, 0);
    [completed fulfill];
  });
  [self waitForExpectations:@[ completed ] timeout:5];
  XCTAssertEqual(computed, 2u);
  XCTAssertEqual(listener->count, 1);
  XCTAssertFalse(listener->on_main_thread);
  [self assertLayoutEvent:listener->detail layoutInfo:latestInfo width:80 height:66];
  LynxTextRenderer* renderer = [textManager takeTextRender:text->impl_id()];
  queue->ForceFlush();
  XCTAssertEqualWithAccuracy(renderer.layoutSpec.width, 40, 0.001);
  for (NSUInteger i = 0; i < 3; ++i) {
    [ui updateFrame:CGRectMake(i, 0, 40, 100)
                withPadding:UIEdgeInsetsZero
                     border:UIEdgeInsetsZero
                     margin:UIEdgeInsetsZero
        withLayoutAnimation:NO];
  }
  if (compatible) {
    XCTAssertEqual(ui.renderer, renderer);
    XCTAssertEqual(ui.view.textRenderer, renderer);
  }
  XCTAssertEqual(computed, 2u);
  XCTAssertEqual(listener->count, 1);
  // FLR keeps its original event cadence for subsequent fragment publications.
  fragment->UpdateLayout(5, 10);
  XCTAssertEqual(listener->count, 2);
  XCTAssertEqual(computed, 2u);
  text->set_will_destroy(true);
  manager->DestroyText(text.get());
  manager->node_manager()->Erase(text->impl_id());
  text = nullptr;
  manager.reset();
  [utils stopMocking];
  [emitter stopMocking];
  [contextMock stopMocking];
}

@end
