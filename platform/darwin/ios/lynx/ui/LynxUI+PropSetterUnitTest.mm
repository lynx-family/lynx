// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#import <Foundation/Foundation.h>
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>
#import <objc/runtime.h>

#import <Lynx/LynxCSSType.h>
#import <Lynx/LynxEnv.h>
#import <Lynx/LynxListScrollEventEmitter.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxUI.h>
#import <Lynx/LynxUICollection+Internal.h>
#import <Lynx/LynxUICollection.h>
#import <Lynx/LynxUIOwner.h>
#import <Lynx/LynxView+Internal.h>
#import <Lynx/LynxView.h>
#import <Lynx/UIView+Lynx.h>
#import "LynxTemplateRender+Internal.h"
#import "LynxUI+Private.h"

#include <stdio.h>
#include <memory>
#include "core/renderer/ui_wrapper/common/ios/prop_bundle_darwin.h"
#include "core/renderer/ui_wrapper/painting/ios/painting_context_darwin.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/shell/lynx_shell.h"

@interface LynxUI (LynxUT)

@property(nonatomic) NSNumber *resetContentOffsetCount;
@property(nonatomic, strong) NSMutableArray<NSNumber *> *applyRTLArray;

@end

@implementation LynxUI (LynxUT)

@dynamic resetContentOffsetCount;
static const void *resetContentOffsetCountKey = &resetContentOffsetCountKey;

@dynamic applyRTLArray;
static const void *applyRTLArrayKey = &applyRTLArrayKey;

+ (void)swizzleOriginalSelector:(SEL)originalSelector withSwizzledSelector:(SEL)swizzledSelector {
  Method originalMethod = class_getInstanceMethod(self, originalSelector);
  Method swizzledMethod = class_getInstanceMethod(self, swizzledSelector);
  BOOL didAddMethod =
      class_addMethod(self, originalSelector, method_getImplementation(swizzledMethod),
                      method_getTypeEncoding(swizzledMethod));

  if (didAddMethod) {
    class_replaceMethod(self, swizzledSelector, method_getImplementation(originalMethod),
                        method_getTypeEncoding(originalMethod));
  } else {
    method_exchangeImplementations(originalMethod, swizzledMethod);
  }
}

+ (void)load {
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    [self swizzleOriginalSelector:NSSelectorFromString(@"resetContentOffset")
             withSwizzledSelector:@selector(swizzled_resetContentOffset)];
    [self swizzleOriginalSelector:NSSelectorFromString(@"applyRTL:")
             withSwizzledSelector:@selector(swizzled_applyRTL:)];
  });
}

- (void)setResetContentOffsetCount:(NSNumber *)resetContentOffsetCount {
  objc_setAssociatedObject(self, resetContentOffsetCountKey, resetContentOffsetCount,
                           OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

- (NSNumber *)resetContentOffsetCount {
  NSNumber *value = objc_getAssociatedObject(self, resetContentOffsetCountKey);
  if (!value) {
    value = @0;
    objc_setAssociatedObject(self, resetContentOffsetCountKey, value,
                             OBJC_ASSOCIATION_RETAIN_NONATOMIC);
  }
  return objc_getAssociatedObject(self, resetContentOffsetCountKey);
}

- (void)swizzled_resetContentOffset {
  self.resetContentOffsetCount = @([self.resetContentOffsetCount integerValue] + 1);
  return [self swizzled_resetContentOffset];
}

- (void)setApplyRTLArray:(NSMutableArray *)applyRTLArray {
  objc_setAssociatedObject(self, applyRTLArrayKey, applyRTLArray,
                           OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

- (NSMutableArray *)applyRTLArray {
  NSMutableArray *value = objc_getAssociatedObject(self, applyRTLArrayKey);
  if (!value) {
    value = [NSMutableArray new];
    objc_setAssociatedObject(self, applyRTLArrayKey, value, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
  }
  return objc_getAssociatedObject(self, applyRTLArrayKey);
}

- (void)swizzled_applyRTL:(BOOL)rtl {
  [self.applyRTLArray addObject:@(rtl)];
  return [self swizzled_applyRTL:rtl];
}

@end

@interface LynxUI_PropSetterUnitTest : XCTestCase

@end

@implementation LynxUI_PropSetterUnitTest {
  LynxView *_lynxView;
  LynxUIOwner *_uiOwner;
  std::unique_ptr<lynx::tasm::PaintingContext> sync_painting_context_darwin_;
  std::unique_ptr<lynx::tasm::PaintingContext> async_painting_context_darwin_;
  std::shared_ptr<lynx::tasm::PropBundleCreatorDarwin> prop_bundle_creator_;
}

- (void)setUp {
  _lynxView = [[LynxView alloc] init];

  LynxScreenMetrics *screenMetrics =
      [[LynxScreenMetrics alloc] initWithScreenSize:[UIScreen mainScreen].bounds.size
                                              scale:[UIScreen mainScreen].scale];

  _uiOwner = [[LynxUIOwner alloc] initWithContainerView:_lynxView
                                         templateRender:_lynxView.templateRender
                                      componentRegistry:[LynxComponentScopeRegistry new]
                                          screenMetrics:screenMetrics];

  sync_painting_context_darwin_ = std::make_unique<lynx::tasm::PaintingContext>(
      std::make_unique<lynx::tasm::PaintingContextDarwin>(_uiOwner, false));
  async_painting_context_darwin_ = std::make_unique<lynx::tasm::PaintingContext>(
      std::make_unique<lynx::tasm::PaintingContextDarwin>(_uiOwner, true));

  prop_bundle_creator_ = std::make_shared<lynx::tasm::PropBundleCreatorDarwin>();

  unsigned int count = 0;
  Ivar *ivars = class_copyIvarList([LynxTemplateRender class], &count);

  for (unsigned int i = 0; i < count; i++) {
    Ivar var = ivars[i];
    const char *name = ivar_getName(var);

    if (strcmp(name, "shell_") == 0) {
      void *shellPtrRaw = (__bridge void *)(object_getIvar(_lynxView.templateRender, var));
      lynx::shell::LynxShell *shell = reinterpret_cast<lynx::shell::LynxShell *>(shellPtrRaw);

      if (shell) {
        sync_painting_context_darwin_->SetUIOperationQueue(shell->ui_operation_queue_);
        async_painting_context_darwin_->SetUIOperationQueue(shell->ui_operation_queue_);
      }
    }
  }

  free(ivars);
}

- (void)tearDown {
}

- (void)testSetOpacity {
  std::shared_ptr<lynx::tasm::PropBundle> prop_bundle = prop_bundle_creator_->CreatePropBundle();
  prop_bundle->SetProps("opacity", 0.5);

  async_painting_context_darwin_->CreatePaintingNode(12, "view", prop_bundle, false, false, 12);
  sync_painting_context_darwin_->CreatePaintingNode(11, "view", prop_bundle, false, false, 11);

  sync_painting_context_darwin_->OnNodeReady(11);
  async_painting_context_darwin_->OnNodeReady(12);

  sync_painting_context_darwin_->UpdateNodeReadyPatching();
  async_painting_context_darwin_->UpdateNodeReadyPatching();

  static_cast<lynx::tasm::PaintingContextDarwin *>(sync_painting_context_darwin_->impl())
      ->ForceFlush();
  static_cast<lynx::tasm::PaintingContextDarwin *>(async_painting_context_darwin_->impl())
      ->ForceFlush();

  LynxUI *sync_ui = [_uiOwner findUIBySign:11];
  LynxUI *async_ui = [_uiOwner findUIBySign:12];

  XCTAssert(sync_ui.view.layer.opacity == async_ui.view.layer.opacity);
  XCTAssert(sync_ui.view.layer.opacity == 0.5);
  XCTAssert(async_ui.view.layer.opacity == 0.5);
}

- (void)testSetVisibility {
  std::shared_ptr<lynx::tasm::PropBundle> prop_bundle = prop_bundle_creator_->CreatePropBundle();
  prop_bundle->SetProps("visibility", false);

  async_painting_context_darwin_->CreatePaintingNode(12, "view", prop_bundle, false, false, 12);
  sync_painting_context_darwin_->CreatePaintingNode(11, "view", prop_bundle, false, false, 11);

  sync_painting_context_darwin_->OnNodeReady(11);
  async_painting_context_darwin_->OnNodeReady(12);

  sync_painting_context_darwin_->UpdateNodeReadyPatching();
  async_painting_context_darwin_->UpdateNodeReadyPatching();

  static_cast<lynx::tasm::PaintingContextDarwin *>(sync_painting_context_darwin_->impl())
      ->ForceFlush();
  static_cast<lynx::tasm::PaintingContextDarwin *>(async_painting_context_darwin_->impl())
      ->ForceFlush();

  LynxUI *sync_ui = [_uiOwner findUIBySign:11];
  LynxUI *async_ui = [_uiOwner findUIBySign:12];

  XCTAssert(sync_ui.view.hidden == YES);
  XCTAssert(async_ui.view.hidden == YES);
}

- (void)testSetDirection {
  std::shared_ptr<lynx::tasm::PropBundle> prop_bundle = prop_bundle_creator_->CreatePropBundle();
  prop_bundle->SetProps("direction", 2);

  async_painting_context_darwin_->CreatePaintingNode(12, "view", prop_bundle, false, false, 12);
  sync_painting_context_darwin_->CreatePaintingNode(11, "view", prop_bundle, false, false, 11);

  sync_painting_context_darwin_->OnNodeReady(11);
  async_painting_context_darwin_->OnNodeReady(12);

  sync_painting_context_darwin_->UpdateNodeReadyPatching();
  async_painting_context_darwin_->UpdateNodeReadyPatching();

  static_cast<lynx::tasm::PaintingContextDarwin *>(sync_painting_context_darwin_->impl())
      ->ForceFlush();
  static_cast<lynx::tasm::PaintingContextDarwin *>(async_painting_context_darwin_->impl())
      ->ForceFlush();

  LynxUI *sync_ui = [_uiOwner findUIBySign:11];
  LynxUI *async_ui = [_uiOwner findUIBySign:12];

  XCTAssert([sync_ui.resetContentOffsetCount isEqual:@1]);
  XCTAssert([sync_ui.applyRTLArray[0] isEqual:@1]);

  XCTAssert([async_ui.resetContentOffsetCount isEqual:@1]);
  XCTAssert([async_ui.applyRTLArray[0] isEqual:@1]);
}

- (void)testSetAccessibilityLabel {
  std::shared_ptr<lynx::tasm::PropBundle> prop_bundle = prop_bundle_creator_->CreatePropBundle();
  prop_bundle->SetProps("accessibility-label", "test");

  async_painting_context_darwin_->CreatePaintingNode(12, "view", prop_bundle, false, false, 12);
  sync_painting_context_darwin_->CreatePaintingNode(11, "view", prop_bundle, false, false, 11);

  sync_painting_context_darwin_->OnNodeReady(11);
  async_painting_context_darwin_->OnNodeReady(12);

  sync_painting_context_darwin_->UpdateNodeReadyPatching();
  async_painting_context_darwin_->UpdateNodeReadyPatching();

  static_cast<lynx::tasm::PaintingContextDarwin *>(sync_painting_context_darwin_->impl())
      ->ForceFlush();
  static_cast<lynx::tasm::PaintingContextDarwin *>(async_painting_context_darwin_->impl())
      ->ForceFlush();

  LynxUI *sync_ui = [_uiOwner findUIBySign:11];
  LynxUI *async_ui = [_uiOwner findUIBySign:12];

  XCTAssert([sync_ui.view.accessibilityLabel isEqual:@"test"]);
  XCTAssert([async_ui.view.accessibilityLabel isEqual:@"test"]);
}

- (void)testOtherProps {
  std::shared_ptr<lynx::tasm::PropBundle> prop_bundle = prop_bundle_creator_->CreatePropBundle();
  prop_bundle->SetProps("native-interaction-enabled", false);
  prop_bundle->SetProps("allow-edge-antialiasing", false);

  async_painting_context_darwin_->CreatePaintingNode(12, "view", prop_bundle, false, false, 12);
  sync_painting_context_darwin_->CreatePaintingNode(11, "view", prop_bundle, false, false, 11);

  sync_painting_context_darwin_->OnNodeReady(11);
  async_painting_context_darwin_->OnNodeReady(12);

  sync_painting_context_darwin_->UpdateNodeReadyPatching();
  async_painting_context_darwin_->UpdateNodeReadyPatching();

  static_cast<lynx::tasm::PaintingContextDarwin *>(sync_painting_context_darwin_->impl())
      ->ForceFlush();
  static_cast<lynx::tasm::PaintingContextDarwin *>(async_painting_context_darwin_->impl())
      ->ForceFlush();

  LynxUI *sync_ui = [_uiOwner findUIBySign:11];
  LynxUI *async_ui = [_uiOwner findUIBySign:12];

  XCTAssert(sync_ui.view.userInteractionEnabled == NO);
  XCTAssert(sync_ui.view.layer.allowsEdgeAntialiasing == NO);

  XCTAssert(async_ui.view.userInteractionEnabled == NO);
  XCTAssert(async_ui.view.layer.allowsEdgeAntialiasing == NO);
}

- (void)testTagName {
  std::shared_ptr<lynx::tasm::PropBundle> prop_bundle = prop_bundle_creator_->CreatePropBundle();
  prop_bundle->SetProps("opacity", 0.5);

  async_painting_context_darwin_->CreatePaintingNode(12, "scroll-view", prop_bundle, false, false,
                                                     12);
  sync_painting_context_darwin_->CreatePaintingNode(11, "scroll-view", prop_bundle, false, false,
                                                    11);

  sync_painting_context_darwin_->OnNodeReady(11);
  async_painting_context_darwin_->OnNodeReady(12);

  sync_painting_context_darwin_->UpdateNodeReadyPatching();
  async_painting_context_darwin_->UpdateNodeReadyPatching();

  static_cast<lynx::tasm::PaintingContextDarwin *>(sync_painting_context_darwin_->impl())
      ->ForceFlush();
  static_cast<lynx::tasm::PaintingContextDarwin *>(async_painting_context_darwin_->impl())
      ->ForceFlush();

  LynxUI *sync_ui = [_uiOwner findUIBySign:11];
  LynxUI *async_ui = [_uiOwner findUIBySign:12];

  XCTAssert(sync_ui.view.layer.opacity == async_ui.view.layer.opacity);
  XCTAssert([sync_ui.tagName isEqualToString:async_ui.tagName]);
  XCTAssert([sync_ui.tagName isEqualToString:@"scroll-view"]);
  XCTAssert([async_ui.tagName isEqualToString:@"scroll-view"]);
}

- (void)testViewSign {
  std::shared_ptr<lynx::tasm::PropBundle> prop_bundle = prop_bundle_creator_->CreatePropBundle();
  prop_bundle->SetProps("opacity", 0.5);

  async_painting_context_darwin_->CreatePaintingNode(12, "scroll-view", prop_bundle, false, false,
                                                     12);
  sync_painting_context_darwin_->CreatePaintingNode(11, "scroll-view", prop_bundle, false, false,
                                                    11);

  sync_painting_context_darwin_->OnNodeReady(11);
  async_painting_context_darwin_->OnNodeReady(12);

  sync_painting_context_darwin_->UpdateNodeReadyPatching();
  async_painting_context_darwin_->UpdateNodeReadyPatching();

  static_cast<lynx::tasm::PaintingContextDarwin *>(sync_painting_context_darwin_->impl())
      ->ForceFlush();
  static_cast<lynx::tasm::PaintingContextDarwin *>(async_painting_context_darwin_->impl())
      ->ForceFlush();

  LynxUI *sync_ui = [_uiOwner findUIBySign:11];
  LynxUI *async_ui = [_uiOwner findUIBySign:12];

  XCTAssert(sync_ui.view.layer.opacity == async_ui.view.layer.opacity);
  XCTAssert([sync_ui.tagName isEqualToString:async_ui.tagName]);
  XCTAssert([sync_ui.tagName isEqualToString:@"scroll-view"]);
  XCTAssert([async_ui.tagName isEqualToString:@"scroll-view"]);

  XCTAssert([async_ui.view.lynxSign isEqual:@12]);
  XCTAssert([sync_ui.view.lynxSign isEqual:@11]);
}

- (void)testListEvent {
  std::shared_ptr<lynx::tasm::PropBundle> prop_bundle = prop_bundle_creator_->CreatePropBundle();
  prop_bundle->SetProps("opacity", 0.5);

  {
    auto array = lynx::lepus::CArray::Create();
    array->emplace_back("scrolltolower");
    array->emplace_back("bindEvent");
    array->emplace_back(true);
    array->emplace_back("function");
    prop_bundle->SetEventHandler(PubLepusValue(lynx::lepus::Value(std::move(array))));
  }

  {
    auto array = lynx::lepus::CArray::Create();
    array->emplace_back("scrolltoupper");
    array->emplace_back("bindEvent");
    array->emplace_back(true);
    array->emplace_back("function");
    prop_bundle->SetEventHandler(PubLepusValue(lynx::lepus::Value(std::move(array))));
  }

  async_painting_context_darwin_->CreatePaintingNode(12, "list", prop_bundle, false, false, 12);
  sync_painting_context_darwin_->CreatePaintingNode(11, "list", prop_bundle, false, false, 11);

  sync_painting_context_darwin_->OnNodeReady(11);
  async_painting_context_darwin_->OnNodeReady(12);

  sync_painting_context_darwin_->UpdateNodeReadyPatching();
  async_painting_context_darwin_->UpdateNodeReadyPatching();

  static_cast<lynx::tasm::PaintingContextDarwin *>(sync_painting_context_darwin_->impl())
      ->ForceFlush();
  static_cast<lynx::tasm::PaintingContextDarwin *>(async_painting_context_darwin_->impl())
      ->ForceFlush();

  LynxUI *sync_ui = [_uiOwner findUIBySign:11];
  LynxUI *async_ui = [_uiOwner findUIBySign:12];

  XCTAssert(sync_ui.view.layer.opacity == async_ui.view.layer.opacity);
  XCTAssert([sync_ui.tagName isEqualToString:async_ui.tagName]);
  XCTAssert([sync_ui.tagName isEqualToString:@"list"]);
  XCTAssert([async_ui.tagName isEqualToString:@"list"]);

  XCTAssert([async_ui.view.lynxSign isEqual:@12]);
  XCTAssert([sync_ui.view.lynxSign isEqual:@11]);

  LynxUICollection *async_list = (LynxUICollection *)async_ui;
  LynxUICollection *sync_list = (LynxUICollection *)sync_ui;

  XCTAssert(async_list.scrollEventEmitter.enableScrollToUpperEvent == YES);
  XCTAssert(async_list.scrollEventEmitter.enableScrollToLowerEvent == YES);
  XCTAssert(sync_list.scrollEventEmitter.enableScrollToUpperEvent == YES);
  XCTAssert(sync_list.scrollEventEmitter.enableScrollToLowerEvent == YES);
}

- (void)testImageAutoPlay {
  std::shared_ptr<lynx::tasm::PropBundle> prop_bundle = prop_bundle_creator_->CreatePropBundle();

  async_painting_context_darwin_->CreatePaintingNode(12, "image", prop_bundle, false, false, 12);
  sync_painting_context_darwin_->CreatePaintingNode(11, "image", prop_bundle, false, false, 11);

  sync_painting_context_darwin_->OnNodeReady(11);
  async_painting_context_darwin_->OnNodeReady(12);

  sync_painting_context_darwin_->UpdateNodeReadyPatching();
  async_painting_context_darwin_->UpdateNodeReadyPatching();

  static_cast<lynx::tasm::PaintingContextDarwin *>(sync_painting_context_darwin_->impl())
      ->ForceFlush();
  static_cast<lynx::tasm::PaintingContextDarwin *>(async_painting_context_darwin_->impl())
      ->ForceFlush();

  LynxUI *sync_ui = [_uiOwner findUIBySign:11];
  LynxUI *async_ui = [_uiOwner findUIBySign:12];

  XCTAssert([sync_ui.tagName isEqualToString:@"image"]);
  XCTAssert([async_ui.tagName isEqualToString:@"image"]);

  XCTAssert([async_ui.view.lynxSign isEqual:@12]);
  XCTAssert([sync_ui.view.lynxSign isEqual:@11]);

  XCTAssert([[async_ui valueForKey:@"autoPlay"] boolValue] == YES);
  XCTAssert([[sync_ui valueForKey:@"autoPlay"] boolValue] == YES);
}

@end
