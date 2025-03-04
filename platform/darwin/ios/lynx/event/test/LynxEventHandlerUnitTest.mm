// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxEventHandlerUnitTest.h"
#import <Lynx/LynxPropsProcessor.h>
#import "LynxEventHandler+Internal.h"
#import "LynxRootUI.h"
#import "LynxTransformRaw.h"
#import "LynxUIContext.h"
#import "LynxUIView.h"

@interface LynxUI ()
@property(nonatomic, weak) LynxUIContext* context;
@end

@interface LynxUIContext ()
@property(nonatomic, weak) LynxEventHandler* eventHandler;
@end

@implementation LynxEventHanderUnitTest {
  LynxEventHandler* _handler;
}

- (void)setUp {
  // Put setup code here. This method is called before the invocation of each test method in the
  // class.
  LynxRootUI* rootUI = [[LynxRootUI alloc] initWithLynxView:(LynxView*)[UIView new]];
  _handler = OCMPartialMock([[LynxEventHandler alloc] initWithRootView:rootUI.view
                                                            withRootUI:rootUI]);
}

- (void)tearDown {
  // Put teardown code here. This method is called after the invocation of each test method in the
  // class.
  _handler = NULL;
}

- (void)testOnGestureRecognizedByEventTargetAsync {
  LynxUIView* mockUI1 = OCMPartialMock([[LynxUIView alloc] initWithView:[UIView new]]);
  LynxUIView* mockUI2 = OCMPartialMock([[LynxUIView alloc] initWithView:[UIView new]]);
  [mockUI1 insertChild:mockUI2 atIndex:0];

  id mockCtx = OCMPartialMock([[LynxUIContext alloc] init]);
  ((LynxUIContext*)mockCtx).eventHandler = _handler;
  OCMStub([mockCtx isTouchMoving]).andReturn(YES);
  NSArray* transformRaw = @[ @[
    @(LynxTransformTypeTranslateX), @10, @(LynxPlatformLengthUnitNumber), @0,
    @(LynxPlatformLengthUnitNumber), @0, @(LynxPlatformLengthUnitNumber)
  ] ];

  dispatch_queue_t concurrentQueue1 = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
  dispatch_queue_t concurrentQueue2 = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
  NSMutableArray* uiArray = [NSMutableArray new];
  for (size_t i = 0; i < 2000; ++i) {
    LynxUI* ui = [[LynxUI alloc] init];
    ui.context = mockCtx;
    ui.sign = i;
    [uiArray addObject:ui];
  }
  dispatch_async(concurrentQueue1, ^{
    for (size_t i = 0; i < 1000; ++i) {
      [LynxPropsProcessor updateProp:transformRaw withKey:@"transform" forUI:uiArray[i]];
    }
  });
  dispatch_async(concurrentQueue2, ^{
    for (size_t i = 1000; i < 2000; ++i) {
      [LynxPropsProcessor updateProp:transformRaw withKey:@"transform" forUI:uiArray[i]];
    }
  });

  NSDate* timeoutDate = [NSDate dateWithTimeIntervalSinceNow:6];
  while ([timeoutDate timeIntervalSinceNow] > 0) {
    [[NSRunLoop mainRunLoop] runMode:NSDefaultRunLoopMode
                          beforeDate:[NSDate dateWithTimeIntervalSinceNow:1]];
  }
  [uiArray enumerateObjectsUsingBlock:^(id _Nonnull ui, NSUInteger idx, BOOL* _Nonnull stop) {
    [(LynxUI*)ui performSelector:@selector(onNodeReadyForUIOwner)];
  }];
  XCTAssertEqual([(NSMutableSet*)[_handler valueForKey:@"_setOfPropsChanged"] count], 2000);
}

@end
