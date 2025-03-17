// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUIUnitTestUtils.h"
#import <Lynx/LynxComponentRegistry.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxUI.h>
#import <Lynx/LynxUIOwner.h>
#import <Lynx/LynxUIView.h>
#import <Lynx/LynxView+Internal.h>
#import <Lynx/LynxView.h>
#import "LynxTemplateRender+Internal.h"

@implementation LynxEventEmitterUnitTestHelper

- (instancetype)init {
  self = [super init];
  if (self) {
    _cachedEvents = [NSMutableArray array];
  }
  return self;
}

- (void)dispatchCustomEvent:(LynxCustomEvent *)event {
  [super dispatchCustomEvent:event];
  _event = event;
  [_cachedEvents addObject:event];
}

- (void)clearEvent {
  _event = nil;
}

@end

@implementation LynxUIMockContext
@end

@implementation LynxUIUnitTestUtils

+ (LynxUIMockContext *)initUIMockContextWithUI:(LynxUI *)ui {
  LynxUIMockContext *context = [[LynxUIMockContext alloc] init];
  context.mockUI = ui;
  [context.mockUI updateFrame:CGRectMake(0, 0, 428, 926)
                  withPadding:UIEdgeInsetsZero
                       border:UIEdgeInsetsZero
          withLayoutAnimation:false];

  // make strong reference to eventEmitter and UIContext, or they will only have weak references.
  context.mockEventEmitter = [[LynxEventEmitterUnitTestHelper alloc] init];
  context.mockUIContext = [[LynxUIContext alloc] init];

  context.mockUIContext.eventEmitter = context.mockEventEmitter;
  context.mockUI.context = context.mockUIContext;
  return context;
}

+ (LynxUIMockContext *)updateUIMockContext:(nullable LynxUIMockContext *)mockContext
                                      sign:(NSInteger)sign
                                       tag:(NSString *)tagName
                                  eventSet:(NSSet *)eventSet
                             lepusEventSet:(NSSet *)lepusEventSet
                                     props:(nonnull NSDictionary *)props {
  if (!mockContext) {
    mockContext = [[LynxUIMockContext alloc] init];
  }
  if (!mockContext.rootView) {
    LynxView *view = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder *builder){
    }];
    mockContext.rootView = view;
    mockContext.UIOwner = [view.templateRender uiOwner];
    [mockContext.UIOwner createUIWithSign:-1
                                  tagName:@"page"
                                 eventSet:eventSet
                            lepusEventSet:lepusEventSet
                                    props:props
                                nodeIndex:0
                       gestureDetectorSet:nil];
  }
  [mockContext.UIOwner createUIWithSign:sign
                                tagName:tagName
                               eventSet:eventSet
                          lepusEventSet:lepusEventSet
                                  props:props
                              nodeIndex:0
                     gestureDetectorSet:nil];
  LynxUI *ui = [mockContext.UIOwner findUIBySign:sign];
  [ui updateFrame:CGRectMake(0, 0, 428, 926)
              withPadding:UIEdgeInsetsZero
                   border:UIEdgeInsetsZero
      withLayoutAnimation:false];

  // make strong reference to eventEmitter and UIContext, or they will only have weak references.
  if (!mockContext.mockEventEmitter) {
    mockContext.mockEventEmitter = [[LynxEventEmitterUnitTestHelper alloc] init];
  }

  ui.context.eventEmitter = mockContext.mockEventEmitter;
  return mockContext;
}

@end
