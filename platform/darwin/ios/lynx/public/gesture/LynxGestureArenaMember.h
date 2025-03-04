// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef LynxGestureArenaMember_h
#define LynxGestureArenaMember_h

#import <Foundation/Foundation.h>
#import <Lynx/LynxEventTarget.h>
#import <Lynx/LynxGestureDetectorDarwin.h>
#import <UIKit/UIKit.h>
@class LynxBaseGestureHandler;

@protocol LynxGestureArenaMember <LynxEventTarget>
/**
 Called when the gesture should be scrolled by the specified delta values.
 @param delta The delta value for scrolling offset
 */
- (void)onGestureScrollBy:(CGPoint)delta;

/**
 Check if the gesture can consume the specified delta values.
 @param delta The delta value for scrolling offset
 */
- (BOOL)canConsumeGesture:(CGPoint)delta;

/**
 get current scroller container is at border edge or not
 @param start if it is at start or end
 @return YES — at border edge,  NO — not at border edge
 */
- (BOOL)getGestureBorder:(BOOL)start;

/**
 Retrieve the ID of the gesture arena member.
 @return The ID of the gesture arena member.
 */
- (NSInteger)getGestureArenaMemberId;

/**
 Retrieve the scroll position of the member in the x-axis.
 @return The scroll position in the x-axis.
 */
- (CGFloat)getMemberScrollX;

/**
 Retrieve the scroll position of the member in the y-axis.
 @return The scroll position in the y-axis.
 */
- (CGFloat)getMemberScrollY;

/**
 Retrieve  the map of gesture detectors associated with the member.
 @return The map of gesture detectors, or null if not available.
 */
- (NSDictionary<NSNumber *, LynxGestureDetectorDarwin *> *)getGestureDetectorMap;

/**
 Retrieve the map of gesture handler associated with th member
 @return The map of gesture handlers, or null if not available,
 @see GestureDetector key —— GestureDetector type value —— gesture handler
 */
- (NSDictionary<NSNumber *, LynxBaseGestureHandler *> *)getGestureHandlers;

@end

#endif /* LynxGestureArenaMember_h */
