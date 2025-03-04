// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/*
 Raw event from front-end will contain type like 'touchstart(bindEvent)',
 LynxEventSpec will parse the raw event to event name and type.
 */
@interface LynxEventSpec : NSObject

@property(nonatomic, copy) NSString* name;

@property(nonatomic) BOOL shouldCaptureBubble;
@property(nonatomic) BOOL shouldCaptureCatch;
@property(nonatomic) BOOL interestedInBubble;
@property(nonatomic) BOOL interestedInCatch;
@property(nonatomic) BOOL shouldLepusCaptureBubble;
@property(nonatomic) BOOL shouldLepusCaptureCatch;
@property(nonatomic) BOOL interestedInLepusBubble;
@property(nonatomic) BOOL interestedInLepusCatch;

- (instancetype)initWithRawEvent:(NSString*)event withJSEvent:(BOOL)isJSEvent;
- (void)composeWithOtherSpec:(LynxEventSpec*)spec;

+ (nullable NSDictionary<NSString*, LynxEventSpec*>*)
     convertRawEvents:(nullable NSSet<NSString*>*)set
    andRwaLepusEvents:(nullable NSSet<NSString*>*)lepusSet;

@end

NS_ASSUME_NONNULL_END
