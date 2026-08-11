// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "core/renderer/ui_wrapper/painting/ios/painting_context_darwin.h"
#import <Lynx/LynxPerformanceController.h>
#import <Lynx/LynxTemplateRender.h>
#import <Lynx/LynxUIOwner.h>
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>
#import "LynxTimingConstants.h"
#include "base/include/fml/message_loop.h"
#import "core/renderer/ui_wrapper/painting/ios/native_painting_context_darwin.h"
#import "core/renderer/ui_wrapper/painting/ios/painting_context_darwin_utils.h"
#include "core/shell/dynamic_ui_operation_queue.h"

@interface painting_context_darwin_UnitTest : XCTestCase {
  std::unique_ptr<lynx::tasm::PaintingContextDarwin> paintingContext;
}

@end

@implementation painting_context_darwin_UnitTest

- (void)setUp {
  LynxUIOwner* uiOwner = [[LynxUIOwner alloc] initWithContainerView:nil
                                                  componentRegistry:nil
                                                      screenMetrics:nil];
  paintingContext = std::make_unique<lynx::tasm::PaintingContextDarwin>(uiOwner, false);
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

- (void)testNativePaintingContextMarksPaintEndTiming {
  LynxUIOwner* uiOwner = [[LynxUIOwner alloc] initWithContainerView:nil
                                                  componentRegistry:nil
                                                      screenMetrics:nil];
  auto nativePaintingContext =
      std::make_unique<lynx::tasm::NativePaintingCtxDarwin>(uiOwner, nullptr);
  lynx::fml::MessageLoop::EnsureInitializedForCurrentThread();
  auto queue = std::make_shared<lynx::shell::DynamicUIOperationQueue>(
      lynx::base::ThreadStrategyForRendering::ALL_ON_UI,
      lynx::fml::MessageLoop::GetCurrent().GetTaskRunner());
  nativePaintingContext->SetUIOperationQueue(queue);

  id performanceController = OCMClassMock([LynxPerformanceController class]);
  lynx::tasm::PaintingContextDarwinUtils::SetPerformanceController(
      nativePaintingContext->GetPlatformRef().get(), performanceController);

  auto options = std::make_shared<lynx::tasm::PipelineOptions>();
  options->need_timestamps = true;
  options->has_layout = true;
  NSString* pipelineID = [NSString stringWithUTF8String:options->pipeline_id.c_str()];
  OCMExpect([performanceController markTiming:kTimingPaintEnd pipelineID:pipelineID]);

  nativePaintingContext->OnFirstScreen();
  nativePaintingContext->FinishLayoutOperation(options);
  queue->ForceFlush();

  OCMVerifyAllWithDelay(performanceController, 1.0);
}

@end
