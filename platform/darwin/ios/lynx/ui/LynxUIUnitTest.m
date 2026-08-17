// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBaseGestureHandler.h>
#import <Lynx/LynxGestureDetectorDarwin.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxUI+Private.h>
#import <Lynx/LynxUIContext.h>
#import <Lynx/LynxUIFilterImage.h>
#import <Lynx/LynxUIView.h>
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>
#import <malloc/malloc.h>
#include <objc/runtime.h>
#import "LynxGestureArenaManager.h"
#import "LynxUI+Gesture.h"
#import "LynxUI+Private.h"
#import "LynxUIContext+Internal.h"
#import "LynxUIUnitTestUtils.h"

@implementation LynxUI (Test)
- (UIView *)createView {
  return nil;
}
@end

@interface LynxUIContext (NewStickyUnitTest)
- (void)setEnableNewSticky:(BOOL)enable;
@end

@interface LynxUI (BorderRadiusMaskUpdateUnitTest)
- (void)scheduleBorderRadiusMaskUpdate;
- (void)consumeScheduledBorderRadiusMaskUpdate;
@end

@interface LynxUIMaskUpdateCounter : LynxUI
@property(nonatomic, assign) NSInteger maskUpdateCount;
@end

@implementation LynxUIMaskUpdateCounter
- (bool)updateLayerMaskOnFrameChanged {
  [self consumeScheduledBorderRadiusMaskUpdate];
  self.maskUpdateCount += 1;
  return true;
}
@end

@interface LynxUIUnitTest : XCTestCase
@end
@implementation LynxUIUnitTest

- (void)setUp {
  // Put setup code here. This method is called before the invocation of each test method in the
  // class.
}

- (void)tearDown {
  // Put teardown code here. This method is called after the invocation of each test method in the
  // class.
}

- (void)testIsVisible {
  LynxUI *ui = OCMPartialMock([[LynxUI alloc] initWithView:nil]);
  XCTAssertFalse([ui isVisible]);

  UIView *view = OCMClassMock([UIView class]);
  CGRect rect = CGRectZero;
  ui = OCMPartialMock([[LynxUI alloc] initWithView:view]);
  OCMExpect([view isHidden]).andReturn(NO);
  OCMExpect([view alpha]).andReturn(1);
  OCMExpect([view frame]).andReturn(rect);
  OCMExpect([view clipsToBounds]).andReturn(NO);
  OCMExpect([view window]).andReturn(OCMClassMock([UIWindow class]));

  XCTAssertTrue([ui isVisible]);
  OCMVerifyAll(ui);
}

- (void)testCopy {
  LynxUIView *parent = [[LynxUIView alloc] initWithView:nil];
  [parent updateFrame:CGRectMake(0, 0, 100, 100)
              withPadding:UIEdgeInsetsZero
                   border:UIEdgeInsetsZero
      withLayoutAnimation:NO];
  [LynxPropsProcessor updateProp:UIColor.redColor withKey:@"background-color" forUI:parent];

  LynxUIView *child = [[LynxUIView alloc] initWithView:nil];
  [child updateFrame:CGRectMake(0, 0, 50, 50)
              withPadding:UIEdgeInsetsZero
                   border:UIEdgeInsetsZero
      withLayoutAnimation:NO];
  [LynxPropsProcessor updateProp:UIColor.greenColor withKey:@"background-color" forUI:child];

  [parent insertChild:child atIndex:0];

  [parent propsDidUpdate];
  [child propsDidUpdate];
  [parent onNodeReadyForUIOwner];
  [child onNodeReadyForUIOwner];

  LynxUIView *copy = [parent copy];

  XCTAssert([copy isKindOfClass:LynxUIView.class]);
  XCTAssert(copy.frame.size.width == 100);
  XCTAssert(copy.frame.size.height == 100);
  XCTAssert(copy.backgroundManager.backgroundColor == parent.backgroundManager.backgroundColor);

  LynxUIView *copyChild = parent.children.firstObject;

  XCTAssert([copyChild isKindOfClass:LynxUIView.class]);
  XCTAssert(copyChild.frame.size.width == 50);
  XCTAssert(copyChild.frame.size.height == 50);
  XCTAssert(copyChild.backgroundManager.backgroundColor == child.backgroundManager.backgroundColor);
}

- (void)testBorderRadiusUpdateRefreshesNonUniformOverflowMask {
  LynxUIView *ui = [[LynxUIView alloc] init];
  [ui updateFrame:CGRectMake(0, 0, 100, 80)
              withPadding:UIEdgeInsetsZero
                   border:UIEdgeInsetsZero
      withLayoutAnimation:NO];
  [LynxPropsProcessor updateProp:@1 withKey:@"overflow" forUI:ui];
  [ui onNodeReadyForUIOwner];

  [LynxPropsProcessor
      updateProp:@[ @10, @0, @20, @0, @30, @0, @40, @0, @50, @0, @60, @0, @70, @0, @80, @0 ]
         withKey:@"border-radius"
           forUI:ui];
  [LynxPropsProcessor updateProp:@[ @25, @0, @35, @0 ] withKey:@"border-top-left-radius" forUI:ui];
  [ui onNodeReadyForUIOwner];

  XCTAssertNotNil(ui.view.layer.mask);
}

- (void)testBorderRadiusMaskUpdateCoalescesAndImmediateUpdateCancelsPendingBlock {
  LynxUIMaskUpdateCounter *ui = [[LynxUIMaskUpdateCounter alloc] init];

  [ui scheduleBorderRadiusMaskUpdate];
  [ui scheduleBorderRadiusMaskUpdate];
  XCTAssertEqual(ui.nodeReadyBlockArray.count, 1U);
  [ui onNodeReadyForUIOwner];
  XCTAssertEqual(ui.maskUpdateCount, 1);

  [ui scheduleBorderRadiusMaskUpdate];
  XCTAssertEqual(ui.nodeReadyBlockArray.count, 1U);
  [ui updateLayerMaskOnFrameChanged];
  [ui onNodeReadyForUIOwner];
  XCTAssertEqual(ui.maskUpdateCount, 2);
}

- (void)testFilterImageBorderRadiusUpdateClearsMaskAtZero {
  LynxUIFilterImage *ui = [[LynxUIFilterImage alloc] init];
  [ui updateFrame:CGRectMake(0, 0, 100, 80)
              withPadding:UIEdgeInsetsZero
                   border:UIEdgeInsetsZero
      withLayoutAnimation:NO];

  [LynxPropsProcessor
      updateProp:@[ @10, @0, @10, @0, @10, @0, @10, @0, @10, @0, @10, @0, @10, @0, @10, @0 ]
         withKey:@"border-radius"
           forUI:ui];
  [ui onNodeReadyForUIOwner];
  XCTAssertNotNil(ui.view.layer.mask);

  [LynxPropsProcessor updateProp:@[ @0, @0, @0, @0, @0, @0, @0, @0, @0, @0, @0, @0, @0, @0, @0, @0 ]
                         withKey:@"border-radius"
                           forUI:ui];
  [ui onNodeReadyForUIOwner];
  XCTAssertNil(ui.view.layer.mask);
}

- (void)testGestureInterfaces {
  LynxUIView *ui = [[LynxUIView alloc] initWithView:nil];

  [ui setGestureDetectorState:1 state:LynxGestureStateActive];
  [ui setGestureDetectorState:1 state:LynxGestureStateFail];
  [ui setGestureDetectorState:1 state:LynxGestureStateEnd];

  [ui onGestureScrollBy:CGPointMake(10, 10)];
  XCTAssertTrue([ui canConsumeGesture:CGPointMake(10, 10)]);
  XCTAssertTrue([ui getGestureArenaMemberId] == 0);
  XCTAssertTrue([ui getMemberScrollX] == 0.0f);
  XCTAssertTrue([ui getMemberScrollY] == 0.0f);
  XCTAssertTrue([ui getGestureDetectorMap].count == 0);
  XCTAssertTrue([ui getGestureHandlers].count == 0);
}

- (void)testGestureDetectorsRefreshHandlersAndSupportClearing {
  LynxUIView *ui = [[LynxUIView alloc] initWithView:nil];
  LynxUIMockContext *mockContext = [LynxUIUnitTestUtils initUIMockContextWithUI:ui];
  [mockContext.mockUIContext setEnableNewGesture:YES];
  LynxUIOwner *uiOwner = OCMClassMock([LynxUIOwner class]);
  LynxGestureArenaManager *manager = OCMPartialMock([[LynxGestureArenaManager alloc] init]);
  mockContext.mockUIContext.uiOwner = uiOwner;
  OCMStub([uiOwner gestureArenaManager]).andReturn(manager);

  LynxGestureDetectorDarwin *firstDetector =
      [[LynxGestureDetectorDarwin alloc] initWithGestureID:1
                                               gestureType:LynxGestureTypeLongPress
                                      gestureCallbackNames:@[ @"onStart" ]
                                               relationMap:@{}];
  NSDictionary *firstDetectorMap = @{@1 : firstDetector};
  OCMExpect([manager registerGestureDetectors:1 detectorMap:firstDetectorMap]);
  [ui setGestureDetectors:[NSSet setWithObject:firstDetector]];

  LynxBaseGestureHandler *firstHandler = [ui getGestureHandlers].allValues.firstObject;
  XCTAssertEqual(firstHandler.gestureDetector.gestureID, 1);
  [firstHandler begin];

  LynxGestureDetectorDarwin *secondDetector =
      [[LynxGestureDetectorDarwin alloc] initWithGestureID:2
                                               gestureType:LynxGestureTypeLongPress
                                      gestureCallbackNames:@[ @"onStart" ]
                                               relationMap:@{}];
  NSDictionary *secondDetectorMap = @{@2 : secondDetector};
  OCMExpect([manager unregisterGestureDetectors:1 detectorMap:firstDetectorMap]);
  OCMExpect([manager registerGestureDetectors:1 detectorMap:secondDetectorMap]);
  [ui setGestureDetectors:[NSSet setWithObject:secondDetector]];

  XCTAssertEqual(firstHandler.status, LynxGestureHandlerStateCancel);
  LynxBaseGestureHandler *secondHandler = [ui getGestureHandlers].allValues.firstObject;
  XCTAssertEqual(secondHandler.gestureDetector.gestureID, 2);
  [secondHandler activate];

  OCMExpect([manager unregisterGestureDetectors:1 detectorMap:secondDetectorMap]);
  OCMExpect([manager removeMember:ui detectorMap:@{}]);
  [ui setGestureDetectors:[NSSet set]];

  XCTAssertEqual(secondHandler.status, LynxGestureHandlerStateCancel);
  XCTAssertEqual([ui getGestureDetectorMap].count, 0);
  XCTAssertEqual([ui getGestureHandlers].count, 0);
  XCTAssertEqual([ui getGestureArenaMemberId], 0);
  OCMVerifyAll(manager);
}

void printAllIvarDetails(Class cls) {
  unsigned int ivarCount = 0;
  Ivar *ivars = class_copyIvarList(cls, &ivarCount);

  // Print the table header
  printf("| %-20s | %-10s | %-10s | %-10s |\n", "Ivar Name", "Type", "Size (bytes)", "Offset");
  printf("|%21s|%11s|%11s|%10s|\n", "---------------------", "----------", "----------",
         "----------");

  for (unsigned int i = 0; i < ivarCount; i++) {
    Ivar ivar = ivars[i];
    const char *ivarName = ivar_getName(ivar);
    const char *ivarType = ivar_getTypeEncoding(ivar);
    ptrdiff_t ivarOffset = ivar_getOffset(ivar);

    NSUInteger size;
    NSUInteger alignment;
    NSGetSizeAndAlignment(ivarType, &size, &alignment);

    printf("| %-20s | %-10s | %-10lu | %-10td |\n", ivarName, ivarType, (unsigned long)size,
           ivarOffset);
  }

  free(ivars);
}

- (void)testMemoryPadding {
  LynxUI *ui = [[LynxUI alloc] init];
  unsigned long original_size = class_getInstanceSize([ui class]);
  unsigned long padding_size = malloc_size((__bridge const void *)(ui));
  printf(@"Size of %@: %zd", NSStringFromClass([ui class]), original_size);
  printf(@"Size of %@: %zd", @"malloc size", padding_size);

  if (padding_size - original_size >= 100) {
    printAllIvarDetails([ui class]);
    XCTFail("LynxUI size is beyond bar causing memory padding, wasting memory usage. Please check "
            "its ivar to reduce it size under bar.");
  }
}

- (void)testNewStickyCalculateTranslateClampsToParentRange {
  LynxUIView *sticky = [[LynxUIView alloc] init];
  LynxUIMockContext *mockContext = [LynxUIUnitTestUtils initUIMockContextWithUI:sticky];
  [mockContext.mockUIContext setEnableNewSticky:YES];
  [sticky updateFrame:CGRectMake(0, 100, 50, 40)
              withPadding:UIEdgeInsetsZero
                   border:UIEdgeInsetsZero
      withLayoutAnimation:NO];

  [sticky updateSticky:@[ @0, @10, @0, @0, @100, @200, @0, @100, @0, @50 ]];
  // The 10-field payload includes parent size and relative offsets to the scroller.
  // Parent bottom clamps the translation to 110 instead of using the larger self-only range.
  [sticky calculateStickyTranslateWithOffset:250 isVertical:YES scrollerSize:200 maxOffset:300];

  XCTAssertEqualWithAccuracy(sticky.backgroundManager.postTranslate.y, 110.f, 0.001f);
}

- (void)testNewStickyInvalidPayloadClearsStickyStateAndTranslate {
  LynxUIView *sticky = [[LynxUIView alloc] init];
  LynxUIMockContext *mockContext = [LynxUIUnitTestUtils initUIMockContextWithUI:sticky];
  [mockContext.mockUIContext setEnableNewSticky:YES];
  [sticky updateFrame:CGRectMake(0, 100, 50, 40)
              withPadding:UIEdgeInsetsZero
                   border:UIEdgeInsetsZero
      withLayoutAnimation:NO];

  // 1. First apply a valid 10-field payload, then pass 4-field sticky info.
  // The new sticky path should clear stale sticky state and postTranslate.
  [sticky updateSticky:@[ @0, @10, @0, @0, @(-1), @(-1), @0, @100, @0, @0 ]];
  [sticky calculateStickyTranslateWithOffset:150 isVertical:YES scrollerSize:200 maxOffset:300];
  XCTAssertEqualWithAccuracy(sticky.backgroundManager.postTranslate.y, 60.f, 0.001f);

  [sticky updateSticky:@[ @0, @10, @0, @0 ]];

  XCTAssertNil(sticky.sticky);
  XCTAssertTrue(CGPointEqualToPoint(sticky.backgroundManager.postTranslate, CGPointZero));

  // 2. Apply a valid 10-field payload, then pass nil sticky info.
  // The new sticky path should clear stale sticky state and postTranslate.
  [sticky updateSticky:@[ @0, @10, @0, @0, @(-1), @(-1), @0, @100, @0, @0 ]];
  [sticky calculateStickyTranslateWithOffset:150 isVertical:YES scrollerSize:200 maxOffset:300];
  XCTAssertEqualWithAccuracy(sticky.backgroundManager.postTranslate.y, 60.f, 0.001f);

  [sticky updateSticky:nil];

  XCTAssertNil(sticky.sticky);
  XCTAssertTrue(CGPointEqualToPoint(sticky.backgroundManager.postTranslate, CGPointZero));
}

@end
