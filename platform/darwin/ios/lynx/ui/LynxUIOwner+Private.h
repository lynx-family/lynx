// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import "LynxUIOwner.h"

NS_ASSUME_NONNULL_BEGIN

// Indicate that whether LynxUIOwner supports specific tagName, there are currently only three
// states: root tag, supported tag, and unsupported tag.
typedef enum : NSInteger {
  LynxRootTag = 0,
  LynxSupportedTag,
  LynxUnsupportedTag
} TagSupportedState;

@interface LynxUIOwner (Private)

// Given a specific tagName and props, return the corresponding Class and whether the tag is
// supported or not.
- (Class)getTargetClass:(NSString*)tagName
                  props:(NSDictionary*)props
         supportedState:(TagSupportedState*)state;

// Given a specific tagName, return whether the class can be created on background thread.
- (BOOL)needCreateUIAsync:(NSString*)tagName;

// Given a Class and props, create the corresponding LynxUI instance, using different LynxUI init
// methods depending on whether it is called on the main thread or not.
- (LynxUI*)createUIWithClass:(Class)clazz
              supportedState:(TagSupportedState)state
                onMainThread:(BOOL)onMainThread;

// Synchronously create LynxUI, once this method is verified as stable, will replace the current
// createUIWithSign method with this method.
- (void)createUISyncWithSign:(NSInteger)sign
                     tagName:(NSString*)tagName
                       clazz:(Class)clazz
              supportedState:(TagSupportedState)state
                    eventSet:(NSSet<NSString*>*)eventSet
               lepusEventSet:(NSSet<NSString*>*)lepusEventSet
                       props:(NSDictionary*)props
                   nodeIndex:(uint32_t)nodeIndex
          gestureDetectorSet:(NSSet<LynxGestureDetectorDarwin*>*)gestureDetectorSet;

// Asynchronously create LynxUI, returning a block. All methods that must be executed on the main
// thread will be encapsulated in this block, including creating the View, recording LynxUI, etc.
// The PaintingContext can Enqueue the return block.
- (LynxUI*)createUIAsyncWithSign:(NSInteger)sign
                         tagName:(NSString*)tagName
                           clazz:(Class)clazz
                  supportedState:(TagSupportedState)state
                        eventSet:(NSSet<NSString*>*)eventSet
                   lepusEventSet:(NSSet<NSString*>*)lepusEventSet
                           props:(NSDictionary*)props
                       nodeIndex:(uint32_t)nodeIndex
              gestureDetectorSet:(NSSet<LynxGestureDetectorDarwin*>*)gestureDetectorSet;

- (void)processUIOnMainThread:(LynxUI*)ui
                     withSign:(NSInteger)sign
                      tagName:(NSString*)tagName
                        props:(NSDictionary*)props;

@end

NS_ASSUME_NONNULL_END
