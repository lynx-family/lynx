// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#import <AnimaX/AnimaXView.h>
#import <Foundation/Foundation.h>
#import <Lynx/LynxEnv.h>
#import <Lynx/LynxTemplateRender+Internal.h>
#import <Lynx/LynxTemplateRender+Protected.h>
#import <Lynx/LynxUI+Private.h>
#import <Lynx/LynxView+Internal.h>
#import <XCTest/XCTest.h>
#import <objc/runtime.h>

#include <stdio.h>
#include <memory>
#include "core/renderer/ui_wrapper/common/ios/prop_bundle_darwin.h"
#include "core/renderer/ui_wrapper/painting/ios/painting_context_darwin.h"
#include "core/renderer/ui_wrapper/painting/painting_context.h"
#include "core/shell/lynx_shell.h"
#include "src/model/value/base_value.h"

@interface AnimaXView (LynxUT)

@property(nonatomic) CGFloat currentAlpha;

@end

@implementation AnimaXView (LynxUT)

@dynamic currentAlpha;

+ (void)load {
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    SEL originalSelector = NSSelectorFromString(@"setAlpha:");
    SEL swizzledSelector = @selector(swizzled_setAlpha:);

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
  });
}

- (void)swizzled_setAlpha:(CGFloat)alpha {
  self.currentAlpha = alpha;
  return [self swizzled_setAlpha:alpha];
}

- (void)setCurrentAlpha:(CGFloat)currentAlpha {
  NSNumber *number = [NSNumber numberWithFloat:currentAlpha];
  objc_setAssociatedObject(self, @selector(currentAlpha), number,
                           OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

- (CGFloat)currentAlpha {
  NSNumber *number = objc_getAssociatedObject(self, @selector(currentAlpha));
  return [number floatValue];
}

@end

@interface LynxUIAnimaXUnitTest : XCTestCase

@end

@implementation LynxUIAnimaXUnitTest {
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
                                      componentRegistry:[LynxComponentScopeRegistry new]
                                          screenMetrics:screenMetrics];

  sync_painting_context_darwin_ = std::make_unique<lynx::tasm::PaintingContext>(
      std::make_unique<lynx::tasm::PaintingContextDarwin>(_uiOwner, false));
  async_painting_context_darwin_ = std::make_unique<lynx::tasm::PaintingContext>(
      std::make_unique<lynx::tasm::PaintingContextDarwin>(_uiOwner, false));

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
  auto prop_bundle = prop_bundle_creator_->CreatePropBundle();
  prop_bundle->SetProps("opacity", 0.5);

  async_painting_context_darwin_->CreatePaintingNode(12, "animax-view", prop_bundle, false, false,
                                                     12);
  sync_painting_context_darwin_->CreatePaintingNode(11, "animax-view", prop_bundle, false, false,
                                                    11);

  sync_painting_context_darwin_->OnNodeReady(11);
  sync_painting_context_darwin_->OnNodeReady(12);

  sync_painting_context_darwin_->UpdateNodeReadyPatching();
  sync_painting_context_darwin_->OnNodeReady(12);

  static_cast<lynx::tasm::PaintingContextDarwin *>(sync_painting_context_darwin_->impl())
      ->ForceFlush();
  static_cast<lynx::tasm::PaintingContextDarwin *>(async_painting_context_darwin_->impl())
      ->ForceFlush();

  LynxUI *sync_ui = [_uiOwner findUIBySign:11];
  LynxUI *async_ui = [_uiOwner findUIBySign:12];

  XCTAssert(sync_ui.view.layer.opacity == async_ui.view.layer.opacity);
}

@end
