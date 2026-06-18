// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef LynxKeyboardEventDispatcher_h
#define LynxKeyboardEventDispatcher_h

#import <Lynx/LynxContext.h>

NS_ASSUME_NONNULL_BEGIN

@protocol LynxKeyboardEventObserver <NSObject>

- (void)keyboardWillShow:(CGFloat)keyboardHeight;

- (void)keyboardWillHide;

@end

@interface LynxKeyboardEventDispatcher : NSObject

- (instancetype)initWithContext:(LynxContext *)context;
- (void)addKeyboardEventObserver:(id<LynxKeyboardEventObserver>)observer;
- (void)inputDidBeginEditingForOwner:(id)owner
                            rootView:(UIView *)rootView
                           inputView:(UIView *)inputView
                       avoidKeyboard:(BOOL)avoidKeyboard
                             spacing:(CGFloat)spacing;
- (void)inputDidEndEditingForOwner:(id)owner finalHideBlock:(dispatch_block_t)block;
- (void)inputDidLayoutForOwner:(id)owner
                      rootView:(UIView *)rootView
                     inputView:(UIView *)inputView
                 avoidKeyboard:(BOOL)avoidKeyboard
                       spacing:(CGFloat)spacing;
- (void)avoidKeyboardPropsDidChangeForOwner:(id)owner
                                   rootView:(UIView *)rootView
                                  inputView:(UIView *)inputView
                              avoidKeyboard:(BOOL)avoidKeyboard
                                    spacing:(CGFloat)spacing;
- (void)keyboardWillShowForOwner:(id)owner
                        rootView:(UIView *)rootView
                       inputView:(UIView *)inputView
                   avoidKeyboard:(BOOL)avoidKeyboard
                         spacing:(CGFloat)spacing
                          height:(CGFloat)height
                        duration:(NSTimeInterval)duration
                           curve:(UIViewAnimationCurve)curve;
- (void)keyboardWillHideForOwner:(id)owner
                        duration:(NSTimeInterval)duration
                           curve:(UIViewAnimationCurve)curve
                  finalHideBlock:(dispatch_block_t)block;
@end

NS_ASSUME_NONNULL_END

#endif /* LynxKeyboardEventDispatcher_h */
