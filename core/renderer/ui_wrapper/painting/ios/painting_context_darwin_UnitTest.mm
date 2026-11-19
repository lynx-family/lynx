// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "core/renderer/ui_wrapper/painting/ios/painting_context_darwin.h"
#import <Lynx/LynxImageLoadOptions.h>
#import <Lynx/LynxImageLoader.h>
#import <Lynx/LynxPerformanceController.h>
#import <Lynx/LynxTemplateRender.h>
#import <Lynx/LynxTextRenderer.h>
#import <Lynx/LynxUIOwner+Private.h>
#import <Lynx/LynxUIOwner.h>
#import <Lynx/LynxUIText.h>
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>
#import <objc/runtime.h>
#include <algorithm>
#import "LynxTimingConstants.h"

#include "base/include/fml/message_loop.h"
#include "core/event/event.h"
#include "core/event/event_dispatcher.h"
#include "core/event/event_listener.h"
#include "core/event/touch_event.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/element_manager_delegate.h"
#include "core/renderer/dom/fiber/image_element.h"
#include "core/renderer/dom/fiber/raw_text_element.h"
#include "core/renderer/dom/fiber/text_element.h"
#include "core/renderer/dom/fragment/display_list_builder.h"
#include "core/renderer/dom/fragment/event/platform_event_target_helper.h"
#include "core/renderer/dom/fragment/fragment.h"
#include "core/renderer/dom/fragment/text_fragment_behavior.h"
#include "core/renderer/ui_wrapper/layout/ios/text_layout_darwin.h"
#include "core/renderer/ui_wrapper/layout/textra/text_layout_textra.h"
#import "core/renderer/ui_wrapper/painting/ios/native_painting_context_darwin.h"
#import "core/renderer/ui_wrapper/painting/ios/native_painting_context_platform_darwin_ref.h"
#import "core/renderer/ui_wrapper/painting/ios/painting_context_darwin_utils.h"
#include "core/renderer/ui_wrapper/painting/painting_context.h"
#include "core/shell/dynamic_ui_operation_queue.h"

namespace {
class TextLayoutTestElement : public lynx::tasm::TextElement {
 public:
  using Element::EnsureSLNode;
  using TextElement::SetAttributeInternal;
  using TextElement::TextElement;
  void SetRenderChild(lynx::tasm::Element* child) { first_render_child_ = child; }
  void SetRenderSibling(lynx::tasm::Element* sibling) { next_render_sibling_ = sibling; }
  void MarkLayoutDirtyLite() override {
    ++layout_dirty_count;
    TextElement::MarkLayoutDirtyLite();
  }
  int layout_dirty_count{0};
};

class TextTruncationTestContainer : public lynx::tasm::Element {
 public:
  using Element::Element;
  void SetRenderChild(Element* child) { first_render_child_ = child; }
};

class TextTruncationTestImage : public lynx::tasm::ImageElement {
 public:
  using Element::EnsureSLNode;
  using ImageElement::ImageElement;
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

- (void)testElementTruncationPreservesAuthoredToken {
  using namespace lynx::tasm;
  for (NSString* content in @[ @"more", @"\u2026more" ]) {
    TextLayoutTestDelegate delegate;
    LynxUIOwner* owner =
        [[LynxUIOwner alloc] initWithContainerView:nil
                                 componentRegistry:nil
                                     screenMetrics:nil
                                      errorHandler:nil
                                          uiConfig:nil
                                      embeddedMode:LynxEmbeddedModeLayoutInElement];
    auto platform = std::make_unique<PaintingContextDarwin>(owner);
    PageOptions options;
    options.SetEmbeddedMode(EmbeddedMode::LAYOUT_IN_ELEMENT);
    auto manager = std::make_unique<ElementManager>(std::move(platform), &delegate,
                                                    LynxEnvConfig(100, 100, 1, 1), options);
    manager->SetElementManagerDelegate(&delegate);
    auto text = lynx::fml::AdoptRef(new TextLayoutTestElement(manager.get(), "text"));
    text->content() =
        lynx::base::String("A long paragraph that must use its custom truncation token.");
    text->EnsureSLNode();
    text->SetAttributeInternal(lynx::base::String("text-maxline"), lynx::lepus::Value(1));
    auto token =
        lynx::fml::AdoptRef(new TextTruncationTestContainer(manager.get(), "inline-truncation"));
    auto label = lynx::fml::AdoptRef(new TextLayoutTestElement(manager.get(), "inline-text"));
    label->MarkAsInline();
    XCTAssertTrue(label->AddEventListener("tap", std::make_shared<TextLayoutEventListener>()));
    label->content() = lynx::base::String(content.UTF8String);
    token->SetRenderChild(label.get());
    text->SetRenderChild(token.get());
    TextLayoutDarwin layout(owner.textRenderManager, owner.fontFaceContext);
    layout.DispatchLayoutBefore(text.get());
    layout.Measure(text.get(), 100, 1, 0, 0);
    LynxTextRenderer* renderer = [owner.textRenderManager takeTextRender:text->impl_id()];
    XCTAssertEqualObjects(renderer.attrStr.string,
                          @"A long paragraph that must use its custom truncation token.");
    XCTAssertEqualObjects(renderer.truncationToken.string, content);
    XCTAssertTrue([renderer.textStorage.string hasSuffix:renderer.truncationToken.string]);
    NSNumber* target = [renderer.textStorage attribute:@"LynxInlineTextEventTargetSignKey"
                                               atIndex:renderer.textStorage.length - 1
                                        effectiveRange:nil];
    XCTAssertEqual(target.intValue, label->impl_id());

    LynxUIText* ui = [LynxUIText new];
    ui.context = owner.uiContext;
    ui.sign = text->impl_id();
    [ui onReceiveUIOperation:renderer];
    NSLayoutManager* layoutManager = renderer.layoutManager;
    NSUInteger glyph =
        [layoutManager glyphIndexForCharacterAtIndex:renderer.textStorage.length - 1];
    CGRect bounds =
        [layoutManager boundingRectForGlyphRange:NSMakeRange(glyph, 1)
                                 inTextContainer:layoutManager.textContainers.firstObject];
    CGPoint point =
        CGPointMake(CGRectGetMidX(bounds) + renderer.textContentOffsetX, CGRectGetMidY(bounds));
    XCTAssertEqual([ui hitTest:point withEvent:nil].signature, label->impl_id());

    // A later short value must not display the token as ordinary body content.
    text->content() = lynx::base::String("short");
    layout.DispatchLayoutBefore(text.get());
    layout.Measure(text.get(), 100, 1, 0, 0);
    renderer = [owner.textRenderManager takeTextRender:text->impl_id()];
    XCTAssertEqualObjects(renderer.textStorage.string, @"short");
  }
}

- (void)testTruncationRawTextUsesComputedFont {
  using namespace lynx::tasm;
  using lynx::starlight::FontStyleType;
  using lynx::starlight::FontWeightType;
  TextLayoutTestDelegate delegate;
  LynxUIOwner* owner = [[LynxUIOwner alloc] initWithContainerView:nil
                                                componentRegistry:nil
                                                    screenMetrics:nil
                                                     errorHandler:nil
                                                         uiConfig:nil
                                                     embeddedMode:LynxEmbeddedModeLayoutInElement];
  PageOptions options;
  options.SetEmbeddedMode(EmbeddedMode::LAYOUT_IN_ELEMENT);
  auto manager =
      std::make_unique<ElementManager>(std::make_unique<PaintingContextDarwin>(owner), &delegate,
                                       LynxEnvConfig(100, 100, 1, 1), options);
  manager->SetElementManagerDelegate(&delegate);
  auto text = lynx::fml::AdoptRef(new TextLayoutTestElement(manager.get(), "text"));
  text->EnsureSLNode();
  text->content() = lynx::base::String("A long paragraph that must be truncated.");
  text->SetAttributeInternal(lynx::base::String("text-maxline"), lynx::lepus::Value(1));
  text->ResolveStyleValue(kPropertyIDFontFamily,
                          CSSValue("Georgia", CSSValuePattern::STRING, CSSValueType::DEFAULT));
  text->ResolveStyleValue(kPropertyIDFontStyle, CSSValue(FontStyleType::kItalic));
  text->ResolveStyleValue(kPropertyIDFontWeight, CSSValue(FontWeightType::kBold));
  auto token =
      lynx::fml::AdoptRef(new TextTruncationTestContainer(manager.get(), "inline-truncation"));
  auto label = manager->CreateFiberRawText();
  label->SetText(lynx::lepus::Value("More"));
  text->SetRenderChild(token.get());
  token->SetRenderChild(label.get());
  auto* tokenStyle = token->computed_css_style();
  tokenStyle->InheritNormalPropertiesFrom(
      *text->computed_css_style(),
      {kPropertyIDFontFamily, kPropertyIDFontStyle, kPropertyIDFontWeight});
  TextLayoutDarwin layout(owner.textRenderManager, owner.fontFaceContext);
  for (auto weight : {FontWeightType::kBold, FontWeightType::kNormal}) {
    if (weight == FontWeightType::kNormal) {
      tokenStyle->SetValue(kPropertyIDFontWeight, CSSValue(weight));
    }
    layout.DispatchLayoutBefore(text.get());
    layout.Measure(text.get(), 100, 1, 0, 0);
    LynxTextRenderer* renderer = [owner.textRenderManager takeTextRender:text->impl_id()];
    XCTAssertTrue([renderer.textStorage.string hasSuffix:@"More"]);
    UIFont* font = [renderer.textStorage attribute:NSFontAttributeName
                                           atIndex:renderer.textStorage.length - 1
                                    effectiveRange:nil];
    XCTAssertEqualObjects(
        font.fontName, weight == FontWeightType::kBold ? @"Georgia-BoldItalic" : @"Georgia-Italic");
  }
}

- (void)testFLRTruncationPreservesNestedTapTargetsAcrossWidths {
  using namespace lynx::tasm;
  TextLayoutTestDelegate delegate;
  LynxUIOwner* owner =
      [[LynxUIOwner alloc] initWithContainerView:nil
                               componentRegistry:nil
                                   screenMetrics:nil
                                    errorHandler:nil
                                        uiConfig:nil
                                    embeddedMode:LynxEmbeddedModeFragmentLayerRender];
  auto platform = std::make_unique<NativePaintingCtxDarwin>(owner, nil, nullptr);
  auto ref =
      std::static_pointer_cast<NativePaintingCtxPlatformDarwinRef>(platform->GetPlatformRef());
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

  const int pageId = manager->GenerateElementID();
  XCTAssertEqual(pageId, kRootId);
  ref->CreatePlatformRenderer(pageId, PlatformRendererType::kPage, nullptr);
  auto text = lynx::fml::AdoptRef(new TextLayoutTestElement(manager.get(), "text"));
  text->MarkAttached();
  text->EnsureSLNode();
  text->SetAttributeInternal(lynx::base::String("text-maxline"), lynx::lepus::Value(1));
  auto listener = std::make_shared<TextLayoutEventListener>();
  auto makeInline = [&](Element* parent) {
    auto node = lynx::fml::AdoptRef(new TextLayoutTestElement(manager.get(), "inline-text"));
    node->MarkAttached();
    node->MarkAsInline();
    node->set_parent(parent);
    node->AddEventListener("tap", listener);
    return node;
  };
  auto outer = makeInline(text.get());
  auto inner = makeInline(outer.get());
  auto marker =
      lynx::fml::AdoptRef(new TextTruncationTestContainer(manager.get(), "inline-truncation"));
  auto label = makeInline(marker.get());
  marker->MarkAttached();
  marker->set_parent(outer.get());
  text->SetRenderChild(outer.get());
  outer->SetRenderChild(inner.get());
  inner->SetRenderSibling(marker.get());
  inner->content() = lynx::base::String("These words belong entirely to the inner target.");
  marker->SetRenderChild(label.get());
  label->content() = lynx::base::String("More");

  DisplayListBuilder builder;
  builder.Begin(kRootId, PlatformRendererType::kPage, 0, 0, 1000, 100)
      .Begin(text->impl_id(), PlatformRendererType::kText, 0, 0, 1000, 100)
      .End()
      .End();
  ref->UpdateDisplayList(kRootId, builder.Build());
  TextLayoutDarwin layout(owner.textRenderManager, owner.fontFaceContext);
  layout.DispatchLayoutBefore(text.get());
  for (int width : {1000, 100, 1000}) {
    layout.Measure(text.get(), width, 1, 0, 0);
    queue->ForceFlush();
    auto root = ref->ReconstructEventTargetTreeRecursively();
    XCTAssertTrue(root);
    auto* helper = ref->GetEventTargetHelper();
    XCTAssertTrue(helper->GetEventTarget(outer->impl_id()));
    XCTAssertEqual(helper->GetEventTarget(label->impl_id()) != nullptr, width == 100);
    auto* expected = width == 100 ? label.get() : inner.get();
    const auto regions = ref->GetTextEventTargetRegions(text->impl_id());
    auto region = std::find_if(regions.begin(), regions.end(),
                               [&](const auto& r) { return r.sign == expected->impl_id(); });
    XCTAssertTrue(region != regions.end());
    if (!root || region == regions.end()) return;
    float point[] = {region->left + region->width / 2, region->top + region->height / 2};
    auto hit = root->HitTest(point);
    XCTAssertTrue(hit);
    if (!hit) return;
    XCTAssertEqual(hit->Sign(), expected->impl_id());
    const int before = listener->count;
    lynx::event::EventDispatcher::DispatchEvent(
        *expected, lynx::fml::MakeRefCounted<lynx::event::TouchEvent>("tap"));
    XCTAssertEqual(listener->count - before, 2);
  }
}

- (void)testElementTruncationMeasuresItsImageAndReusesNodeMeasurement {
  using namespace lynx::tasm;
  TextLayoutTestDelegate delegate;
  LynxUIOwner* owner = [[LynxUIOwner alloc] initWithContainerView:nil
                                                componentRegistry:nil
                                                    screenMetrics:nil
                                                     errorHandler:nil
                                                         uiConfig:nil
                                                     embeddedMode:LynxEmbeddedModeLayoutInElement];
  auto platform = std::make_unique<PaintingContextDarwin>(owner);
  PageOptions options;
  options.SetEmbeddedMode(EmbeddedMode::LAYOUT_IN_ELEMENT);
  auto manager = std::make_unique<ElementManager>(std::move(platform), &delegate,
                                                  LynxEnvConfig(100, 100, 1, 1), options);
  manager->SetElementManagerDelegate(&delegate);
  auto text = lynx::fml::AdoptRef(new TextLayoutTestElement(manager.get(), "text"));
  text->content() = lynx::base::String("A long paragraph that must be truncated beside an image.");
  text->EnsureSLNode();
  text->SetAttributeInternal(lynx::base::String("text-maxline"), lynx::lepus::Value(1));
  auto token =
      lynx::fml::AdoptRef(new TextTruncationTestContainer(manager.get(), "inline-truncation"));
  auto label = lynx::fml::AdoptRef(new TextLayoutTestElement(manager.get(), "inline-text"));
  label->content() = lynx::base::String("\u2026more");
  auto image = lynx::fml::AdoptRef(new TextTruncationTestImage(manager.get(), "image"));
  image->EnsureSLNode();
  int measures = 0;
  image->SetMeasureFunc(&measures, [](void* context, const lynx::starlight::Constraints&, bool) {
    ++*static_cast<int*>(context);
    return FloatSize(9, 9, 9);
  });
  label->SetRenderSibling(image.get());
  token->SetRenderChild(label.get());
  text->SetRenderChild(token.get());
  text->slnode()->AppendChild(image->slnode());
  text->DispatchLayoutBefore();
  lynx::starlight::Constraints constraints;
  constraints[lynx::starlight::kHorizontal] = lynx::starlight::OneSideConstraint::Definite(100);
  constraints[lynx::starlight::kVertical] = lynx::starlight::OneSideConstraint::Indefinite();
  text->slnode()->UpdateMeasure(constraints, false);
  LynxTextRenderer* first = [owner.textRenderManager takeTextRender:text->impl_id()];
  for (int i = 0; i < 3; ++i) text->slnode()->UpdateMeasure(constraints, true);
  XCTAssertEqual(measures, 1);
  XCTAssertEqual(first, [owner.textRenderManager takeTextRender:text->impl_id()]);
  XCTAssertTrue(text->slnode()->IsInflowSubTreeInSyncWithLastMeasurement());
  XCTAssertTrue([first.textStorage.string hasSuffix:@"\u2026more\ufffc"]);
  text->Align();
  XCTAssertEqual(image->slnode()->GetBorderBoundWidth(), 9);
  text->slnode()->MarkUpdated();

  text->content() = lynx::base::String("short");
  text->slnode()->MarkDirty();
  text->DispatchLayoutBefore();
  text->slnode()->UpdateMeasure(constraints, true);
  text->Align();
  XCTAssertEqual(image->slnode()->GetBorderBoundWidth(), 0);
  text->slnode()->MarkUpdated();

  text->content() = lynx::base::String("A long paragraph that must be truncated beside an image.");
  text->slnode()->MarkDirty();
  text->DispatchLayoutBefore();
  text->slnode()->UpdateMeasure(constraints, true);
  text->Align();
  XCTAssertEqual(image->slnode()->GetBorderBoundWidth(), 9);
  text->slnode()->RemoveChild(image->slnode());
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

@end
